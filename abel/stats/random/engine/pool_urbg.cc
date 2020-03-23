//

#include <abel/stats/random/engine/pool_urbg.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iterator>

#include <abel/base/profile.h>
#include <abel/asl/functional/call_once.h>
#include <abel/system/endian.h>
#include <abel/log/raw_logging.h>
#include <abel/thread/internal/spinlock.h>
#include <abel/system/sysinfo.h>
#include <abel/atomic/unaligned_access.h>
#include <abel/stats/random/engine/randen.h>
#include <abel/stats/random/seed/seed_material.h>
#include <abel/stats/random/seed/seed_gen_exception.h>

using abel::thread_internal::SpinLock;
using abel::thread_internal::SpinLockHolder;

namespace abel {

    namespace random_internal {
        namespace {

// RandenPoolEntry is a thread-safe pseudorandom bit generator, implementing a
// single generator within a randen_pool<T>. It is an internal implementation
// detail, and does not aim to conform to [rand.req.urng].
//
// NOTE: There are alignment issues when used on ARM, for instance.
// See the allocation code in PoolAlignedAlloc().
            class RandenPoolEntry {
            public:
                static constexpr size_t kState = randen_traits::kStateBytes / sizeof(uint32_t);
                static constexpr size_t kCapacity =
                        randen_traits::kCapacityBytes / sizeof(uint32_t);

                void Init(abel::span<const uint32_t> data) {
                    SpinLockHolder l(&mu_);  // Always uncontested.
                    std::copy(data.begin(), data.end(), std::begin(state_));
                    next_ = kState;
                }

                // Copy bytes into out.
                void Fill(uint8_t *out, size_t bytes) ABEL_LOCKS_EXCLUDED(mu_);

                // Returns random bits from the buffer in units of T.
                template<typename T>
                ABEL_FORCE_INLINE T Generate() ABEL_LOCKS_EXCLUDED(mu_);

                ABEL_FORCE_INLINE void MaybeRefill() ABEL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
                    if (next_ >= kState) {
                        next_ = kCapacity;
                        impl_.Generate(state_);
                    }
                }

            private:
                // randen URBG state.
                uint32_t state_[kState] ABEL_GUARDED_BY(mu_);  // First to satisfy alignment.
                SpinLock mu_;
                const randen impl_;
                size_t next_ ABEL_GUARDED_BY(mu_);
            };

            template<>
            ABEL_FORCE_INLINE uint8_t RandenPoolEntry::Generate<uint8_t>() {
                SpinLockHolder l(&mu_);
                MaybeRefill();
                return static_cast<uint8_t>(state_[next_++]);
            }

            template<>
            ABEL_FORCE_INLINE uint16_t RandenPoolEntry::Generate<uint16_t>() {
                SpinLockHolder l(&mu_);
                MaybeRefill();
                return static_cast<uint16_t>(state_[next_++]);
            }

            template<>
            ABEL_FORCE_INLINE uint32_t RandenPoolEntry::Generate<uint32_t>() {
                SpinLockHolder l(&mu_);
                MaybeRefill();
                return state_[next_++];
            }

            template<>
            ABEL_FORCE_INLINE uint64_t RandenPoolEntry::Generate<uint64_t>() {
                SpinLockHolder l(&mu_);
                if (next_ >= kState - 1) {
                    next_ = kCapacity;
                    impl_.Generate(state_);
                }
                auto p = state_ + next_;
                next_ += 2;

                uint64_t result;
                std::memcpy(&result, p, sizeof(result));
                return result;
            }

