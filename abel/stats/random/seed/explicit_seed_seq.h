//

#ifndef ABEL_RANDOM_INTERNAL_EXPLICIT_SEED_SEQ_H_
#define ABEL_RANDOM_INTERNAL_EXPLICIT_SEED_SEQ_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <vector>

#include <abel/base/profile.h>

namespace abel {

    namespace random_internal {

// This class conforms to the C++ Standard "Seed Sequence" concept
// [rand.req.seedseq].
//
// An "explicit_seed_seq" is meant to provide a conformant interface for
// forwarding pre-computed seed material to the constructor of a class
// conforming to the "uniform Random Bit Generator" concept. This class makes no
// attempt to mutate the state provided by its constructor, and returns it
// directly via explicit_seed_seq::generate().
//
// If this class is asked to generate more seed material than was provided to
// the constructor, then the remaining bytes will be filled with deterministic,
// nonrandom data.
        class explicit_seed_seq {
        public:
            using result_type = uint32_t;

            explicit_seed_seq() : state_() {}

            // Copy and move both allowed.
            explicit_seed_seq(const explicit_seed_seq &other) = default;

            explicit_seed_seq &operator=(const explicit_seed_seq &other) = default;

            explicit_seed_seq(explicit_seed_seq &&other) = default;

            explicit_seed_seq &operator=(explicit_seed_seq &&other) = default;

            template<typename Iterator>
            explicit_seed_seq(Iterator begin, Iterator end) {
                for (auto it = begin; it != end; it++) {
                    state_.push_back(*it & 0xffffffff);
                }
            }

            template<typename T>
            explicit_seed_seq(std::initializer_list<T> il)
                    : explicit_seed_seq(il.begin(), il.end()) {}

            size_t size() const { return state_.size(); }

            template<typename OutIterator>
            void param(OutIterator out) const {
                std::copy(std::begin(state_), std::end(state_), out);
            }

            template<typename OutIterator>
            void generate(OutIterator begin, OutIterator end) {
                for (size_t index = 0; begin != end; begin++) {
                    *begin = state_.empty() ? 0 : state_[index++];
                    if (index >= state_.size()) {
                        index = 0;
                    }
                }
            }

        protected:
            std::vector<uint32_t> state_;
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_EXPLICIT_SEED_SEQ_H_
