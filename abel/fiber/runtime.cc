//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/runtime.h"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "abel/base/annotation.h"
#include "abel/base/random.h"
#include "abel/thread/numa.h"
#include "abel/fiber/internal/fiber_worker.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/internal/scheduling_parameters.h"
#include "abel/fiber/internal/timer_worker.h"
#include "abel/strings/case_conv.h"
#include "abel/fiber/fiber_config.h"

/*

DEFINE_int32(
        concurrency_hint, 0,
        "Hint to how many threads should be started to run fibers. abel may "
        "adjust this value when it deems fit. The default is `nproc()`.");
DEFINE_int32(scheduling_group_size, 16,
             "Internally abel divides worker threads into groups, and tries "
             "to avoid sharing between them. This option controls group size "
             "of workers. Setting it too small may result in unbalanced "
             "workload, setting it too large can hurt overall scalability.");
DEFINE_bool(numa_aware, false,
            "If set, abel allocates (binds) worker threads (in group) to CPU "
            "nodes. Otherwise it's up to OS's scheduler to determine which "
            "worker thread should run on which CPU (/node).");
DEFINE_string(fiber_worker_accessible_cpus, "",
              "If set, fiber workers only use CPUs given. CPUs are specified "
              "in range or CPU IDs, e.g.: 0-10,11,12. Negative CPU IDs can be "
              "used to specify CPU IDs in opposite order (e.g., -1 means the "
              "last CPU.). Negative IDs can only be specified individually due "
              "to difficulty in parse. This option may not be used in "
              "conjuction with `fiber_worker_inaccessible_cpus`.");
DEFINE_string(
        fiber_worker_inaccessible_cpus, "",
        "If set, fiber workers use CPUs that are NOT listed here. Both CPU ID "
        "ranges or individual IDs are recognized. This option may not be used in "
        "conjuction with `fiber_worker_accessible_cpus`. CPUs that are not "
        "accessible to us due to thread affinity or other resource contraints are "
        "also respected when this option is used, you don't have to (yet, not "
        "prohibited to) specify them in the list. ");
DEFINE_bool(
        fiber_worker_disallow_cpu_migration, false,
        "If set, each fiber worker is bound to exactly one CPU core, and each core "
        "is dedicated to exactly one fiber worker. `concurrency_hint` (if "
        "set) must be equal to the number of CPUs in the system (or in case "
        "`fiber_worker_accessible_cpus` is set as well, the number of CPUs "
        "accessible to fiber worker.). Incorrect use of this option can actually "
        "hurt performance.");
DEFINE_int32(work_stealing_ratio, 16,
             "Reciprocal of ratio for stealing job from other scheduling "
             "groups in same NUMA domain. Note that if multiple \"foreign\" "
             "scheduling groups present, the actual work stealing ratio is "
             "multiplied by foreign scheduling group count.");
DEFINE_string(
        fiber_scheduling_optimize_for, "neutral",
        "This option controls for which use case should fiber scheduling parameter "
        "optimized for. The valid choices are 'compute-heavy', 'compute', "
        "'neutral', 'io', 'io-heavy', 'customized'. Optimize for computation if "
        "your workloads tend to run long (without triggering fiber scheduling), "
        "optimize for I/O if your workloads run quickly or triggers fiber "
        "scheduling often. If none of the predefine optimization profile suits "
        "your needs, use `customized` and specify "
        "`scheduling_parameters.workers_per_group` "
        "and `numa_aware` with your own choice.");

// In our test, cross-NUMA work stealing hurts performance.
//
// The performance hurt comes from multiple aspects, notably the imbalance in
// shared pool of `MemoryNodeShared` object pool.
//
// Therefore, by default, we disables cross-NUMA work stealing completely.
DEFINE_int32(cross_numa_work_stealing_ratio, 0,
             "Same as `work_stealing_ratio`, but applied to stealing "
             "work from scheduling groups belonging to different NUMA domain. "
             "Setting it to 0 disables stealing job cross NUMA domain. Blindly "
             "enabling this options can actually hurt performance. You should "
             "do thorough test before changing this option.");
*/
namespace abel {

    namespace {

        // `scheduling_group` and its workers (both fiber worker and timer worker).
        struct scheduling_worker {
            int node_id;
            std::unique_ptr<fiber_internal::scheduling_group> scheduling_group;
            std::vector<std::unique_ptr<fiber_internal::fiber_worker>> fiber_workers;
            std::unique_ptr<fiber_internal::timer_worker> timer_worker;

