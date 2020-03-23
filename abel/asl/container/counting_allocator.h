//

#ifndef ABEL_ASL_CONTAINER_COUNTING_ALLOCATOR_H_
#define ABEL_ASL_CONTAINER_COUNTING_ALLOCATOR_H_

#include <cassert>
#include <cstdint>
#include <memory>

#include <abel/base/profile.h>

namespace abel {

    namespace container_internal {

// This is a stateful allocator, but the state lives outside of the
// allocator (in whatever test is using the allocator). This is odd
// but helps in tests where the allocator is propagated into nested
// containers - that chain of allocators uses the same state and is
// thus easier to query for aggregate allocation information.
        template<typename T>
        class CountingAllocator : public std::allocator<T> {
        public:
            using Alloc = std::allocator<T>;
            using pointer = typename Alloc::pointer;
            using size_type = typename Alloc::size_type;

            CountingAllocator() : bytes_used_(nullptr) {}

            explicit CountingAllocator(int64_t *b) : bytes_used_(b) {}

            template<typename U>
            CountingAllocator(const CountingAllocator<U> &x)
                    : Alloc(x), bytes_used_(x.bytes_used_) {}

            pointer allocate(size_type n,
                             std::allocator<void>::const_pointer hint = nullptr) {
                assert(bytes_used_ != nullptr);
                *bytes_used_ += n * sizeof(T);
                return Alloc::allocate(n, hint);
            }

            void deallocate(pointer p, size_type n) {
                Alloc::deallocate(p, n);
                assert(bytes_used_ != nullptr);
                *bytes_used_ -= n * sizeof(T);
            }

            template<typename U>
            class rebind {
            public:
                using other = CountingAllocator<U>;
            };

            friend bool operator==(const CountingAllocator &a,
                                   const CountingAllocator &b) {
                return a.bytes_used_ == b.bytes_used_;
            }

            friend bool operator!=(const CountingAllocator &a,
                                   const CountingAllocator &b) {
                return !(a == b);
            }

            int64_t *bytes_used_;
        };

    }  // namespace container_internal

}  // namespace abel

#endif  // ABEL_ASL_CONTAINER_COUNTING_ALLOCATOR_H_
