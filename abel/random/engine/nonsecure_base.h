//

#ifndef ABEL_RANDOM_INTERNAL_NONSECURE_BASE_H_
#define ABEL_RANDOM_INTERNAL_NONSECURE_BASE_H_

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>
#include <vector>
#include <optional>
#include "abel/utility/span.h"

#include "abel/base/profile.h"
#include "abel/meta/type_traits.h"
#include "abel/random/engine/pool_urbg.h"
#include "abel/random/seed/salted_seed_seq.h"
#include "abel/random/seed/seed_material.h"


namespace abel {

namespace random_internal {

// Each instance of nonsecure_urgb_base<URBG> will be seeded by variates produced
// by a thread-unique URBG-instance.
template<typename URBG>
class nonsecure_urgb_base {
  public:
    using result_type = typename URBG::result_type;

    // Default constructor
    nonsecure_urgb_base() : urbg_(construct_urgb()) {}

    // Copy disallowed, move allowed.
    nonsecure_urgb_base(const nonsecure_urgb_base &) = delete;

    nonsecure_urgb_base &operator=(const nonsecure_urgb_base &) = delete;

    nonsecure_urgb_base(nonsecure_urgb_base &&) = default;

    nonsecure_urgb_base &operator=(nonsecure_urgb_base &&) = default;

    // Constructor using a seed
    template<class SSeq, typename = typename abel::enable_if_t<
            !std::is_same<SSeq, nonsecure_urgb_base>::value>>
    explicit nonsecure_urgb_base(SSeq &&seq)
            : urbg_(construct_urgb(std::forward<SSeq>(seq))) {}

    // Note: on MSVC, min() or max() can be interpreted as MIN() or MAX(), so we
    // enclose min() or max() in parens as (min)() and (max)().
    // Additionally, clang-format requires no space before this construction.

    // nonsecure_urgb_base::min()
    static constexpr result_type (min)() { return (URBG::min)(); }

    // nonsecure_urgb_base::max()
    static constexpr result_type (max)() { return (URBG::max)(); }

    // nonsecure_urgb_base::operator()()
    result_type operator()() { return urbg_(); }

    // nonsecure_urgb_base::discard()
    void discard(unsigned long long values) {  // NOLINT(runtime/int)
        urbg_.discard(values);
    }

    bool operator==(const nonsecure_urgb_base &other) const {
        return urbg_ == other.urbg_;
    }

    bool operator!=(const nonsecure_urgb_base &other) const {
        return !(urbg_ == other.urbg_);
    }

  private:
    // Seeder is a custom seed sequence type where generate() fills the provided
    // buffer via the randen_pool entropy source.
    struct Seeder {
        using result_type = uint32_t;

        size_t size() { return 0; }

        template<typename OutIterator>
        void param(OutIterator) const {}

        template<typename RandomAccessIterator>
        void generate(RandomAccessIterator begin, RandomAccessIterator end) {
            if (begin != end) {
                // begin, end must be random access iterators assignable from uint32_t.
                generate_impl(
                        std::integral_constant<bool, sizeof(*begin) == sizeof(uint32_t)>{},
                        begin, end);
            }
        }

        // Commonly, generate is invoked with a pointer to a buffer which
        // can be cast to a uint32_t.
        template<typename RandomAccessIterator>
        void generate_impl(std::integral_constant<bool, true>,
                           RandomAccessIterator begin, RandomAccessIterator end) {
            auto buffer = abel::make_span(begin, end);
            auto target = abel::make_span(reinterpret_cast<uint32_t *>(buffer.data()),
                                          buffer.size());
            randen_pool<uint32_t>::fill(target);
        }

        // The non-uint32_t case should be uncommon, and involves an extra copy,
        // filling the uint32_t buffer and then mixing into the output.
        template<typename RandomAccessIterator>
        void generate_impl(std::integral_constant<bool, false>,
                           RandomAccessIterator begin, RandomAccessIterator end) {
            const size_t n = std::distance(begin, end);
            abel::inlined_vector<uint32_t, 8> data(n, 0);
            randen_pool<uint32_t>::fill(abel::make_span(data.begin(), data.end()));
            std::copy(std::begin(data), std::end(data), begin);
        }
    };

    static URBG construct_urgb() {
        Seeder seeder;
        return URBG(seeder);
    }

    template<typename SSeq>
    static URBG construct_urgb(SSeq &&seq) {  // NOLINT(runtime/references)
        auto salted_seq =
                random_internal::Makesalted_seed_seq(std::forward<SSeq>(seq));
        return URBG(salted_seq);
    }

    URBG urbg_;
};

}  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_NONSECURE_BASE_H_