            void Start(bool no_cpu_migration) {
                timer_worker->start();
                for (auto &&e : fiber_workers) {
                    e->start(no_cpu_migration);
                }
            }

            void Stop() {
                timer_worker->stop();
                scheduling_group->stop();
            }

            void Join() {
                timer_worker->join();
                for (auto &&e : fiber_workers) {
                    e->join();
                }
            }
        };

        // Final decision of scheduling parameters.
        std::size_t fiber_concurrency_in_effect = 0;
        fiber_internal::SchedulingParameters scheduling_parameters;

        // Index by node ID. i.e., `scheduling_group[node][sg_index]`
        //
        // If `numa_aware` is not set, `node` should always be 0.
        //
        // 64 nodes should be enough.
        std::vector<std::unique_ptr<scheduling_worker>> scheduling_groups[64];

        // This vector holds pointer to scheduling groups in `scheduling_groups`. It's
        // primarily used for randomly choosing a scheduling group or finding scheduling
        // group by ID.
        std::vector<scheduling_worker *> flatten_scheduling_groups;

        const std::vector<int> &GetFiberWorkerAccessibleCPUs();

        const std::vector<numa::numa_node> &GetFiberWorkerAccessibleNodes();

        std::uint64_t DivideRoundUp(std::uint64_t divisor, std::uint64_t dividend) {
            return divisor / dividend + (divisor % dividend != 0);
        }

        // Call `f` in a thread with the specified affinity.
        //
        // This method helps you allocates resources from memory attached to one of the
        // CPUs listed in `affinity`, instead of the calling node (unless they're the
        // same).
        template<class F>
        void ExecuteWithAffinity(const std::vector<int> &affinity, F &&f) {
            // Dirty but works.
            //
            // TODO(yinbinli): Set & restore this thread's affinity to `affinity` (instead
            // of starting a new thread) to accomplish this.
            std::thread([&] {
                SetCurrentThreadAffinity(affinity);
                std::forward<F>(f)();
            }).join();
        }

        std::unique_ptr<scheduling_worker> CreateFullyFledgedSchedulingGroup(
                int node_id, const std::vector<int> &affinity, std::size_t size) {
            if (numa::support_affinity()) {
                DCHECK(!fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration ||
                       affinity.size() == size);
            }

            // TODO(yinbinli): Create these objects in a thread with affinity `affinity.
            auto rc = std::make_unique<scheduling_worker>();
            core_affinity aff = core_affinity::group_cores(node_id, affinity);
            rc->node_id = node_id;
            rc->scheduling_group =
                    std::make_unique<fiber_internal::scheduling_group>(aff, size);
            for (size_t i = 0; i != size; ++i) {
                rc->fiber_workers.push_back(
                        std::make_unique<fiber_internal::fiber_worker>(rc->scheduling_group.get(), i));
            }
            rc->timer_worker =
                    std::make_unique<fiber_internal::timer_worker>(rc->scheduling_group.get());
            rc->scheduling_group->set_timer_worker(rc->timer_worker.get());
            return rc;
        }

        // Add all scheduling groups in `victims` to fiber workers in `thieves`.
        //
        // Even if scheduling the thief is inside presents in `victims`, it won't be
        // added to the corresponding thief.
        void initialize_foreign_scheduling_groups(
                const std::vector<std::unique_ptr<scheduling_worker>> &thieves,
                const std::vector<std::unique_ptr<scheduling_worker>> &victims,
                std::uint64_t steal_every_n) {
            ABEL_UNUSED(steal_every_n);
            for (std::size_t thief = 0; thief != thieves.size(); ++thief) {
                for (std::size_t victim = 0; victim != victims.size(); ++victim) {
                    if (thieves[thief]->scheduling_group ==
                        victims[victim]->scheduling_group) {
                        return;
                    }
                }
            }
        }

