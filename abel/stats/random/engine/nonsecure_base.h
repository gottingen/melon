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

#include <abel/base/profile.h>
#include <abel/meta/type_traits.h>
#include <abel/stats/random/engine/pool_urbg.h>
#include <abel/stats/random/seed/salted_seed_seq.h>
#include <abel/stats/random/seed/seed_material.h>
#include <abel/types/optional.h>
#include <abel/types/span.h>

namespace abel {

    namespace random_internal {

// Each instance of NonsecureURBGBase<URBG> will be seeded by variates produced
// by a thread-unique URBG-instance.
        template<typename URBG>
        class NonsecureURBGBase {
        public:
            using result_type = typename URBG::result_type;

            // Default constructor
            NonsecureURBGBase() : urbg_(ConstructURBG()) {}

            // Copy disallowed, move allowed.
            NonsecureURBGBase(const NonsecureURBGBase &) = delete;

            NonsecureURBGBase &operator=(const NonsecureURBGBase &) = delete;

            NonsecureURBGBase(NonsecureURBGBase &&) = default;

            NonsecureURBGBase &operator=(NonsecureURBGBase &&) = default;

            // Constructor using a seed
            template<class SSeq, typename = typename abel::enable_if_t<
                    !std::is_same<SSeq, NonsecureURBGBase>::value>>
            explicit NonsecureURBGBase(SSeq &&seq)
                    : urbg_(ConstructURBG(std::forward<SSeq>(seq))) {}

            // Note: on MSVC, min() or max() can be interpreted as MIN() or MAX(), so we
            // enclose min() or max() in parens as (min)() and (max)().
            // Additionally, clang-format requires no space before this construction.

            // NonsecureURBGBase::min()
            static constexpr result_type (min)() { return (URBG::min)(); }

            // NonsecureURBGBase::max()
            static constexpr result_type (max)() { return (URBG::max)(); }

            // NonsecureURBGBase::operator()()
            result_type operator()() { return urbg_(); }

            // NonsecureURBGBase::discard()
            void discard(unsigned long long values) {  // NOLINT(runtime/int)
                urbg_.discard(values);
            }

            bool operator==(const NonsecureURBGBase &other) const {
                return urbg_ == other.urbg_;
            }

            bool operator!=(const NonsecureURBGBase &other) const {
                return !(urbg_ == other.urbg_);
            }

        private:
            // Seeder is a custom seed sequence type where generate() fills the provided
            // buffer via the RandenPool entropy source.
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
                    RandenPool<uint32_t>::Fill(target);
                }

                // The non-uint32_t case should be uncommon, and involves an extra copy,
                // filling the uint32_t buffer and then mixing into the output.
                template<typename RandomAccessIterator>
                void generate_impl(std::integral_constant<bool, false>,
                                   RandomAccessIterator begin, RandomAccessIterator end) {
                    const size_t n = std::distance(begin, end);
                    abel::InlinedVector<uint32_t, 8> data(n, 0);
                    RandenPool<uint32_t>::Fill(abel::make_span(data.begin(), data.end()));
                    std::copy(std::begin(data), std::end(data), begin);
                }
            };

            static URBG ConstructURBG() {
                Seeder seeder;
                return URBG(seeder);
            }

            template<typename SSeq>
            static URBG ConstructURBG(SSeq &&seq) {  // NOLINT(runtime/references)
                auto salted_seq =
                        random_internal::MakeSaltedSeedSeq(std::forward<SSeq>(seq));
                return URBG(salted_seq);
            }

            URBG urbg_;
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_NONSECURE_BASE_H_
