// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#include "abel/hash/internal/hash.h"

namespace abel {

    namespace hash_internal {

        uint64_t city_hash_state::combine_large_contiguous_impl32(uint64_t state,
                                                                  const unsigned char *first,
                                                                  size_t len) {
            while (len >= piecewise_chunk_size()) {
                state =
                        read_mix(state, abel::hash_internal::city_hash32(reinterpret_cast<const char *>(first),
                                                                         piecewise_chunk_size()));
                len -= piecewise_chunk_size();
                first += piecewise_chunk_size();
            }
            // Handle the remainder.
            return combine_contiguous_impl(state, first, len,
                                           std::integral_constant<int, 4>{});
        }

        uint64_t city_hash_state::combine_large_contiguous_impl64(uint64_t state,
                                                                  const unsigned char *first,
                                                                  size_t len) {
            while (len >= piecewise_chunk_size()) {
                state =
                        read_mix(state, abel::hash_internal::city_hash64(reinterpret_cast<const char *>(first),
                                                                         piecewise_chunk_size()));
                len -= piecewise_chunk_size();
                first += piecewise_chunk_size();
            }
            // Handle the remainder.
            return combine_contiguous_impl(state, first, len,
                                           std::integral_constant<int, 8>{});
        }

        ABEL_CONST_INIT const void *const city_hash_state::kSeed = &kSeed;

    }  // namespace hash_internal

}  // namespace abel
