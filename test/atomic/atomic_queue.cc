//
// Created by liyinbin on 2020/3/10.
//

#include <abel/atomic/atomic_queue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/base/profile.h>

namespace abel {

    struct tracking_allocator
    {
        union tag {
            std::size_t size;
#ifdef __GNUC__
            max_align_t dummy;		// GCC forgot to add it to std:: for a while
#else
            std::max_align_t dummy;	// Others (e.g. MSVC) insist it can *only* be accessed via std::
#endif
        };

        static inline void* malloc(std::size_t size)
        {
            auto ptr = std::malloc(size + sizeof(tag));
            if (ptr) {
                reinterpret_cast<tag*>(ptr)->size = size;
                usage.fetch_add(size, std::memory_order_relaxed);
                return reinterpret_cast<char*>(ptr) + sizeof(tag);
            }
            return nullptr;
        }

        static inline void free(void* ptr)
        {
            if (ptr) {
                ptr = reinterpret_cast<char*>(ptr) - sizeof(tag);
                auto size = reinterpret_cast<tag*>(ptr)->size;
                usage.fetch_add(-size, std::memory_order_relaxed);
            }
            std::free(ptr);
        }

        static inline std::size_t current_usage() { return usage.load(std::memory_order_relaxed); }

    private:
        static std::atomic<std::size_t> usage;
    };

    std::atomic<std::size_t> tracking_allocator::usage(0);

    struct MallocTrackingTraits : public ConcurrentQueueDefaultTraits
    {
        static inline void* malloc(std::size_t size) { return tracking_allocator::malloc(size); }
        static inline void free(void* ptr) { tracking_allocator::free(ptr); }
    };

TEST(atomic_queue, ctor) {
    abel:: ConcurrentQueue<int, MallocTrackingTraits> q;
    EXPECT_EQ(q.size_approx(), 0);
}

} //namespace abel
