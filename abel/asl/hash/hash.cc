//

#include <abel/asl/hash/hash.h>

namespace abel {

    namespace hash_internal {

        uint64_t city_hash_state::combine_large_contiguous_impl32(uint64_t state,
                                                             const unsigned char *first,
                                                             size_t len) {
            while (len >= PiecewiseChunkSize()) {
                state =
                        Mix(state, abel::hash_internal::city_hash32(reinterpret_cast<const char *>(first),
                                                                   PiecewiseChunkSize()));
                len -= PiecewiseChunkSize();
                first += PiecewiseChunkSize();
            }
            // Handle the remainder.
            return combine_contiguous_impl(state, first, len,
                                         std::integral_constant<int, 4>{});
        }

        uint64_t city_hash_state::combine_large_contiguous_impl64(uint64_t state,
                                                             const unsigned char *first,
                                                             size_t len) {
            while (len >= PiecewiseChunkSize()) {
                state =
                        Mix(state, abel::hash_internal::city_hash64(reinterpret_cast<const char *>(first),
                                                                   PiecewiseChunkSize()));
                len -= PiecewiseChunkSize();
                first += PiecewiseChunkSize();
            }
            // Handle the remainder.
            return combine_contiguous_impl(state, first, len,
                                         std::integral_constant<int, 8>{});
        }

        ABEL_CONST_INIT const void *const city_hash_state::kSeed = &kSeed;

    }  // namespace hash_internal

}  // namespace abel
