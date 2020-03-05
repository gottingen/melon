//
//

#ifndef ABEL_RANDOM_INTERNAL_DISTRIBUTION_CALLER_H_
#define ABEL_RANDOM_INTERNAL_DISTRIBUTION_CALLER_H_

#include <utility>

#include <abel/base/profile.h>

namespace abel {

    namespace random_internal {

// distribution_caller provides an opportunity to overload the general
// mechanism for calling a distribution, allowing for mock-RNG classes
// to intercept such calls.
        template<typename URBG>
        struct distribution_caller {
            // Call the provided distribution type. The parameters are expected
            // to be explicitly specified.
            // DistrT is the distribution type.
            // FormatT is the formatter type:
            //
            // struct FormatT {
            //   using result_type = distribution_t::result_type;
            //   static std::string FormatCall(
            //       const distribution_t& distr,
            //       abel::span<const result_type>);
            //
            //   static std::string FormatExpectation(
            //       abel::string_view match_args,
            //       abel::span<const result_t> results);
            // }
            //
            template<typename DistrT, typename FormatT, typename... Args>
            static typename DistrT::result_type call(URBG *urbg, Args &&... args) {
                DistrT dist(std::forward<Args>(args)...);
                return dist(*urbg);
            }
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_DISTRIBUTION_CALLER_H_
