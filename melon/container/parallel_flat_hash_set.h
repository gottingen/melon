
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_CONTAINER_PARALLEL_FLAT_HASH_SET_H_
#define MELON_CONTAINER_PARALLEL_FLAT_HASH_SET_H_

#include "melon/container/internal/raw_hash_set.h"

namespace melon {

    // -----------------------------------------------------------------------------
    // melon::parallel_flat_hash_set
    // -----------------------------------------------------------------------------
    template<class T, class Hash, class Eq, class Alloc, size_t N, class Mtx_> // default values in map_fwd_decl.h
    class parallel_flat_hash_set
            : public melon::priv::parallel_hash_set<
                    N, melon::priv::raw_hash_set, Mtx_,
                    melon::priv::flat_hash_set_policy<T>,
                    Hash, Eq, Alloc> {
        using Base = typename parallel_flat_hash_set::parallel_hash_set;

    public:
        parallel_flat_hash_set() {}

#ifdef __INTEL_COMPILER
        using Base::parallel_hash_set;
#else
        using Base::Base;
#endif
        using Base::hash;
        using Base::subidx;
        using Base::subcnt;
        using Base::begin;
        using Base::cbegin;
        using Base::cend;
        using Base::end;
        using Base::capacity;
        using Base::empty;
        using Base::max_size;
        using Base::size;
        using Base::clear;
        using Base::erase;
        using Base::insert;
        using Base::emplace;
        using Base::emplace_hint;
        using Base::emplace_with_hash;
        using Base::emplace_hint_with_hash;
        using Base::extract;
        using Base::merge;
        using Base::swap;
        using Base::rehash;
        using Base::reserve;
        using Base::contains;
        using Base::count;
        using Base::equal_range;
        using Base::find;
        using Base::bucket_count;
        using Base::load_factor;
        using Base::max_load_factor;
        using Base::get_allocator;
        using Base::hash_function;
        using Base::key_eq;
    };

    template<class T, class Hash, class Eq, class Alloc, size_t N, class Mtx_> // default values in map_fwd_decl.h
    class case_ignored_parallel_flat_hash_set
            : public melon::priv::parallel_hash_set<
                    N, melon::priv::raw_hash_set, Mtx_,
                    melon::priv::flat_hash_set_policy<T>,
                    Hash, Eq, Alloc> {
        using Base = typename case_ignored_parallel_flat_hash_set::parallel_hash_set;

    public:
        case_ignored_parallel_flat_hash_set() {}

#ifdef __INTEL_COMPILER
        using Base::parallel_hash_set;
#else
        using Base::Base;
#endif
        using Base::hash;
        using Base::subidx;
        using Base::subcnt;
        using Base::begin;
        using Base::cbegin;
        using Base::cend;
        using Base::end;
        using Base::capacity;
        using Base::empty;
        using Base::max_size;
        using Base::size;
        using Base::clear;
        using Base::erase;
        using Base::insert;
        using Base::emplace;
        using Base::emplace_hint;
        using Base::emplace_with_hash;
        using Base::emplace_hint_with_hash;
        using Base::extract;
        using Base::merge;
        using Base::swap;
        using Base::rehash;
        using Base::reserve;
        using Base::contains;
        using Base::count;
        using Base::equal_range;
        using Base::find;
        using Base::bucket_count;
        using Base::load_factor;
        using Base::max_load_factor;
        using Base::get_allocator;
        using Base::hash_function;
        using Base::key_eq;
    };

}  // namespace melon

#endif  // MELON_CONTAINER_PARALLEL_FLAT_HASH_SET_H_
