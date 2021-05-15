//
// Created by liyinbin on 2021/4/5.
//


#include "abel/thread/numa.h"
#include <dlfcn.h>
#include <unistd.h>

#if defined(ABEL_PLATFORM_LINUX)
#include <sys/sysinfo.h>
#include <syscall.h>
#include <dlfcn.h>
#endif

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>

#include "abel/log/logging.h"
#include "abel/strings/str_split.h"
#include "abel/container/flat_hash_set.h"
#include "abel/strings/numbers.h"
#include "abel/base/profile.h"

namespace abel {


    namespace {

        // Initialized by `InitializeProcessorInfoOnce()`.
        bool inaccessible_cpus_present{false};
        std::vector<int> node_of_cpus;
        std::vector<std::size_t> node_index;
        std::vector<int> nodes_present;

        bool is_valgrind_present() {
            // You need to export this var yourself in bash.
            char *rov = getenv("RUNNING_ON_VALGRIND");
            if (rov) {
                return strcmp(rov, "0") != 0;
            }
            return false;
        }

        int SyscallGetCpu(unsigned *cpu, unsigned *node, void *cache) {
#if defined(ABEL_PLATFORM_LINUX)
#ifndef SYS_getcpu
            DLOG_CRITICAL(
                    "Not supported: sys_getcpu. Note that this is only required if you want "
                    "to run program in valgrind or on some exotic ISAs.");
#else
            return syscall(SYS_getcpu, cpu, node, cache);
#endif
#else
            *cpu = GetNumberOfProcessorsAvailable();
            *node = 1;
            cache = nullptr;
            return 0;
#endif
        }

// @sa: https://gist.github.com/chergert/eb6149916b10d3bf094c
        int (*GetCpu)(unsigned *cpu, unsigned *node, void *cache) = [] {
            if (is_valgrind_present()) {
                return SyscallGetCpu;
            } else {
#if defined(__aarch64__)
                // `getcpu` is not available on AArch64.
                return SyscallGetCpu;
#endif
                // Not all ISAs use the same name here. We'll try our best to locate
                // `getcpu` via vDSO.
                //
                // @sa http://man7.org/linux/man-pages/man7/vdso.7.html for more details.

                static const char *kvDSONames[] = {"linux-gate.so.1", "linux-vdso.so.1",
                                                   "linux-vdso32.so.1",
                                                   "linux-vdso64.so.1"};
                static const char *kGetCpuNames[] = {"__vdso_getcpu", "__kernel_getcpu"};
                for (auto &&e : kvDSONames) {
                    if (auto vdso = dlopen(e, RTLD_NOW)) {
                        for (auto &&e2 : kGetCpuNames) {
                            if (auto p = dlsym(vdso, e2)) {
                                // AFAICT, leaking `vdso` does nothing harmful to us. We use a
                                // managed pointer to hold it only to comfort Coverity. (But it
                                // still doesn't seem comforttable with this..)
                                [[maybe_unused]] static std::unique_ptr<void, int (*)(void *)>
                                        suppress_vdso_leak{vdso, &dlclose};

                                return reinterpret_cast<int (*)(unsigned *, unsigned *, void *)>(p);
                            }
                        }
                        dlclose(vdso);
                    }
                }
                // Fall back to syscall. This can be slow.
                DLOG_WARN("Failed to locate `getcpu` in vDSO. Failing back to syscall. "
                          "Performance will degrade.");
                return SyscallGetCpu;
            }
        }();

        int GetNodeOfProcessorImpl(int proc_id) {
            // Slow, indeed. We don't expect this method be called much.
            std::atomic<int> rc;
            std::thread([&] {
                if (auto err = TrySetCurrentThreadAffinity({proc_id}); err != 0) {
                    DCHECK(err == EINVAL, "Unexpected error #{}: {}", err,
                               strerror(err));
                    rc = -1;
                    return;
                }
                unsigned cpu, node;
                DCHECK(GetCpu(&cpu, &node, nullptr) == 0, "`GetCpu` failed.");
                rc = node;
            }).join();
            return rc.load();
        }

// Initialize those global variables.
        void InitializeProcessorInfoOnce() {
            // I don't think it's possible to print log reliably here, unfortunately.
            static std::once_flag once;
            std::call_once(once, [&] {
                node_of_cpus.resize(GetNumberOfProcessorsConfigured(), -1);
                for (size_t i = 0; i != node_of_cpus.size(); ++i) {
                    auto n = GetNodeOfProcessorImpl(i);
                    if (n == -1) {
                        // Failed to determine the processor's belonging node.
                        inaccessible_cpus_present = true;
                    } else {
                        if (node_index.size() < n + 1) {
                            node_index.resize(n + 1, -1);
                        }
                        if (node_index[n] == -1) {
                            // New node discovered.
                            node_index[n] = nodes_present.size();
                            nodes_present.push_back(n);
                        }
                        node_of_cpus[i] = n;
                        // New processor discovered.
                    }
                }
            });
        }

        struct ProcessorInfoInitializer {
            ProcessorInfoInitializer() { InitializeProcessorInfoOnce(); }
        } processor_info_initializer [[maybe_unused]];

    }  // namespace

    namespace numa {

        namespace {

            std::vector<numa_node> GetAvailableNodesImpl() {
                // NUMA node -> vector of processor ID.
                std::unordered_map<int, std::vector<int>> desc;
                for (int index = 0; index != node_of_cpus.size(); ++index) {
                    auto n = node_of_cpus[index];
                    if (n == -1) {
                        DLOG_WARN(
                                "Cannot determine node ID of processor #{}, we're silently ignoring "
                                "that CPU. Unless that CPU indeed shouldn't be used by the program "
                                "(e.g., containerized environment or disabled), you should check the "
                                "situation as it can have a negative impact on performance.",
                                index);
                        continue;
                    }
                    desc[n].push_back(index);
                }

                std::vector<numa_node> rc;
                for (auto &&id : nodes_present) {
                    numa_node n;
                    n.id = id;
                    n.logical_cpus = desc[id];
                    rc.push_back(n);
                }
                return rc;
            }

        }  // namespace