        void StartWorkersUma() {
            DLOG_INFO(
                    "Starting {} worker threads per group, for a total of {} groups. The "
                    "system is treated as UMA.",
                    scheduling_parameters.workers_per_group,
                    scheduling_parameters.scheduling_groups);
            DLOG_IF_WARN(
                    fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration &&
                    GetFiberWorkerAccessibleNodes().size() > 1,
                    "CPU migration of fiber worker is disallowed, and we're trying to start "
                    "in UMA way on NUMA architecture. Performance will likely degrade.");

            for (std::size_t index = 0; index != scheduling_parameters.scheduling_groups;
                 ++index) {
                if (!fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration) {
                    scheduling_groups[0].push_back(CreateFullyFledgedSchedulingGroup(
                            0 /* Not sigfinicant */, GetFiberWorkerAccessibleCPUs(),
                            scheduling_parameters.workers_per_group));
                } else {
                    // Each group of processors is dedicated to a scheduling group.
                    //
                    // Later when we start the fiber workers, we'll instruct them to set their
                    // affinity to their dedicated processors.
                    auto &&cpus = GetFiberWorkerAccessibleCPUs();
                    DCHECK_LE((index + 1) * scheduling_parameters.workers_per_group,
                              cpus.size());
                    scheduling_groups[0].push_back(CreateFullyFledgedSchedulingGroup(
                            0,
                            {cpus.begin() + index * scheduling_parameters.workers_per_group,
                             cpus.begin() +
                             (index + 1) * scheduling_parameters.workers_per_group},
                            scheduling_parameters.workers_per_group));
                }
            }

            initialize_foreign_scheduling_groups(scheduling_groups[0], scheduling_groups[0],
                                                 fiber_config::get_global_fiber_config().work_stealing_ratio);
        }

        void StartWorkersNuma() {
            auto topo = GetFiberWorkerAccessibleNodes();
            DCHECK_MSG(topo.size() < std::size(scheduling_groups),
                       "Far more nodes that abel can support present on this "
                       "machine. Bail out.");

            auto groups_per_node = scheduling_parameters.scheduling_groups / topo.size();
            DLOG_INFO(
                    "Starting {} worker threads per group, {} group per node, for a total of "
                    "{} nodes.",
                    scheduling_parameters.workers_per_group, groups_per_node, topo.size());

            for (size_t i = 0; i != topo.size(); ++i) {
                for (size_t j = 0; j != groups_per_node; ++j) {
                    if (!fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration) {
                        auto &&affinity = topo[i].logical_cpus;
                        ExecuteWithAffinity(affinity, [&] {
                            scheduling_groups[i].push_back(CreateFullyFledgedSchedulingGroup(
                                    i, affinity, scheduling_parameters.workers_per_group));
                        });
                    } else {
                        // Same as `StartWorkersUma()`, fiber worker's affinity is set upon
                        // start.
                        auto &&cpus = topo[i].logical_cpus;
                        DCHECK_LE((j + 1) * groups_per_node, cpus.size());
                        std::vector<int> affinity = {
                                cpus.begin() + j * scheduling_parameters.workers_per_group,
                                cpus.begin() + (j + 1) * scheduling_parameters.workers_per_group};
                        ExecuteWithAffinity(affinity, [&] {
                            scheduling_groups[i].push_back(CreateFullyFledgedSchedulingGroup(
                                    i, affinity, scheduling_parameters.workers_per_group));
                        });
                    }
                }
            }

            for (size_t i = 0; i != topo.size(); ++i) {
                for (size_t j = 0; j != topo.size(); ++j) {
                    if (fiber_config::get_global_fiber_config().cross_numa_work_stealing_ratio == 0 && i != j) {
                        // Different NUMA domain.
                        //
                        // `enable_cross_numa_work_stealing` is not enabled, so we skip
                        // this pair.
                        continue;
                    }
                    initialize_foreign_scheduling_groups(
                            scheduling_groups[i], scheduling_groups[j],
                            i == j ? fiber_config::get_global_fiber_config().work_stealing_ratio
                                   : fiber_config::get_global_fiber_config().cross_numa_work_stealing_ratio);
                }
            }
        }

