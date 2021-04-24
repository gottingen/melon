//
// Created by 李寅斌 on 2021/2/17.
//

#ifndef ABEL_ATOMIC_BITSET_H_
#define ABEL_ATOMIC_BITSET_H_

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>

namespace abel {

// An atomic bitset of fixed size (specified at compile time).
template<size_t N>
class atomic_bitset {
  public:

    // Construct a atomic_bitset; all bits are initially false.
    atomic_bitset();

    atomic_bitset(const atomic_bitset &) = delete;

    atomic_bitset &operator=(const atomic_bitset &) = delete;

    /// Set bit idx to true, using the given memory order. Returns the
    /// previous value of the bit.
    ///
    /// Note that the operation is a read-modify-write operation due to the use
    /// of fetch_or.
    ///
    bool set(size_t idx, std::memory_order order = std::memory_order_seq_cst);

    ///
    /// Set bit idx to false, using the given memory order. Returns the
    /// previous value of the bit.
    ///
    /// Note that the operation is a read-modify-write operation due to the use
    /// of fetch_and.

    bool reset(size_t idx, std::memory_order order = std::memory_order_seq_cst);


    /// Set bit idx to the given value, using the given memory order. Returns
    /// the previous value of the bit.
    ///
    /// Note that the operation is a read-modify-write operation due to the use
    /// of fetch_and or fetch_or.
    ///
    /// Yes, this is an overload of set(), to keep as close to std::bitset's
    /// interface as possible.
    ///
    bool set(
            size_t idx,
            bool value,
            std::memory_order order = std::memory_order_seq_cst);

    // Read bit idx.
    bool test(
            size_t idx, std::memory_order order = std::memory_order_seq_cst) const;


    // Same as test() with the default memory order.

    bool operator[](size_t idx) const;


    // Return the size of the bitset.

    constexpr size_t size() const { return N; }

  private:
    // Pick the largest lock-free type available
#if (ATOMIC_LLONG_LOCK_FREE == 2)
    typedef unsigned long long BlockType;
#elif (ATOMIC_LONG_LOCK_FREE == 2)
    typedef unsigned long BlockType;
#else
// Even if not lock free, what can we do?
typedef unsigned int BlockType;
#endif
    typedef std::atomic<BlockType> AtomicBlockType;

    static constexpr size_t kBitsPerBlock =
            std::numeric_limits<BlockType>::digits;

    static constexpr size_t block_index(size_t bit) { return bit / kBitsPerBlock; }

    static constexpr size_t bit_offset(size_t bit) { return bit % kBitsPerBlock; }

    // avoid casts
    static constexpr BlockType kOne = 1;
    static constexpr size_t kNumBlocks = (N + kBitsPerBlock - 1) / kBitsPerBlock;
    std::array<AtomicBlockType, kNumBlocks> data_;
};

// value-initialize to zero
template<size_t N>
inline atomic_bitset<N>::atomic_bitset() : data_() {}

template<size_t N>
inline bool atomic_bitset<N>::set(size_t idx, std::memory_order order) {
    assert(idx < N);
    BlockType mask = kOne << bit_offset(idx);
    return data_[block_index(idx)].fetch_or(mask, order) & mask;
}

template<size_t N>
inline bool atomic_bitset<N>::reset(size_t idx, std::memory_order order) {
    assert(idx < N);
    BlockType mask = kOne << bit_offset(idx);
    return data_[block_index(idx)].fetch_and(~mask, order) & mask;
}

template<size_t N>
inline bool atomic_bitset<N>::set(
        size_t idx, bool value, std::memory_order order) {
    return value ? set(idx, order) : reset(idx, order);
}

template<size_t N>
inline bool atomic_bitset<N>::test(
        size_t idx, std::memory_order order) const {
    assert(idx < N);
    BlockType mask = kOne << bit_offset(idx);
    return data_[block_index(idx)].load(order) & mask;
}

template<size_t N>
inline bool atomic_bitset<N>::operator[](size_t idx) const {
    return test(idx);
}


}  // namespace abel

#endif  // ABEL_ATOMIC_BITSET_H_