        std::vector<numa_node> get_available_nodes() {
            static const auto rc = GetAvailableNodesImpl();
            return rc;
        }

        int get_current_node() {
            unsigned cpu, node;

            // Another approach: https://stackoverflow.com/a/27450168
            DCHECK(0 == GetCpu(&cpu, &node, nullptr), "Cannot get NUMA ID.");
            return node;
        }

        std::size_t get_current_node_index() { return get_node_index(get_current_node()); }

        int get_node_id(std::size_t index) {
            DCHECK_LT(index, nodes_present.size());
            return nodes_present[index];
        }

        std::size_t get_node_index(int node_id) {
            DCHECK_LT(node_id, node_index.size());
            DCHECK_LT(node_index[node_id], nodes_present.size());
            return node_index[node_id];
        }

        std::size_t get_number_of_nodes_available() { return nodes_present.size(); }

        int get_node_of_processor(int cpu) {
            DCHECK_LE(cpu, node_of_cpus.size());
            auto node = node_of_cpus[cpu];
            DCHECK(node != -1, "Processor #{} is not accessible.", cpu);
            return node;
        }

    }  // namespace numa

    int get_current_processor_id() {
        unsigned cpu, node;
        DCHECK(0 == GetCpu(&cpu, &node, nullptr), "Cannot get current CPU ID.");
        return cpu;
    }

    std::size_t GetNumberOfProcessorsAvailable() {
        // We do not support CPU hot-plugin, so we use `static` here.
#if defined(ABEL_PLATFORM_LINUX)
        static const auto rc = get_nprocs();
#elif defined(ABEL_PLATFORM_OSX)
        static const auto rc = static_cast<unsigned int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
        return rc;
    }

    std::size_t GetNumberOfProcessorsConfigured() {
        // We do not support CPU hot-plugin, so we use `static` here.
#if defined(ABEL_PLATFORM_LINUX)
        static const auto rc = get_nprocs_conf();
#elif defined(ABEL_PLATFORM_OSX)
        static const auto rc = static_cast<unsigned int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
        return rc;
    }

    bool IsInaccessibleProcessorPresent() { return inaccessible_cpus_present; }

    bool IsProcessorAccessible(int cpu) {
        DCHECK_LT(cpu, node_of_cpus.size());
        return node_of_cpus[cpu] != -1;
    }

    std::optional<std::vector<int>> TryParseProcesserList(const std::string &s) {
        std::vector<int> result;
        std::vector<std::string> parts = abel::string_split(s, ",");
        for (auto &&e : parts) {
            int64_t id;
            auto idr = abel::simple_atoi(e, &id);
            if (idr) {
                if (id < 0) {
                    result.push_back(GetNumberOfProcessorsConfigured() +
                                     id);
                    if (result.back() < 0) {
                        return std::nullopt;
                    }
                } else {
                    result.push_back(id);
                }
            } else {
                std::vector<std::string> range = abel::string_split(e, "-");
                if (range.size() != 2) {
                    return std::nullopt;
                }
                int64_t s;
                int64_t e;
                auto sr = abel::simple_atoi(range[0], &s);
                auto er = abel::simple_atoi(range[1], &e);
                if (!sr || !er || s > e) {
                    return std::nullopt;
                }
                for (int i = s; i <= e; ++i) {
                    result.push_back(i);
                }
            }
        }
        return result;
    }


    int TrySetCurrentThreadAffinity(const std::vector<int> &affinity) {
#if defined(ABEL_PLATFORM_LINUX)
        DCHECK(!affinity.empty());
        cpu_set_t cpuset;

        CPU_ZERO(&cpuset);
        for (auto&& e : affinity) {
            CPU_SET(e, &cpuset);
        }
        return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
        ABEL_UNUSED(affinity);
        return 0;
#endif

    }

    void SetCurrentThreadAffinity(const std::vector<int> &affinity) {
        auto rc = TrySetCurrentThreadAffinity(affinity);
        DCHECK(rc == 0, "Cannot set thread affinity for thread [{}]: [{}] {}.",
                   reinterpret_cast<const void *>(pthread_self()), rc, strerror(rc));
    }

    std::vector<int> GetCurrentThreadAffinity() {
#if defined(ABEL_PLATFORM_LINUX)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        auto rc = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        DCHECK(rc == 0, "Cannot get thread affinity of thread [{}]: [{}] {}.",
                    reinterpret_cast<const void*>(pthread_self()), rc, strerror(rc));

        std::vector<int> result;
        for (int i = 0; i != __CPU_SETSIZE; ++i) {
            if (CPU_ISSET(i, &cpuset)) {
                result.push_back(i);
            }
        }
        return result;
#else
        return std::vector<int>();
#endif
    }

    void SetCurrentThreadName(const std::string &name) {

#if defined(ABEL_PLATFORM_LINUX)
        auto rc = pthread_setname_np(pthread_self(), name.c_str());
        if (rc != 0) {
            DLOG_WARN("Cannot set name for thread [{}]: [{}] {}",
                              reinterpret_cast<const void*>(pthread_self()), rc,
                              strerror(rc));
            // Silently ignored.
        }
#else
    ABEL_UNUSED(name);
#endif
    }
}  // namespace abel
