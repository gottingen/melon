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
#include <turbo/log/logging.h>                           // LOG
#include <melon/base/macros.h>
#include <melon/base/scoped_lock.h>                       // MELON_SCOPE_LOCK
#include <melon/base/doubly_buffered_data.h>   // DBD
#include <melon/utility/containers/flat_map.h>               // mutil::FlatMap
#include <melon/var/mvariable.h>

namespace melon::var {

    constexpr uint64_t MAX_MULTI_DIMENSION_STATS_COUNT = 20000;

    template<typename T>
    class MultiDimension : public MVariable {
    public:

        enum STATS_OP {
            READ_ONLY,
            READ_OR_INSERT,
        };

        typedef MVariable Base;
        typedef std::list<std::string> key_type;
        typedef T value_type;
        typedef T *value_ptr_type;

        struct KeyHash {
            size_t operator()(const key_type &key) const {
                size_t hash_value = 0;
                for (auto &k: key) {
                    hash_value += std::hash<std::string>()(k);
                }
                return hash_value;
            }
        };

        typedef value_ptr_type op_value_type;
        typedef typename mutil::FlatMap<key_type, op_value_type, KeyHash> MetricMap;

        typedef typename MetricMap::const_iterator MetricMapConstIterator;
        typedef typename mutil::DoublyBufferedData<MetricMap> MetricMapDBD;
        typedef typename MetricMapDBD::ScopedPtr MetricMapScopedPtr;

        explicit MultiDimension(const key_type &labels);

        MultiDimension(const std::string_view &name,
                       const key_type &labels);

        MultiDimension(const std::string_view &prefix,
                       const std::string_view &name,
                       const key_type &labels);

        ~MultiDimension();

        // Implement this method to print the variable into ostream.
        void describe(std::ostream &os);

        // Dump real var pointer
        size_t dump(Dumper *dumper, const DumpOptions *options);

        // Get real var pointer object
        // Return real var pointer on success, NULL otherwise.
        T *get_stats(const key_type &labels_value) {
            return get_stats_impl(labels_value, READ_OR_INSERT);
        }

        // Remove stat so those not count and dump
        void delete_stats(const key_type &labels_value);

        // Remove all stat
        void clear_stats();

        // True if var pointer exists
        bool has_stats(const key_type &labels_value);

        // Get number of stats
        size_t count_stats();

        // Put name of all stats label into `names'
        void list_stats(std::vector<key_type> *names);

#ifdef UNIT_TEST
        // Get real var pointer object
        // Return real var pointer if labels_name exist, NULL otherwise.
        // CAUTION!!! Just For Debug!!!
        T* get_stats_read_only(const key_type& labels_value) {
            return get_stats_impl(labels_value);
        }

        // Get real var pointer object
        // Return real var pointer if labels_name exist, otherwise(not exist) create var pointer.
        // CAUTION!!! Just For Debug!!!
        T* get_stats_read_or_insert(const key_type& labels_value, bool* do_write = NULL) {
            return get_stats_impl(labels_value, READ_OR_INSERT, do_write);
        }
#endif

    private:
        T *get_stats_impl(const key_type &labels_value);

        T *get_stats_impl(const key_type &labels_value, STATS_OP stats_op, bool *do_write = NULL);

        void make_dump_key(std::ostream &os,
                           const key_type &labels_value,
                           const std::string &suffix = "",
                           const int quantile = 0);

        void make_labels_kvpair_string(std::ostream &os,
                                       const key_type &labels_value,
                                       const int quantile);

        bool is_valid_lables_value(const key_type &labels_value) const;

        // Remove all stats so those not count and dump
        void delete_stats();

        static size_t init_flatmap(MetricMap &bg);

    private:
        MetricMapDBD _metric_map;
    };

} // namespace melon::var

#include <melon/var/multi_dimension_inl.h>
