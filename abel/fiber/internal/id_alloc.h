//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_ID_ALLOC_H
#define ABEL_ID_ALLOC_H


#include <atomic>

#include "abel/base/annotation.h"
#include "abel/base/profile.h"

namespace abel {
    namespace fiber_internal {

        template<class Traits>
        class alloc_impl;

        // `Traits` is defined as:
        //
        // struct Traits {
        //   // (Integral) type of ID.
        //   using Type = ...;
        //
        //   // Minimum / maximum value of ID. The interval is left-closed, right-open.
        //   // Technically this means we can waste one ID if the entire value range of
        //   // `type` can be used, however, wasting it simplifies our implementation,
        //   // and shouldn't hurt much anyway.
        //   static constexpr auto kMin = ...;
        //   static constexpr auto kMax = ...;
        //
        //   // For better performance, the implementation grabs a batch of IDs from
        //   // global counter to its thread local cache. This parameter controls batch
        //   // size.
        //   static constexpr auto kBatchSize = ...;
        // };
        //
        // This method differs from `index_alloc` in that `index_alloc` tries its best to
        // reuse indices, in trade of performance. In contrast, this method does not
        // try to reuse ID, for better performance.
        template<class Traits>
        inline auto next_id() {
            return alloc_impl<Traits>::next();
        }

        template<class Traits>
        class alloc_impl {
        public:
            using Type = typename Traits::Type;

            // This method will likely be inlined.
            static Type next() {
                // Let's see if our thread local cache can serve us.
                if (auto v = current++; ABEL_LIKELY(v < max)) {
                    return v;
                }

                return slow_next();
            }

        private:
            static Type slow_next();

        private:
            static inline std::atomic<Type> global{Traits::kMin};
            ABEL_INTERNAL_TLS_MODEL static inline thread_local Type
            current = 0;
            ABEL_INTERNAL_TLS_MODEL static inline thread_local Type
            max = 0;
        };

        template<class Traits>
        typename alloc_impl<Traits>::Type alloc_impl<Traits>::slow_next() {
            // Get more IDs from global counter...
            Type read, nt;
            do {
                read = global.load(std::memory_order_relaxed);
                nt = read + Traits::kBatchSize;
                current = read;
                max = nt;
                if (nt >= Traits::kMax) {
                    max = Traits::kMax;
                    nt = Traits::kMin;
                }
            } while (
                    !global.compare_exchange_weak(read, nt, std::memory_order_relaxed));

            // ... and retry.
            return next();
        }
    }  // namespace fiber_internal
}  // namespace abel


#endif //ABEL_ID_ALLOC_H