        std::vector<int> GetFiberWorkerAccessibleCPUsImpl() {
            DCHECK_MSG(fiber_config::get_global_fiber_config().fiber_worker_accessible_cpus.empty() ||
                               fiber_config::get_global_fiber_config().fiber_worker_inaccessible_cpus.empty(),
                       "At most one of `fiber_worker_accessible_cpus` or "
                       "`fiber_worker_inaccessible_cpus` may be specified.");
            if (!core_affinity::supported) {
                return std::vector<int>();
            }
            // If user specified accessible CPUs explicitly.
            if (!fiber_config::get_global_fiber_config().fiber_worker_accessible_cpus.empty()) {
                auto procs = TryParseProcesserList(
                        fiber_config::get_global_fiber_config().fiber_worker_accessible_cpus);
                DCHECK_MSG(procs, "Failed to parse `fiber_worker_accessible_cpus`.");
                return *procs;
            }

            // If affinity is set on the process, show our respect.
            //
            // Note that we don't have to do some dirty trick to check if processors we
            // get from affinity is accessible to us -- Inaccessible CPUs shouldn't be
            // returned to us in the first place.
            auto accessible_cpus = GetCurrentThreadAffinity();
            DCHECK(!accessible_cpus.empty());

            // If user specified inaccessible CPUs explicitly.
            if (!fiber_config::get_global_fiber_config().fiber_worker_inaccessible_cpus.empty()) {
                auto option = TryParseProcesserList(
                        fiber_config::get_global_fiber_config().fiber_worker_inaccessible_cpus);
                DCHECK_MSG(option,
                           "Failed to parse `fiber_worker_inaccessible_cpus`.");
                std::set<int> inaccessible(option->begin(), option->end());

                // Drop processors we're forbidden to access.
                accessible_cpus.erase(
                        std::remove_if(accessible_cpus.begin(), accessible_cpus.end(),
                                       [&](auto &&x) { return inaccessible.count(x) != 0; }),
                        accessible_cpus.end());
                return accessible_cpus;
            }

            // Thread affinity is respected implicitly.
            return accessible_cpus;
        }

        const std::vector<int> &GetFiberWorkerAccessibleCPUs() {
            static auto result = GetFiberWorkerAccessibleCPUsImpl();
            return result;
        }

        const std::vector<numa::numa_node> &GetFiberWorkerAccessibleNodes() {
            static std::vector<numa::numa_node> result =
#ifdef ABEL_PLATFORM_LINUX
             [] {
                std::map<int, std::vector<int>> node_to_processor;
                for (auto&& e : GetFiberWorkerAccessibleCPUs()) {
                    auto n = numa::get_node_of_processor(e);
                    node_to_processor[n].push_back(e);
                }

                std::vector<numa::numa_node> result;
                for (auto&& [k, v] : node_to_processor) {
                    result.push_back({k, v});
                }
                return result;
            }();
#else
            [] {
                return std::vector<numa::numa_node>();
            }();

#endif
            return result;
        }


        void DisallowProcessorMigrationPreconditionCheck() {
            auto expected_concurrency =
                    DivideRoundUp(fiber_concurrency_in_effect,
                                  scheduling_parameters.workers_per_group) *
                    scheduling_parameters.workers_per_group;
            DLOG_IF_CRITICAL(
                    expected_concurrency > GetFiberWorkerAccessibleCPUs().size() &&
                            fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration,
                    "CPU migration of fiber workers is explicitly disallowed, but there "
                    "isn't enough CPU to dedicate one for each fiber worker. {} CPUs got, at "
                    "least {} CPUs required.",
                    GetFiberWorkerAccessibleCPUs().size(), expected_concurrency);
        }

    }  // namespace

    namespace fiber_internal {

        std::size_t get_current_scheduling_groupIndex_slow() {
            auto rc = fiber_internal::nearest_scheduling_group_index();
            DCHECK_MSG(rc != -1,
                       "Calling `get_current_scheduling_group_index` outside of any "
                       "scheduling group is undefined.");
            return rc;
        }

        std::optional<SchedulingProfile> GetSchedulingProfile() {

            if (fiber_config::get_global_fiber_config().fiber_scheduling_optimize_for != "customized") {

                DLOG_ERROR(
                        "Flag `scheduling_group_size` and `numa_aware` are only "
                        "respected if `customized` scheduling optimization strategy is used. "
                        "We're still respecting your parameters to keep the old behavior. Try "
                        "set `fiber_scheduling_optimize_for` to `customized` to suppress "
                        "this error.");
                return std::nullopt;
            }

            static const std::unordered_map<std::string, SchedulingProfile> kProfiles = {
                    {"compute-heavy", SchedulingProfile::ComputeHeavy},
                    {"compute",       SchedulingProfile::Compute},
                    {"neutral",       SchedulingProfile::Neutral},
                    {"io",            SchedulingProfile::Io},
                    {"io-heavy",      SchedulingProfile::IoHeavy}};
            auto key = abel::string_to_lower(fiber_config::get_global_fiber_config().fiber_scheduling_optimize_for);
            DLOG_INFO("Using fiber scheduling profile [{}].", key);

            if (auto iter = kProfiles.find(key); iter != kProfiles.end()) {
                return iter->second;
            }
            if (key == "customized") {
                return std::nullopt;
            }
            DLOG_CRITICAL(
                    "Unrecognized value for `--fiber_scheduling_optimize_for`: [{}]",
                    fiber_config::get_global_fiber_config().fiber_scheduling_optimize_for);
            return std::nullopt;
        }

