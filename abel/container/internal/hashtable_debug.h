// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// This library provides APIs to debug the probing behavior of hash tables.
//
// In general, the probing behavior is a black box for users and only the
// side effects can be measured in the form of performance differences.
// These APIs give a glimpse on the actual behavior of the probing algorithms in
// these hashtables given a specified hash function and a set of elements.
//
// The probe count distribution can be used to assess the quality of the hash
// function for that particular hash table. Note that a hash function that
// performs well in one hash table implementation does not necessarily performs
// well in a different one.
//
// This library supports std::unordered_{set,map}, dense_hash_{set,map} and
// abel::{flat,node,string}_hash_{set,map}.

#ifndef ABEL_CONTAINER_INTERNAL_HASHTABLE_DEBUG_H_
#define ABEL_CONTAINER_INTERNAL_HASHTABLE_DEBUG_H_

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <vector>

#include "abel/container/internal/hashtable_debug_hooks.h"

namespace abel {

namespace container_internal {

// Returns the number of probes required to lookup `key`.  Returns 0 for a
// search with no collisions.  Higher values mean more hash collisions occurred;
// however, the exact meaning of this number varies according to the container
// type.
template<typename C>
size_t get_hashtable_debug_num_probes(
        const C &c, const typename C::key_type &key) {
    return abel::container_internal::hashtable_debug_internal::
    hashtable_debug_access<C>::get_num_probes(c, key);
}

// Gets a histogram of the number of probes for each elements in the container.
// The sum of all the values in the vector is equal to container.size().
template<typename C>
std::vector<size_t> get_hashtable_debug_num_probes_histogram(const C &container) {
    std::vector<size_t> v;
    for (auto it = container.begin(); it != container.end(); ++it) {
        size_t num_probes = get_hashtable_debug_num_probes(
                container,
                abel::container_internal::hashtable_debug_internal::get_key<C>(*it, 0));
        v.resize((std::max)(v.size(), num_probes + 1));
        v[num_probes]++;
    }
    return v;
}

struct hashtable_debug_probe_summary {
    size_t total_elements;
    size_t total_num_probes;
    double mean;
};

// Gets a summary of the probe count distribution for the elements in the
// container.
template<typename C>
hashtable_debug_probe_summary GetHashtableDebugProbeSummary(const C &container) {
    auto probes = get_hashtable_debug_num_probes_histogram(container);
    hashtable_debug_probe_summary summary = {};
    for (size_t i = 0; i < probes.size(); ++i) {
        summary.total_elements += probes[i];
        summary.total_num_probes += probes[i] * i;
    }
    summary.mean = 1.0 * summary.total_num_probes / summary.total_elements;
    return summary;
}

// Returns the number of bytes requested from the allocator by the container
// and not freed.
template<typename C>
size_t allocated_byte_size(const C &c) {
    return abel::container_internal::hashtable_debug_internal::
    hashtable_debug_access<C>::allocated_byte_size(c);
}

// Returns a tight lower bound for allocated_byte_size(c) where `c` is of type `C`
// and `c.size()` is equal to `num_elements`.
template<typename C>
size_t lower_bound_allocated_byte_size(size_t num_elements) {
    return abel::container_internal::hashtable_debug_internal::
    hashtable_debug_access<C>::lower_bound_allocated_byte_size(num_elements);
}

}  // namespace container_internal

}  // namespace abel

#endif  // ABEL_CONTAINER_INTERNAL_HASHTABLE_DEBUG_H_
