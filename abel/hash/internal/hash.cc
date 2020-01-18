//

#include <abel/hash/internal/hash.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace hash_internal {

uint64_t CityHashState::CombineLargeContiguousImpl32(uint64_t state,
                                                     const unsigned char* first,
                                                     size_t len) {
  while (len >= PiecewiseChunkSize()) {
    state =
        Mix(state, abel::hash_internal::CityHash32(reinterpret_cast<const char*>(first),
                                         PiecewiseChunkSize()));
    len -= PiecewiseChunkSize();
    first += PiecewiseChunkSize();
  }
  // Handle the remainder.
  return CombineContiguousImpl(state, first, len,
                               std::integral_constant<int, 4>{});
}

uint64_t CityHashState::CombineLargeContiguousImpl64(uint64_t state,
                                                     const unsigned char* first,
                                                     size_t len) {
  while (len >= PiecewiseChunkSize()) {
    state =
        Mix(state, abel::hash_internal::CityHash64(reinterpret_cast<const char*>(first),
                                         PiecewiseChunkSize()));
    len -= PiecewiseChunkSize();
    first += PiecewiseChunkSize();
  }
  // Handle the remainder.
  return CombineContiguousImpl(state, first, len,
                               std::integral_constant<int, 8>{});
}

ABEL_CONST_INIT const void* const CityHashState::kSeed = &kSeed;

}  // namespace hash_internal
ABEL_NAMESPACE_END
}  // namespace abel
