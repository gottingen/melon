//

#ifndef TEST_TESTING_CHI_SQUARE_H_
#define TEST_TESTING_CHI_SQUARE_H_

// The chi-square statistic.
//
// Useful for evaluating if `D` independent random variables are behaving as
// expected, or if two distributions are similar.  (`D` is the degrees of
// freedom).
//
// Each bucket should have an expected count of 10 or more for the chi square to
// be meaningful.

#include <cassert>

#include <abel/base/profile.h>

namespace abel {

    namespace random_internal {

        constexpr const char kChiSquared[] = "chi-squared";

// Returns the measured chi square value, using a single expected value.  This
// assumes that the values in [begin, end) are uniformly distributed.
        template<typename Iterator>
        double chi_square_with_expected(Iterator begin, Iterator end, double expected) {
            // Compute the sum and the number of buckets.
            assert(expected >= 10);  // require at least 10 samples per bucket.
            double chi_square = 0;
            for (auto it = begin; it != end; it++) {
                double d = static_cast<double>(*it) - expected;
                chi_square += d * d;
            }
            chi_square = chi_square / expected;
            return chi_square;
        }

// Returns the measured chi square value, taking the actual value of each bucket
// from the first set of iterators, and the expected value of each bucket from
// the second set of iterators.
        template<typename Iterator, typename Expected>
        double chi_square(Iterator it, Iterator end, Expected eit, Expected eend) {
            double chi_square = 0;
            for (; it != end && eit != eend; ++it, ++eit) {
                if (*it > 0) {
                    assert(*eit > 0);
                }
                double e = static_cast<double>(*eit);
                double d = static_cast<double>(*it - *eit);
                if (d != 0) {
                    assert(e > 0);
                    chi_square += (d * d) / e;
                }
            }
            assert(it == end && eit == eend);
            return chi_square;
        }

// ======================================================================
// The following methods can be used for an arbitrary significance level.
//

// Calculates critical chi-square values to produce the given p-value using a
// bisection search for a value within epsilon, relying on the monotonicity of
// chi_square_p_value().
        double chi_square_value(int dof, double p);

// Calculates the p-value (probability) of a given chi-square value.
        double chi_square_p_value(double chi_square, int dof);

    }  // namespace random_internal

}  // namespace abel

#endif  // TEST_TESTING_CHI_SQUARE_H_
