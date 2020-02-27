
#ifndef TEST_TESTING_MOCK_DISTRIBUTIONS_H_
#define TEST_TESTING_MOCK_DISTRIBUTIONS_H_

#include <limits>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/meta/type_traits.h>
#include <abel/random/distributions.h>
#include <testing/mock_overload_set.h>
#include <testing/mocking_bit_gen.h>

namespace abel {


// -----------------------------------------------------------------------------
// abel::MockUniform
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Uniform.
//
// `abel::MockUniform` is a class template used in conjunction with Googletest's
// `ON_CALL()` and `EXPECT_CALL()` macros. To use it, default-construct an
// instance of it inside `ON_CALL()` or `EXPECT_CALL()`, and use `Call(...)` the
// same way one would define mocks on a Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockUniform<uint32_t>(), Call(mock))
//     .WillOnce(Return(123456));
//  auto x = abel::Uniform<uint32_t>(mock);
//  assert(x == 123456)
//
    template<typename R>
    using MockUniform = random_internal::MockOverloadSet<
            random_internal::UniformDistributionWrapper<R>,
            R(IntervalClosedOpenTag, MockingBitGen &, R, R),
            R(IntervalClosedClosedTag, MockingBitGen &, R, R),
            R(IntervalOpenOpenTag, MockingBitGen &, R, R),
            R(IntervalOpenClosedTag, MockingBitGen &, R, R), R(MockingBitGen &, R, R),
            R(MockingBitGen &)>;

// -----------------------------------------------------------------------------
// abel::MockBernoulli
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Bernoulli.
//
// `abel::MockBernoulli` is a class used in conjunction with Googletest's
// `ON_CALL()` and `EXPECT_CALL()` macros. To use it, default-construct an
// instance of it inside `ON_CALL()` or `EXPECT_CALL()`, and use `Call(...)` the
// same way one would define mocks on a Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockBernoulli(), Call(mock, testing::_))
//     .WillOnce(Return(false));
//  assert(abel::Bernoulli(mock, 0.5) == false);
//
    using MockBernoulli =
    random_internal::MockOverloadSet<abel::bernoulli_distribution,
            bool(MockingBitGen &, double)>;

// -----------------------------------------------------------------------------
// abel::MockBeta
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Beta.
//
// `abel::MockBeta` is a class used in conjunction with Googletest's `ON_CALL()`
// and `EXPECT_CALL()` macros. To use it, default-construct an instance of it
// inside `ON_CALL()` or `EXPECT_CALL()`, and use `Call(...)` the same way one
// would define mocks on a Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockBeta(), Call(mock, 3.0, 2.0))
//     .WillOnce(Return(0.567));
//  auto x = abel::Beta<double>(mock, 3.0, 2.0);
//  assert(x == 0.567);
//
    template<typename RealType>
    using MockBeta =
    random_internal::MockOverloadSet<abel::beta_distribution<RealType>,
            RealType(MockingBitGen &, RealType,
                     RealType)>;

// -----------------------------------------------------------------------------
// abel::MockExponential
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Exponential.
//
// `abel::MockExponential` is a class template used in conjunction with
// Googletest's `ON_CALL()` and `EXPECT_CALL()` macros. To use it,
// default-construct an instance of it inside `ON_CALL()` or `EXPECT_CALL()`,
// and use `Call(...)` the same way one would define mocks on a
// Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockExponential<double>(), Call(mock, 0.5))
//     .WillOnce(Return(12.3456789));
//  auto x = abel::Exponential<double>(mock, 0.5);
//  assert(x == 12.3456789)
//
    template<typename RealType>
    using MockExponential =
    random_internal::MockOverloadSet<abel::exponential_distribution<RealType>,
            RealType(MockingBitGen &, RealType)>;

// -----------------------------------------------------------------------------
// abel::MockGaussian
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Gaussian.
//
// `abel::MockGaussian` is a class template used in conjunction with
// Googletest's `ON_CALL()` and `EXPECT_CALL()` macros. To use it,
// default-construct an instance of it inside `ON_CALL()` or `EXPECT_CALL()`,
// and use `Call(...)` the same way one would define mocks on a
// Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockGaussian<double>(), Call(mock, 16.3, 3.3))
//     .WillOnce(Return(12.3456789));
//  auto x = abel::Gaussian<double>(mock, 16.3, 3.3);
//  assert(x == 12.3456789)
//
    template<typename RealType>
    using MockGaussian =
    random_internal::MockOverloadSet<abel::gaussian_distribution<RealType>,
            RealType(MockingBitGen &, RealType,
                     RealType)>;

// -----------------------------------------------------------------------------
// abel::MockLogUniform
// -----------------------------------------------------------------------------
//
// Matches calls to abel::LogUniform.
//
// `abel::MockLogUniform` is a class template used in conjunction with
// Googletest's `ON_CALL()` and `EXPECT_CALL()` macros. To use it,
// default-construct an instance of it inside `ON_CALL()` or `EXPECT_CALL()`,
// and use `Call(...)` the same way one would define mocks on a
// Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockLogUniform<int>(), Call(mock, 10, 10000, 10))
//     .WillOnce(Return(1221));
//  auto x = abel::LogUniform<int>(mock, 10, 10000, 10);
//  assert(x == 1221)
//
    template<typename IntType>
    using MockLogUniform = random_internal::MockOverloadSet<
            abel::log_uniform_int_distribution<IntType>,
            IntType(MockingBitGen &, IntType, IntType, IntType)>;

// -----------------------------------------------------------------------------
// abel::MockPoisson
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Poisson.
//
// `abel::MockPoisson` is a class template used in conjunction with Googletest's
// `ON_CALL()` and `EXPECT_CALL()` macros. To use it, default-construct an
// instance of it inside `ON_CALL()` or `EXPECT_CALL()`, and use `Call(...)` the
// same way one would define mocks on a Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockPoisson<int>(), Call(mock, 2.0))
//     .WillOnce(Return(1221));
//  auto x = abel::Poisson<int>(mock, 2.0);
//  assert(x == 1221)
//
    template<typename IntType>
    using MockPoisson =
    random_internal::MockOverloadSet<abel::poisson_distribution<IntType>,
            IntType(MockingBitGen &, double)>;

// -----------------------------------------------------------------------------
// abel::MockZipf
// -----------------------------------------------------------------------------
//
// Matches calls to abel::Zipf.
//
// `abel::MockZipf` is a class template used in conjunction with Googletest's
// `ON_CALL()` and `EXPECT_CALL()` macros. To use it, default-construct an
// instance of it inside `ON_CALL()` or `EXPECT_CALL()`, and use `Call(...)` the
// same way one would define mocks on a Googletest `MockFunction()`.
//
// Example:
//
//  abel::MockingBitGen mock;
//  EXPECT_CALL(abel::MockZipf<int>(), Call(mock, 1000000, 2.0, 1.0))
//     .WillOnce(Return(1221));
//  auto x = abel::Zipf<int>(mock, 1000000, 2.0, 1.0);
//  assert(x == 1221)
//
    template<typename IntType>
    using MockZipf =
    random_internal::MockOverloadSet<abel::zipf_distribution<IntType>,
            IntType(MockingBitGen &, IntType, double,
                    double)>;

}  // namespace abel

#endif  // TEST_TESTING_MOCK_DISTRIBUTIONS_H_