            void RandenPoolEntry::Fill(uint8_t *out, size_t bytes) {
                SpinLockHolder l(&mu_);
                while (bytes > 0) {
                    MaybeRefill();
                    size_t remaining = (kState - next_) * sizeof(state_[0]);
                    size_t to_copy = std::min(bytes, remaining);
                    std::memcpy(out, &state_[next_], to_copy);
                    out += to_copy;
                    bytes -= to_copy;
                    next_ += (to_copy + sizeof(state_[0]) - 1) / sizeof(state_[0]);
                }
            }

// Number of pooled urbg entries.
            static constexpr int kPoolSize = 8;

// Shared pool entries.
            static abel::once_flag pool_once;
            ABEL_CACHE_LINE_ALIGNED static RandenPoolEntry *shared_pools[kPoolSize];

// Returns an id in the range [0 ... kPoolSize), which indexes into the
// pool of random engines.
//
// Each thread to access the pool is assigned a sequential ID (without reuse)
// from the pool-id space; the id is cached in a thread_local variable.
// This id is assigned based on the arrival-order of the thread to the
// GetPoolID call; this has no binary, CL, or runtime stability because
// on subsequent runs the order within the same program may be significantly
// different. However, as other thread IDs are not assigned sequentially,
// this is not expected to matter.
            int GetPoolID() {
                static_assert(kPoolSize >= 1,
                              "At least one urbg instance is required for pool_urbg");

                ABEL_CONST_INIT static std::atomic<int64_t> sequence{0};

                static thread_local int my_pool_id = -1;
                if (ABEL_UNLIKELY(my_pool_id < 0)) {
                    my_pool_id = (sequence++ % kPoolSize);
                }
                return my_pool_id;

            }

// Allocate a RandenPoolEntry with at least 32-byte alignment, which is required
// by ARM platform code.
            RandenPoolEntry *PoolAlignedAlloc() {
                constexpr size_t kAlignment =
                        ABEL_CACHE_LINE_SIZE > 32 ? ABEL_CACHE_LINE_SIZE : 32;

                // Not all the platforms that we build for have std::aligned_alloc, however
                // since we never free these objects, we can over allocate and munge the
                // pointers to the correct alignment.
                void *memory = std::malloc(sizeof(RandenPoolEntry) + kAlignment);
                auto x = reinterpret_cast<intptr_t>(memory);
                auto y = x % kAlignment;
                void *aligned =
                        (y == 0) ? memory : reinterpret_cast<void *>(x + kAlignment - y);
                return new(aligned) RandenPoolEntry();
            }

// Allocate and initialize kPoolSize objects of type RandenPoolEntry.
//
// The initialization strategy is to initialize one object directly from
// OS entropy, then to use that object to seed all of the individual
// pool instances.
            void InitPoolURBG() {
                static constexpr size_t kSeedSize =
                        randen_traits::kStateBytes / sizeof(uint32_t);
                // Read the seed data from OS entropy once.
                uint32_t seed_material[kPoolSize * kSeedSize];
                if (!random_internal::read_seed_material_from_os_entropy(
                        abel::make_span(seed_material))) {
                    random_internal::throw_seed_gen_exception();
                }
                for (int i = 0; i < kPoolSize; i++) {
                    shared_pools[i] = PoolAlignedAlloc();
                    shared_pools[i]->Init(
                            abel::make_span(&seed_material[i * kSeedSize], kSeedSize));
                }
            }

// Returns the pool entry for the current thread.
            RandenPoolEntry *GetPoolForCurrentThread() {
                abel::call_once(pool_once, InitPoolURBG);
                return shared_pools[GetPoolID()];
            }

        }  // namespace

        template<typename T>
        typename randen_pool<T>::result_type randen_pool<T>::generate() {
            auto *pool = GetPoolForCurrentThread();
            return pool->Generate<T>();
        }

        template<typename T>
        void randen_pool<T>::fill(abel::span<result_type> data) {
            auto *pool = GetPoolForCurrentThread();
            pool->Fill(reinterpret_cast<uint8_t *>(data.data()),
                       data.size() * sizeof(result_type));
        }

        template
        class randen_pool<uint8_t>;

        template
        class randen_pool<uint16_t>;

        template
        class randen_pool<uint32_t>;

        template
        class randen_pool<uint64_t>;

    }  // namespace random_internal

}  // namespace abel
