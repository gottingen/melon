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


#include <melon/var/detail/percentile.h>
#include <melon/utility/logging.h>

namespace melon::var {
    namespace detail {

        inline uint32_t ones32(uint32_t x) {
            /* 32-bit recursive reduction using SWAR...
             * but first step is mapping 2-bit values
             * into sum of 2 1-bit values in sneaky way
             */
            x -= ((x >> 1) & 0x55555555);
            x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
            x = (((x >> 4) + x) & 0x0f0f0f0f);
            x += (x >> 8);
            x += (x >> 16);
            return (x & 0x0000003f);
        }

        inline uint32_t log2(uint32_t x) {
            int y = (x & (x - 1));
            y |= -y;
            y >>= 31;
            x |= (x >> 1);
            x |= (x >> 2);
            x |= (x >> 4);
            x |= (x >> 8);
            x |= (x >> 16);
            return (ones32(x) - 1 - y);
        }

        inline size_t get_interval_index(int64_t &x) {
            if (x <= 2) {
                return 0;
            } else if (x > std::numeric_limits<uint32_t>::max()) {
                x = std::numeric_limits<uint32_t>::max();
                return 31;
            } else {
                return log2(x) - 1;
            }
        }

        class AddLatency {
        public:
            AddLatency(int64_t latency) : _latency(latency) {}

            void operator()(GlobalValue<Percentile::combiner_type> &global_value,
                            ThreadLocalPercentileSamples &local_value) const {
                // Copy to latency since get_interval_index may change input.
                int64_t latency = _latency;
                const size_t index = get_interval_index(latency);
                PercentileInterval<ThreadLocalPercentileSamples::SAMPLE_SIZE> &
                        interval = local_value.get_interval_at(index);
                if (interval.full()) {
                    GlobalPercentileSamples *g = global_value.lock();
                    g->get_interval_at(index).merge(interval);
                    g->_num_added += interval.added_count();
                    global_value.unlock();
                    local_value._num_added -= interval.added_count();
                    interval.clear();
                }
                interval.add64(latency);
                ++local_value._num_added;
            }

        private:
            int64_t _latency;
        };

        Percentile::Percentile() : _combiner(NULL), _sampler(NULL) {
            _combiner = new combiner_type;
        }

        Percentile::~Percentile() {
            // Have to destroy sampler first to avoid the race between destruction and
            // sampler
            if (_sampler != NULL) {
                _sampler->destroy();
                _sampler = NULL;
            }
            delete _combiner;
        }

        Percentile::value_type Percentile::reset() {
            return _combiner->reset_all_agents();
        }

        Percentile::value_type Percentile::get_value() const {
            return _combiner->combine_agents();
        }

        Percentile &Percentile::operator<<(int64_t latency) {
            agent_type *agent = _combiner->get_or_create_tls_agent();
            if (MELON_UNLIKELY(!agent)) {
                MLOG(FATAL) << "Fail to create agent";
                return *this;
            }
            if (latency < 0) {
                // we don't check overflow(of uint32) in percentile because the
                // overflowed value which is included in last range does not affect
                // overall distribution of other values too much.
                if (!_debug_name.empty()) {
                    MLOG(WARNING) << "Input=" << latency << " to `" << _debug_name
                                 << "' is negative, drop";
                } else {
                    MLOG(WARNING) << "Input=" << latency << " to Percentile("
                                 << (void *) this << ") is negative, drop";
                }
                return *this;
            }
            agent->merge_global(AddLatency(latency));
            return *this;
        }

    }  // namespace detail
}  // namespace melon::var