        void InitializeSchedulingParametersFromFlags() {

            auto profile = GetSchedulingProfile();
            fiber_concurrency_in_effect =
                    fiber_config::get_global_fiber_config().concurrency_hint ? fiber_config::get_global_fiber_config().concurrency_hint
                                           : 4;
            if (profile) {
                scheduling_parameters = GetSchedulingParameters(
                        *profile, numa::get_number_of_nodes_available(),
                        GetNumberOfProcessorsAvailable(),
                        fiber_concurrency_in_effect);
                return;
            }
            std::size_t groups =
                    (fiber_concurrency_in_effect + fiber_config::get_global_fiber_config().scheduling_group_size - 1) /
                            fiber_config::get_global_fiber_config().scheduling_group_size;
            scheduling_parameters = SchedulingParameters{
                    .scheduling_groups = groups,
                    .workers_per_group = (fiber_concurrency_in_effect + groups - 1) / groups,
                    .enable_numa_affinity = fiber_config::get_global_fiber_config().numa_aware};
        }

    }  // namespace fiber_internal

    void start_runtime() {
        // Get our final decision for scheduling parameters.
        fiber_internal::InitializeSchedulingParametersFromFlags();
        // If CPU migration is explicit disallowed, we need to make sure there are
        // enough CPUs for us.
        //DisallowProcessorMigrationPreconditionCheck();
        DLOG_INFO("enable_numa_affinity: {}", scheduling_parameters.enable_numa_affinity);
        if (scheduling_parameters.enable_numa_affinity) {
            StartWorkersNuma();
        } else {
            StartWorkersUma();
        }


        // Fill `flatten_scheduling_groups`.
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                flatten_scheduling_groups.push_back(ee.get());
            }
        }

        // Start the workers.
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                ee->Start(fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration);
            }
        }

    }

    void terminate_runtime() {
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                ee->Stop();
            }
        }
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                ee->Join();
            }
        }
        for (auto &&e : scheduling_groups) {
            e.clear();
        }
        flatten_scheduling_groups.clear();
    }

    std::size_t get_scheduling_group_count() {
        return flatten_scheduling_groups.size();
    }

    std::size_t get_scheduling_group_size() {
        return scheduling_parameters.workers_per_group;
    }

    int get_scheduling_group_assigned_node(std::size_t sg_index) {
        DCHECK_LT(sg_index, flatten_scheduling_groups.size());
        return flatten_scheduling_groups[sg_index]->node_id;
    }

    namespace fiber_internal {

        scheduling_group *routine_get_scheduling_group(std::size_t index) {
            DCHECK_LT(index, flatten_scheduling_groups.size());
            return flatten_scheduling_groups[index]->scheduling_group.get();
        }

        scheduling_group *nearest_scheduling_group_slow(scheduling_group **cache) {
            if (auto rc = scheduling_group::current()) {
                // Only if we indeed belong to the scheduling group (in which case the
                // "nearest" scheduling group never changes) we fill the cache.
                *cache = rc;
                return rc;
            }

            // We don't pay for overhead of initialize `next` unless we're not in running
            // fiber worker.
            ABEL_INTERNAL_TLS_MODEL thread_local std::size_t next = Random();

            auto &&current_node =
                    scheduling_groups[scheduling_parameters.enable_numa_affinity
                                      ? numa::get_current_node()
                                      : 0];
            if (!current_node.empty()) {
                return current_node[next++ % current_node.size()]->scheduling_group.get();
            }

            if (!flatten_scheduling_groups.empty()) {
                return flatten_scheduling_groups[next++ % flatten_scheduling_groups.size()]
                        ->scheduling_group.get();
            }

            return nullptr;
        }

        std::ptrdiff_t nearest_scheduling_group_index() {
            ABEL_INTERNAL_TLS_MODEL thread_local auto cached = []() -> std::ptrdiff_t {
                auto sg = nearest_scheduling_group();
                if (sg) {
                    auto iter = std::find_if(
                            flatten_scheduling_groups.begin(), flatten_scheduling_groups.end(),
                            [&](auto &&e) { return e->scheduling_group.get() == sg; });
                    DCHECK(iter != flatten_scheduling_groups.end());
                    return iter - flatten_scheduling_groups.begin();
                }
                return -1;
            }();
            return cached;
        }

    }  // namespace fiber_internal

}  // namespace abel
