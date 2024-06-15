//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#pragma once

#include <vector>                                      // std::vector
#include <deque>                                       // std::deque
#include <map>                                         // std::map
#include <melon/utility/containers/flat_map.h>                  // FlatMap
#include <melon/utility/containers/doubly_buffered_data.h>      // DoublyBufferedData
#include <melon/utility/containers/bounded_queue.h>             // BoundedQueue
#include <melon/rpc/load_balancer.h>
#include <melon/rpc/controller.h>


namespace melon::lb {

    DECLARE_int64(min_weight);
    DECLARE_double(punish_inflight_ratio);

    // Locality-aware is an iterative algorithm to send requests to servers which
    // have lowest expected latencies. Read docs/cn/lalb.md to get a peek at the
    // algorithm. The implementation is complex.
    class LocalityAwareLoadBalancer : public LoadBalancer {
    public:
        LocalityAwareLoadBalancer();

        ~LocalityAwareLoadBalancer();

        bool AddServer(const ServerId &id);

        bool RemoveServer(const ServerId &id);

        size_t AddServersInBatch(const std::vector<ServerId> &servers);

        size_t RemoveServersInBatch(const std::vector<ServerId> &servers);

        LocalityAwareLoadBalancer *New(const std::string_view &) const;

        void Destroy();

        int SelectServer(const SelectIn &in, SelectOut *out);

        void Feedback(const CallInfo &info);

        void Describe(std::ostream &os, const DescribeOptions &options);

    private:
        struct TimeInfo {
            int64_t latency_sum;         // microseconds
            int64_t end_time_us;
        };

        class Servers;

        class Weight {
            friend class Servers;

        public:
            static const int RECV_QUEUE_SIZE = 128;

            explicit Weight(int64_t initial_weight);

            ~Weight();

            // Called in Feedback() to recalculate _weight.
            // Returns diff of _weight.
            int64_t Update(const CallInfo &, size_t index);

            // Weight of self. Notice that this value may change at any time.
            int64_t volatile_value() const { return _weight; }

            struct AddInflightResult {
                bool chosen;
                int64_t weight_diff;
            };

            AddInflightResult AddInflight(
                    const SelectIn &in, size_t index, int64_t dice);

            int64_t MarkFailed(size_t index, int64_t avg_weight);

            void Describe(std::ostream &os, int64_t now);

            int64_t Disable();

            bool Disabled() const { return _base_weight < 0; }

            int64_t MarkOld(size_t index);

            std::pair<int64_t, int64_t> ClearOld();

            int64_t ResetWeight(size_t index, int64_t now_us);

        private:
            int64_t _weight;
            int64_t _base_weight;
            mutil::Mutex _mutex;
            int64_t _begin_time_sum;
            int _begin_time_count;
            int64_t _old_diff_sum;
            size_t _old_index;
            int64_t _old_weight;
            int64_t _avg_latency;
            mutil::BoundedQueue<TimeInfo> _time_q;
            // content of _time_q
            TimeInfo _time_q_items[RECV_QUEUE_SIZE];
        };

        struct ServerInfo {
            SocketId server_id;
            std::atomic<int64_t> *left;
            Weight *weight;
        };

        class Servers {
        public:
            std::vector<ServerInfo> weight_tree;
            mutil::FlatMap<SocketId, size_t> server_map;

            Servers() {
                CHECK_EQ(0, server_map.init(1024, 70));
            }

            // Add diff to left_weight of all parent nodes of node `index'.
            // Not require position `index' to exist.
            void UpdateParentWeights(int64_t diff, size_t index) const;
        };

        static bool Add(Servers &bg, const Servers &fg,
                        SocketId id, LocalityAwareLoadBalancer *);

        static bool Remove(Servers &bg, SocketId id,
                           LocalityAwareLoadBalancer *);

        static size_t BatchAdd(Servers &bg, const Servers &fg,
                               const std::vector<SocketId> &servers,
                               LocalityAwareLoadBalancer *);

        static size_t BatchRemove(Servers &bg,
                                  const std::vector<SocketId> &servers,
                                  LocalityAwareLoadBalancer *);

        static bool RemoveAll(Servers &bg, const Servers &fg);

        // Add a entry to _left_weights.
        std::atomic<int64_t> *PushLeft() {
            _left_weights.push_back(0);
            return (std::atomic<int64_t> *) &_left_weights.back();
        }

        void PopLeft() { _left_weights.pop_back(); }

        std::atomic<int64_t> _total;
        mutil::DoublyBufferedData<Servers> _db_servers;
        std::deque<int64_t> _left_weights;
        ServerId2SocketIdMapper _id_mapper;
    };

    inline void LocalityAwareLoadBalancer::Servers::UpdateParentWeights(
            int64_t diff, size_t index) const {
        while (index != 0) {
            const size_t parent_index = (index - 1) >> 1;
            if ((parent_index << 1) + 1 == index) {  // left child
                weight_tree[parent_index].left->fetch_add(
                        diff, std::memory_order_relaxed);
            }
            index = parent_index;
        }
    }

    inline int64_t LocalityAwareLoadBalancer::Weight::ResetWeight(
            size_t index, int64_t now_us) {
        int64_t new_weight = _base_weight;
        if (_begin_time_count > 0) {
            const int64_t inflight_delay =
                    now_us - _begin_time_sum / _begin_time_count;
            const int64_t punish_latency =
                    (int64_t) (_avg_latency * FLAGS_punish_inflight_ratio);
            if (inflight_delay >= punish_latency && _avg_latency > 0) {
                new_weight = new_weight * punish_latency / inflight_delay;
            }
        }
        if (new_weight < FLAGS_min_weight) {
            new_weight = FLAGS_min_weight;
        }
        const int64_t old_weight = _weight;
        _weight = new_weight;
        const int64_t diff = new_weight - old_weight;
        if (_old_index == index && diff != 0) {
            _old_diff_sum += diff;
        }
        return diff;
    }

    inline LocalityAwareLoadBalancer::Weight::AddInflightResult
    LocalityAwareLoadBalancer::Weight::AddInflight(
            const SelectIn &in, size_t index, int64_t dice) {
        MELON_SCOPED_LOCK(_mutex);
        if (Disabled()) {
            AddInflightResult r = {false, 0};
            return r;
        }
        const int64_t diff = ResetWeight(index, in.begin_time_us);
        if (_weight < dice) {
            // inflight delay makes the weight too small to choose.
            AddInflightResult r = {false, diff};
            return r;
        }
        _begin_time_sum += in.begin_time_us;
        ++_begin_time_count;
        AddInflightResult r = {true, diff};
        return r;
    }

    inline int64_t LocalityAwareLoadBalancer::Weight::MarkFailed(
            size_t index, int64_t avg_weight) {
        MELON_SCOPED_LOCK(_mutex);
        if (_base_weight <= avg_weight) {
            return 0;
        }
        _base_weight = avg_weight;
        return ResetWeight(index, 0);
    }

} // namespace melon::lb
