//

#ifndef ABEL_RANDOM_INTERNAL_SEQUENCE_URBG_H_
#define ABEL_RANDOM_INTERNAL_SEQUENCE_URBG_H_

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

#include <abel/base/profile.h>

namespace abel {

    namespace random_internal {

// `sequence_urbg` is a simple random number generator which meets the
// requirements of [rand.req.urbg], and is solely for testing abel
// distributions.
        class sequence_urbg {
        public:
            using result_type = uint64_t;

            static constexpr result_type (min)() {
                return (std::numeric_limits<result_type>::min)();
            }

            static constexpr result_type (max)() {
                return (std::numeric_limits<result_type>::max)();
            }

            sequence_urbg(std::initializer_list<result_type> data) : i_(0), data_(data) {}

            void reset() { i_ = 0; }

            result_type operator()() { return data_[i_++ % data_.size()]; }

            size_t invocations() const { return i_; }

        private:
            size_t i_;
            std::vector<result_type> data_;
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_SEQUENCE_URBG_H_
