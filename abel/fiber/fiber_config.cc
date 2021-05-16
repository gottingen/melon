//
// Created by liyinbin on 2021/4/17.
//

#include "abel/fiber/fiber_config.h"

namespace abel {

    fiber_config &fiber_config::set_worker_num(uint32_t n) {
        workers_per_group = n;
        return *this;
    }

    fiber_config &fiber_config::set_policy(std::shared_ptr<abel::core_affinity::affinity_policy> other) {
        policy = other;
        return *this;
    }

    fiber_config fiber_config::all_cores() {
        fiber_config cfg;
        cfg.workers_per_group = core_affinity::num_logical_cores() / 2;
        cfg.scheduling_groups = 2;
        cfg.policy = abel::core_affinity::affinity_policy::any_of(abel::core_affinity::all());
        return cfg;
    }

    namespace fiber_internal {
        constexpr auto kMaximumSchedulingGroupSize = 64;

        fiber_config make_config_for_compute_heavy(std::size_t concurrency) {
            auto groups = (concurrency + kMaximumSchedulingGroupSize - 1) /
                          kMaximumSchedulingGroupSize;
            auto group_size = (concurrency + groups - 1) / groups;
            return fiber_config{.scheduling_groups = groups,
                    .workers_per_group = group_size,
                    .enable_numa_aware = false};
        }

        fiber_config make_config_for_compute(
                std::size_t numa_domains, std::size_t available_processors,
                std::size_t desired_concurrency) {
            auto numa_aware =
                    numa_domains > 1 && desired_concurrency * 2 >= available_processors;
            if (!numa_aware) {
                return make_config_for_compute_heavy(desired_concurrency);
            }
            auto per_node = (desired_concurrency + numa_domains - 1) / numa_domains;
            auto group_per_node = (per_node + kMaximumSchedulingGroupSize - 1) /
                                  kMaximumSchedulingGroupSize;
            auto group_size = (per_node + group_per_node - 1) / group_per_node;
            return fiber_config{
                    .scheduling_groups = group_per_node * numa_domains,
                    .workers_per_group = group_size,
                    .enable_numa_aware = true};
        }

        fiber_config make_config_by_param(
                std::size_t numa_domains, std::size_t concurrency,
                std::size_t group_size_low, std::size_t group_size_high) {
            if (concurrency <= group_size_low) {
                return fiber_config{.scheduling_groups = 1,
                        .workers_per_group = concurrency,
                        .enable_numa_aware = false};
            }

            bool numa_aware = true;
            std::size_t best_group_size_so_far = 0;
            std::size_t best_group_extra_workers = 0x7fff'ffff;

            // Try respect NUMA topology first.
            if (numa_domains > 1) {
                for (size_t i = group_size_low; i != group_size_high; ++i) {
                    auto groups = (concurrency + i - 1) / i;
                    auto extra = groups * i - concurrency;
                    if (groups % numa_domains != 0) {
                        continue;
                    }
                    if (extra < best_group_extra_workers) {
                        best_group_extra_workers = extra;
                        best_group_size_so_far = i;
                    }
                }
            }

            // If we can't find a suitable configuration w.r.t NUMA topology, retry with
            // UMA configuration.
            if (!best_group_size_so_far || best_group_extra_workers > concurrency / 10) {
                numa_aware = false;
                best_group_extra_workers = 0x7fff'ffff;
                for (size_t i = group_size_low; i != group_size_high; ++i) {
                    auto groups = (concurrency + i - 1) / i;
                    auto extra = groups * i - concurrency;
                    if (extra < best_group_extra_workers) {
                        best_group_extra_workers = extra;
                        best_group_size_so_far = i;
                    }
                }
            }

            return fiber_config{
                    .scheduling_groups =
                    (concurrency + best_group_size_so_far - 1) / best_group_size_so_far,
                    .workers_per_group = best_group_size_so_far,
                    .enable_numa_aware = numa_aware};
        }
    }  // namespace fiber_internal
    fiber_config fiber_config::make_fiber_conf(schedule_type profile,
                                               std::size_t numa_domains,
                                               std::size_t available_processors,
                                               std::size_t desired_concurrency) {
        if (profile == schedule_type::eComputeHeavy) {
            return fiber_internal::make_config_for_compute_heavy(desired_concurrency);
        } else if (profile == schedule_type::eCompute) {
            return fiber_internal::make_config_for_compute(numa_domains, available_processors,
                                                           desired_concurrency);
        } else if (profile == schedule_type::eNormal) {
            return fiber_internal::make_config_by_param(numa_domains, desired_concurrency,
                                                        16, 32);
        } else if (profile == schedule_type::eIo) {
            return fiber_internal::make_config_by_param(numa_domains, desired_concurrency,
                                                        12, 24);
        } else if (profile == schedule_type::eIoHeavy) {
            return fiber_internal::make_config_by_param(numa_domains, desired_concurrency,
                                                        8, 16);
        }
        return fiber_config();
    }

    fiber_config g_fiber_conf;

    fiber_config &fiber_config::get_global_fiber_config() {
        return g_fiber_conf;
    }
}  // namespace abel