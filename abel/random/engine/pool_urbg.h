//

#ifndef ABEL_RANDOM_INTERNAL_POOL_URBG_H_
#define ABEL_RANDOM_INTERNAL_POOL_URBG_H_

#include <cinttypes>
#include <limits>

#include "abel/meta/type_traits.h"
#include "abel/utility/span.h"

namespace abel {

namespace random_internal {

// randen_pool is a thread-safe random number generator [random.req.urbg] that
// uses an underlying pool of randen generators to generate values.  Each thread
// has affinity to one instance of the underlying pool generators.  Concurrent
// access is guarded by a spin-lock.
template<typename T>
class randen_pool {
  public:
    using result_type = T;
    static_assert(std::is_unsigned<result_type>::value,
                  "randen_pool template argument must be a built-in unsigned "
                  "integer type");

    static constexpr result_type (min)() {
        return (std::numeric_limits<result_type>::min)();
    }

    static constexpr result_type (max)() {
        return (std::numeric_limits<result_type>::max)();
    }

    randen_pool() {}

    // Returns a single value.
    ABEL_FORCE_INLINE result_type operator()() { return generate(); }

    // Fill data with random values.
    static void fill(abel::span<result_type> data);

  protected:
    // Generate returns a single value.
    static result_type generate();
};

extern template
class randen_pool<uint8_t>;

extern template
class randen_pool<uint16_t>;

extern template
class randen_pool<uint32_t>;

extern template
class randen_pool<uint64_t>;

// pool_urbg uses an underlying pool of random generators to implement a
// thread-compatible [random.req.urbg] interface with an internal cache of
// values.
template<typename T, size_t kBufferSize>
class pool_urbg {
    // Inheritance to access the protected static members of randen_pool.
    using unsigned_type = typename make_unsigned_bits<T>::type;
    using PoolType = randen_pool<unsigned_type>;
    using SpanType = abel::span<unsigned_type>;

    static constexpr size_t kInitialBuffer = kBufferSize + 1;
    static constexpr size_t kHalfBuffer = kBufferSize / 2;

  public:
    using result_type = T;

    static_assert(std::is_unsigned<result_type>::value,
                  "pool_urbg must be parameterized by an unsigned integer type");

    static_assert(kBufferSize > 1,
                  "pool_urbg must be parameterized by a buffer-size > 1");

    static_assert(kBufferSize <= 256,
                  "pool_urbg must be parameterized by a buffer-size <= 256");

    static constexpr result_type (min)() {
        return (std::numeric_limits<result_type>::min)();
    }

    static constexpr result_type (max)() {
        return (std::numeric_limits<result_type>::max)();
    }

    pool_urbg() : next_(kInitialBuffer) {}

    // copy-constructor does not copy cache.
    pool_urbg(const pool_urbg &) : next_(kInitialBuffer) {}

    const pool_urbg &operator=(const pool_urbg &) {
        next_ = kInitialBuffer;
        return *this;
    }

    // move-constructor does move cache.
    pool_urbg(pool_urbg &&) = default;

    pool_urbg &operator=(pool_urbg &&) = default;

    ABEL_FORCE_INLINE result_type operator()() {
        if (next_ >= kBufferSize) {
            next_ = (kBufferSize > 2 && next_ > kBufferSize) ? kHalfBuffer : 0;
            PoolType::fill(SpanType(reinterpret_cast<unsigned_type *>(state_ + next_),
                                    kBufferSize - next_));
        }
        return state_[next_++];
    }

  private:
    // Buffer size.
    size_t next_;  // index within state_
    result_type state_[kBufferSize];
};

}  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_POOL_URBG_H_
