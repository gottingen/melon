//
// Created by liyinbin on 2021/4/3.
//

#include "abel/memory/internal/thread_local.h"
#include "abel/functional/function.h"

namespace abel {

    namespace memory_internal {

        constexpr std::size_t kMinimumFreePerWash = 32;
        constexpr auto kMinimumWashInterval = abel::duration::milliseconds(5);

        namespace {

            std::size_t get_free_count(std::size_t upto) {
                return std::min(upto, std::max(upto / 2, kMinimumFreePerWash));
            }

            void wash_out_cache(pool_descriptor *pool) {
                auto ts = abel::time_now();
                if (ts < pool->last_wash + kMinimumWashInterval) {
                    return;  // We're called too frequently.
                } else {
                    pool->last_wash = ts;
                }

                auto &&primary = pool->primary_cache;
                auto &&secondary = pool->secondary_cache;
                auto move_to_secondary_or_free = [&](std::size_t count) {
                    while (count--) {
                        if (secondary.size() < pool->low_water_mark) {
                            secondary.push_back(std::move(primary.front()));
                        }
                        primary.pop_front();
                    }
                };

                // We've reached the high-water mark, free some objects.
                if (pool->primary_cache.size() > pool->high_water_mark) {
                    auto upto =
                            get_free_count(pool->primary_cache.size() - pool->high_water_mark);
                    move_to_secondary_or_free(upto);
                    if (upto == kMinimumFreePerWash) {
                        return;  // We've freed enough objects then.
                    }
                }

#ifndef NDEBUG
                std::size_t objects_had =
                        pool->primary_cache.size() + pool->secondary_cache.size();
#endif

                // Let's see how many objects have been idle for too long.
                auto idle_objects = std::find_if(primary.begin(), primary.end(),
                                                 [&](auto &&e) {
                                                     return ts - e.last_used < pool->max_idle;
                                                 }) -
                                    primary.begin();
                move_to_secondary_or_free(get_free_count(idle_objects));

#ifndef NDEBUG
                if (objects_had >= pool->low_water_mark) {
                    DCHECK_GE(pool->primary_cache.size() + pool->secondary_cache.size(),
                                   pool->low_water_mark);
                }
#endif
            }

        }  // namespace

        void *tls_get(const type_descriptor &desc, pool_descriptor *pool) {
            if (pool->primary_cache.empty()) {
                if (!pool->secondary_cache.empty()) {
                    pool->primary_cache = std::move(pool->secondary_cache);
                    // Reset the timestamp, otherwise they'll likely be moved to secondary
                    // cache immediately.
                    for (auto &&e : pool->primary_cache) {
                        e.last_used = abel::time_now();
                    }
                } else {
                    // We could just return the object just created instead of temporarily
                    // push it into `primary_cache`. However, since we expect the pool should
                    // satisfy most needs (i.e., this path should be seldom taken), this won't
                    // hurt much.
                    pool->primary_cache.push_back(
                            timestamped_object{.ptr = {desc.create(), desc.destroy},
                                    .last_used = abel::time_now()});
                }
            }
            auto rc = std::move(pool->primary_cache.back());
            pool->primary_cache.pop_back();
            return rc.ptr.leak();
        }

        void tls_put(const type_descriptor &desc, pool_descriptor *pool, void *ptr) {
            abel::scoped_deferred _([&] { wash_out_cache(pool); });
            pool->primary_cache.push_back(timestamped_object{
                    .ptr = {ptr, desc.destroy}, .last_used = abel::time_now()});
        }
    }  // namespace memory_internal
}  // namespace abel
