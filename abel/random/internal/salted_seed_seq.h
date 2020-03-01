//

#ifndef ABEL_RANDOM_INTERNAL_SALTED_SEED_SEQ_H_
#define ABEL_RANDOM_INTERNAL_SALTED_SEED_SEQ_H_

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include <abel/container/inlined_vector.h>
#include <abel/meta/type_traits.h>
#include <abel/random/internal/seed_material.h>
#include <abel/types/optional.h>
#include <abel/types/span.h>

namespace abel {

    namespace random_internal {

// This class conforms to the C++ Standard "Seed Sequence" concept
// [rand.req.seedseq].
//
// A `SaltedSeedSeq` is meant to wrap an existing seed sequence and modify
// generated sequence by mixing with extra entropy. This entropy may be
// build-dependent or process-dependent. The implementation may change to be
// have either or both kinds of entropy. If salt is not available sequence is
// not modified.
        template<typename SSeq>
        class SaltedSeedSeq {
        public:
            using inner_sequence_type = SSeq;
            using result_type = typename SSeq::result_type;

            SaltedSeedSeq() : seq_(abel::make_unique<SSeq>()) {}

            template<typename Iterator>
            SaltedSeedSeq(Iterator begin, Iterator end)
                    : seq_(abel::make_unique<SSeq>(begin, end)) {}

            template<typename T>
            SaltedSeedSeq(std::initializer_list<T> il)
                    : SaltedSeedSeq(il.begin(), il.end()) {}

            SaltedSeedSeq(const SaltedSeedSeq &) = delete;

            SaltedSeedSeq &operator=(const SaltedSeedSeq &) = delete;

            SaltedSeedSeq(SaltedSeedSeq &&) = default;

            SaltedSeedSeq &operator=(SaltedSeedSeq &&) = default;

            template<typename RandomAccessIterator>
            void generate(RandomAccessIterator begin, RandomAccessIterator end) {
                // The common case is that generate is called with ContiguousIterators
                // to uint arrays. Such contiguous memory regions may be optimized,
                // which we detect here.
                using tag = abel::conditional_t<
                        (std::is_pointer<RandomAccessIterator>::value &&
                         std::is_same<abel::decay_t<decltype(*begin)>, uint32_t>::value),
                        ContiguousAndUint32Tag, DefaultTag>;
                if (begin != end) {
                    generate_impl(begin, end, tag{});
                }
            }

            template<typename OutIterator>
            void param(OutIterator out) const {
                seq_->param(out);
            }

            size_t size() const { return seq_->size(); }

        private:
            struct ContiguousAndUint32Tag {
            };
            struct DefaultTag {
            };

            // Generate which requires the iterators are contiguous pointers to uint32_t.
            void generate_impl(uint32_t *begin, uint32_t *end, ContiguousAndUint32Tag) {
                generate_contiguous(abel::MakeSpan(begin, end));
            }

            // The uncommon case for generate is that it is called with iterators over
            // some other buffer type which is assignable from a 32-bit value. In this
            // case we allocate a temporary 32-bit buffer and then copy-assign back
            // to the initial inputs.
            template<typename RandomAccessIterator>
            void generate_impl(RandomAccessIterator begin, RandomAccessIterator end,
                               DefaultTag) {
                return generate_and_copy(std::distance(begin, end), begin);
            }

            // Fills the initial seed buffer the underlying SSeq::generate() call,
            // mixing in the salt material.
            void generate_contiguous(abel::Span<uint32_t> buffer) {
                seq_->generate(buffer.begin(), buffer.end());
                const uint32_t salt = abel::random_internal::GetSaltMaterial().value_or(0);
                MixIntoSeedMaterial(abel::MakeConstSpan(&salt, 1), buffer);
            }

            // Allocates a seed buffer of `n` elements, generates the seed, then
            // copies the result into the `out` iterator.
            template<typename Iterator>
            void generate_and_copy(size_t n, Iterator out) {
                // Allocate a temporary buffer, generate, and then copy.
                abel::InlinedVector<uint32_t, 8> data(n, 0);
                generate_contiguous(abel::MakeSpan(data.data(), data.size()));
                std::copy(data.begin(), data.end(), out);
            }

            // Because [rand.req.seedseq] is not required to be copy-constructible,
            // copy-assignable nor movable, we wrap it with unique pointer to be able
            // to move SaltedSeedSeq.
            std::unique_ptr<SSeq> seq_;
        };

// is_salted_seed_seq indicates whether the type is a SaltedSeedSeq.
        template<typename T, typename = void>
        struct is_salted_seed_seq : public std::false_type {
        };

        template<typename T>
        struct is_salted_seed_seq<
                T, typename std::enable_if<std::is_same<
                        T, SaltedSeedSeq<typename T::inner_sequence_type>>::value>::type>
                : public std::true_type {
        };

// MakeSaltedSeedSeq returns a salted variant of the seed sequence.
// When provided with an existing SaltedSeedSeq, returns the input parameter,
// otherwise constructs a new SaltedSeedSeq which embodies the original
// non-salted seed parameters.
        template<
                typename SSeq,  //
                typename EnableIf = abel::enable_if_t<is_salted_seed_seq<SSeq>::value>>
        SSeq MakeSaltedSeedSeq(SSeq &&seq) {
            return SSeq(std::forward<SSeq>(seq));
        }

        template<
                typename SSeq,  //
                typename EnableIf = abel::enable_if_t<!is_salted_seed_seq<SSeq>::value>>
        SaltedSeedSeq<typename std::decay<SSeq>::type> MakeSaltedSeedSeq(SSeq &&seq) {
            using sseq_type = typename std::decay<SSeq>::type;
            using result_type = typename sseq_type::result_type;

            abel::InlinedVector<result_type, 8> data;
            seq.param(std::back_inserter(data));
            return SaltedSeedSeq<sseq_type>(data.begin(), data.end());
        }

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_SALTED_SEED_SEQ_H_
