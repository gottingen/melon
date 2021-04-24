//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_THREAD_LOCAL_H_
#define ABEL_MEMORY_THREAD_LOCAL_H_

#include <deque>

#include "abel/chrono/clock.h"
#include "abel/memory/erased_ptr.h"
#include "abel/memory/internal/type_descriptor.h"
#include "abel/debugging/class_name.h"
#include "abel/log/logging.h"

namespace abel {
    namespace memory_internal {


        struct timestamped_object {
            abel::erased_ptr ptr;
            abel::abel_time last_used;
        };

        struct pool_descriptor {
            const std::size_t low_water_mark;
            const std::size_t high_water_mark;
            const abel::duration max_idle;
            abel_time last_wash{abel::now()};

            // Objects in primary cache is washed out to `secondary_cache` if there's
            // still room.
            std::deque<timestamped_object> primary_cache;

            // Objects here are not subject to washing out.
            std::deque<timestamped_object> secondary_cache;
        };

        // `template <class T> inline thread_local pool_descriptor pool` does not work:
        //
        // > redefinition of 'bool __tls_guard'
        //
        // I think it's a bug in GCC.
        template<class T>
        pool_descriptor *get_thread_local_pool() {
            static_assert(pool_traits<T>::kHighWaterMark > pool_traits<T>::kLowWaterMark,
                          "You should leave some room between the water marks.");

            // Internally we always keep `kLowWaterMark` objects in secondary cache,
            // so the "effective" high water mark should subtract `kLowWaterMark`.
            constexpr auto kEffectiveHighWaterMark =
                    pool_traits<T>::kHighWaterMark - pool_traits<T>::kLowWaterMark;

            thread_local pool_descriptor pool = {
                    .low_water_mark = pool_traits<T>::kLowWaterMark,
                    .high_water_mark = kEffectiveHighWaterMark,
                    .max_idle = pool_traits<T>::kMaxIdle};
            DCHECK_MSG((pool.low_water_mark == pool_traits<T>::kLowWaterMark) &&
                       (pool.high_water_mark == kEffectiveHighWaterMark) &&
                       (pool.max_idle == pool_traits<T>::kMaxIdle),
                       "You likely had an ODR-violation when customizing type [{}].",
                       abel::get_type_name<T>());
            return &pool;
        }

        void *tls_get(const type_descriptor &desc, pool_descriptor *pool);

        void tls_put(const type_descriptor &desc, pool_descriptor *pool, void *ptr);

    }  // namespace memory_internal

}  // namespace abel

#endif  // ABEL_MEMORY_THREAD_LOCAL_H_
