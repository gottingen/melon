//
// Provides the internal API for hashtable_debug.h.

#ifndef ABEL_ASL_CONTAINER_HASHTABLE_DEBUG_HOOKS_H_
#define ABEL_ASL_CONTAINER_HASHTABLE_DEBUG_HOOKS_H_

#include <cstddef>

#include <algorithm>
#include <type_traits>
#include <vector>

#include <abel/base/profile.h>

namespace abel {

    namespace container_internal {
        namespace hashtable_debug_internal {

// If it is a map, call get<0>().
            using std::get;

            template<typename T, typename = typename T::mapped_type>
            auto GetKey(const typename T::value_type &pair, int) -> decltype(get<0>(pair)) {
                return get<0>(pair);
            }

// If it is not a map, return the value directly.
            template<typename T>
            const typename T::key_type &GetKey(const typename T::key_type &key, char) {
                return key;
            }

// Containers should specialize this to provide debug information for that
// container.
            template<class Container, typename Enabler = void>
            struct HashtableDebugAccess {
                // Returns the number of probes required to find `key` in `c`.  The "number of
                // probes" is a concept that can vary by container.  Implementations should
                // return 0 when `key` was found in the minimum number of operations and
                // should increment the result for each non-trivial operation required to find
                // `key`.
                //
                // The default implementation uses the bucket api from the standard and thus
                // works for `std::unordered_*` containers.
                static size_t GetNumProbes(const Container &c,
                                           const typename Container::key_type &key) {
                    if (!c.bucket_count()) return {};
                    size_t num_probes = 0;
                    size_t bucket = c.bucket(key);
                    for (auto it = c.begin(bucket), e = c.end(bucket);; ++it, ++num_probes) {
                        if (it == e) return num_probes;
                        if (c.key_eq()(key, GetKey<Container>(*it, 0))) return num_probes;
                    }
                }

                // Returns the number of bytes requested from the allocator by the container
                // and not freed.
                //
                // static size_t AllocatedByteSize(const Container& c);

                // Returns a tight lower bound for AllocatedByteSize(c) where `c` is of type
                // `Container` and `c.size()` is equal to `num_elements`.
                //
                // static size_t LowerBoundAllocatedByteSize(size_t num_elements);
            };

        }  // namespace hashtable_debug_internal
    }  // namespace container_internal

}  // namespace abel

#endif  // ABEL_ASL_CONTAINER_HASHTABLE_DEBUG_HOOKS_H_
