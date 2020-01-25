//

#include <abel/random/exponential_distribution.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/log/raw_logging.h>
#include <abel/base/profile.h>
#include <test/testing/chi_square.h>
#include <test/testing/distribution_test_util.h>
#include <abel/random/internal/sequence_urbg.h>
#include <abel/random/random.h>
#include <abel/strings/str_cat.h>
#include <abel/format/str_format.h>
#include <abel/strings/str_replace.h>
#include <abel/strings/strip.h>

namespace {

using abel::random_internal::kChiSquared;

template <typename RealType>
class ExponentialDistributionTypedTest : public ::testing::Test {};

#if defined(__EMSCRIPTEN__)
using RealTypes = ::testing::Types<float, double>;
#else
using RealTypes = ::testing::Types<float, double, long double>;
#endif  // defined(__EMSCRIPTEN__)
TYPED_TEST_CASE(ExponentialDistributionTypedTest, RealTypes);

TYPED_TEST(ExponentialDistributionTypedTest, SerializeTest) {
  using param_type =
      typename abel::exponential_distribution<TypeParam>::param_type;

  const TypeParam kParams[] = {
      // Cases around 1.
      1,                                           //
      std::nextafter(TypeParam(1), TypeParam(0)),  // 1 - epsilon
      std::nextafter(TypeParam(1), TypeParam(2)),  // 1 + epsilon
      // Typical cases.
      TypeParam(1e-8), TypeParam(1e-4), TypeParam(1), TypeParam(2),
      TypeParam(1e4), TypeParam(1e8), TypeParam(1e20), TypeParam(2.5),
      // Boundary cases.
      std::numeric_limits<TypeParam>::max(),
      std::numeric_limits<TypeParam>::epsilon(),
      std::nextafter(std::numeric_limits<TypeParam>::min(),
                     TypeParam(1)),           // min + epsilon
      std::numeric_limits<TypeParam>::min(),  // smallest normal
      // There are some errors dealing with denorms on apple platforms.
      std::numeric_limits<TypeParam>::denorm_min(),  // smallest denorm
      std::numeric_limits<TypeParam>::min() / 2,     // denorm
      std::nextafter(std::numeric_limits<TypeParam>::min(),
                     TypeParam(0)),  // denorm_max
  };

  constexpr int kCount = 1000;
  abel::InsecureBitGen gen;

  for (const TypeParam lambda : kParams) {
    // Some values may be invalid; skip those.
    if (!std::isfinite(lambda)) continue;
    ABEL_ASSERT(lambda > 0);

    const param_type param(lambda);

    abel::exponential_distribution<TypeParam> before(lambda);
    EXPECT_EQ(before.lambda(), param.lambda());

    {
      abel::exponential_distribution<TypeParam> via_param(param);
      EXPECT_EQ(via_param, before);
      EXPECT_EQ(via_param.param(), before.param());
    }

    // Smoke test.
    auto sample_min = before.max();
    auto sample_max = before.min();
    for (int i = 0; i < kCount; i++) {
      auto sample = before(gen);
      EXPECT_GE(sample, before.min()) << before;
      EXPECT_LE(sample, before.max()) << before;
      if (sample > sample_max) sample_max = sample;
      if (sample < sample_min) sample_min = sample;
    }
    if (!std::is_same<TypeParam, long double>::value) {
      ABEL_INTERNAL_LOG(INFO,
                        abel::string_format("Range {%f}: %f, %f, lambda=%f", lambda,
                                        sample_min, sample_max, lambda));
    }

    std::stringstream ss;
    ss << before;

    if (!std::isfinite(lambda)) {
      // Streams do not deserialize inf/nan correctly.
      continue;
    }
    // Validate stream serialization.
    abel::exponential_distribution<TypeParam> after(34.56f);

    EXPECT_NE(before.lambda(), after.lambda());
    EXPECT_NE(before.param(), after.param());
    EXPECT_NE(before, after);

    ss >> after;

#if defined(__powerpc64__) || defined(__PPC64__) || defined(__powerpc__) || \
    defined(__ppc__) || defined(__PPC__)
    if (std::is_same<TypeParam, long double>::value) {
      // Roundtripping floating point values requires sufficient precision to
      // reconstruct the exact value. It turns out that long double has some
      // errors doing this on ppc, particularly for values
      // near {1.0 +/- epsilon}.
      if (lambda <= std::numeric_limits<double>::max() &&
          lambda >= std::numeric_limits<double>::lowest()) {
        EXPECT_EQ(static_cast<double>(before.lambda()),
                  static_cast<double>(after.lambda()))
            << ss.str();
      }
      continue;
    }
#endif

    EXPECT_EQ(before.lambda(), after.lambda())  //
        << ss.str() << " "                      //
        << (ss.good() ? "good " : "")           //
        << (ss.bad() ? "bad " : "")             //
        << (ss.eof() ? "eof " : "")             //
        << (ss.fail() ? "fail " : "");
  }
}

// http://www.itl.nist.gov/div898/handbook/eda/section3/eda3667.htm

class ExponentialModel {
 public:
  explicit ExponentialModel(double lambda)
      : lambda_(lambda), beta_(1.0 / lambda) {}

  double lambda() const { return lambda_; }

  double mean() const { return beta_; }
  double variance() const { return beta_ * beta_; }
  double stddev() const { return std::sqrt(variance()); }
  double skew() const { return 2; }
  double kurtosis() const { return 6.0; }

  double CDF(double x) { return 1.0 - std::exp(-lambda_ * x); }

  // The inverse CDF, or PercentPoint function of the distribution
  double InverseCDF(double p) {
    ABEL_ASSERT(p >= 0.0);
    ABEL_ASSERT(p < 1.0);
    return -beta_ * std::log(1.0 - p);
  }

 private:
  const double lambda_;
  const double beta_;
};

struct Param {
  double lambda;
  double p_fail;
  int trials;
};

class ExponentialDistributionTests : public testing::TestWithParam<Param>,
                                     public ExponentialModel {
 public:
  ExponentialDistributionTests() : ExponentialModel(GetParam().lambda) {}

  // SingleZTest provides a basic z-squared test of the mean vs. expected
  // mean for data generated by the poisson distribution.
  template <typename D>
  bool SingleZTest(const double p, const size_t samples);

  // SingleChiSquaredTest provides a basic chi-squared test of the normal
  // distribution.
  template <typename D>
  double SingleChiSquaredTest();

  abel::InsecureBitGen rng_;
};

template <typename D>
bool ExponentialDistributionTests::SingleZTest(const double p,
                                               const size_t samples) {
  D dis(lambda());

  std::vector<double> data;
  data.reserve(samples);
  for (size_t i = 0; i < samples; i++) {
    const double x = dis(rng_);
    data.push_back(x);
  }

  const auto m = abel::random_internal::ComputeDistributionMoments(data);
  const double max_err = abel::random_internal::MaxErrorTolerance(p);
  const double z = abel::random_internal::ZScore(mean(), m);
  const bool pass = abel::random_internal::Near("z", z, 0.0, max_err);

  if (!pass) {
    ABEL_INTERNAL_LOG(
        INFO, abel::string_format("p=%f max_err=%f\n"
                              " lambda=%f\n"
                              " mean=%f vs. %f\n"
                              " stddev=%f vs. %f\n"
                              " skewness=%f vs. %f\n"
                              " kurtosis=%f vs. %f\n"
                              " z=%f vs. 0",
                              p, max_err, lambda(), m.mean, mean(),
                              std::sqrt(m.variance), stddev(), m.skewness,
                              skew(), m.kurtosis, kurtosis(), z));
  }
  return pass;
}

template <typename D>
double ExponentialDistributionTests::SingleChiSquaredTest() {
  const size_t kSamples = 10000;
  const int kBuckets = 50;

  // The InverseCDF is the percent point function of the distribution, and can
  // be used to assign buckets roughly uniformly.
  std::vector<double> cutoffs;
  const double kInc = 1.0 / static_cast<double>(kBuckets);
  for (double p = kInc; p < 1.0; p += kInc) {
    cutoffs.push_back(InverseCDF(p));
  }
  if (cutoffs.back() != std::numeric_limits<double>::infinity()) {
    cutoffs.push_back(std::numeric_limits<double>::infinity());
  }

  D dis(lambda());

  std::vector<int32_t> counts(cutoffs.size(), 0);
  for (size_t j = 0; j < kSamples; j++) {
    const double x = dis(rng_);
    auto it = std::upper_bound(cutoffs.begin(), cutoffs.end(), x);
    counts[std::distance(cutoffs.begin(), it)]++;
  }

  // Null-hypothesis is that the distribution is exponentially distributed
  // with the provided lambda (not estimated from the data).
  const int dof = static_cast<int>(counts.size()) - 1;

  // Our threshold for logging is 1-in-50.
  const double threshold = abel::random_internal::ChiSquareValue(dof, 0.98);

  const double expected =
      static_cast<double>(kSamples) / static_cast<double>(counts.size());

  double chi_square = abel::random_internal::ChiSquareWithExpected(
      std::begin(counts), std::end(counts), expected);
  double p = abel::random_internal::ChiSquarePValue(chi_square, dof);

  if (chi_square > threshold) {
    for (size_t i = 0; i < cutoffs.size(); i++) {
      ABEL_INTERNAL_LOG(
          INFO, abel::string_format("%d : (%f) = %d", i, cutoffs[i], counts[i]));
    }

    ABEL_INTERNAL_LOG(INFO,
                      abel::string_cat("lambda ", lambda(), "\n",     //
                                   " expected ", expected, "\n",  //
                                   kChiSquared, " ", chi_square, " (", p, ")\n",
                                   kChiSquared, " @ 0.98 = ", threshold));
  }
  return p;
}

TEST_P(ExponentialDistributionTests, ZTest) {
  const size_t kSamples = 10000;
  const auto& param = GetParam();
  const int expected_failures =
      std::max(1, static_cast<int>(std::ceil(param.trials * param.p_fail)));
  const double p = abel::random_internal::RequiredSuccessProbability(
      param.p_fail, param.trials);

  int failures = 0;
  for (int i = 0; i < param.trials; i++) {
    failures += SingleZTest<abel::exponential_distribution<double>>(p, kSamples)
                    ? 0
                    : 1;
  }
  EXPECT_LE(failures, expected_failures);
}

TEST_P(ExponentialDistributionTests, ChiSquaredTest) {
  const int kTrials = 20;
  int failures = 0;

  for (int i = 0; i < kTrials; i++) {
    double p_value =
        SingleChiSquaredTest<abel::exponential_distribution<double>>();
    if (p_value < 0.005) {  // 1/200
      failures++;
    }
  }

  // There is a 0.10% chance of producing at least one failure, so raise the
  // failure threshold high enough to allow for a flake rate < 10,000.
  EXPECT_LE(failures, 4);
}

std::vector<Param> GenParams() {
  return {
      Param{1.0, 0.02, 100},
      Param{2.5, 0.02, 100},
      Param{10, 0.02, 100},
      // large
      Param{1e4, 0.02, 100},
      Param{1e9, 0.02, 100},
      // small
      Param{0.1, 0.02, 100},
      Param{1e-3, 0.02, 100},
      Param{1e-5, 0.02, 100},
  };
}

std::string ParamName(const ::testing::TestParamInfo<Param>& info) {
  const auto& p = info.param;
  std::string name = abel::string_cat("lambda_", abel::SixDigits(p.lambda));
  return abel::string_replace_all(name, {{"+", "_"}, {"-", "_"}, {".", "_"}});
}

INSTANTIATE_TEST_CASE_P(All, ExponentialDistributionTests,
                        ::testing::ValuesIn(GenParams()), ParamName);

// NOTE: abel::exponential_distribution is not guaranteed to be stable.
TEST(ExponentialDistributionTest, StabilityTest) {
  // abel::exponential_distribution stability relies on std::log1p and
  // abel::uniform_real_distribution.
  abel::random_internal::sequence_urbg urbg(
      {0x0003eb76f6f7f755ull, 0xFFCEA50FDB2F953Bull, 0xC332DDEFBE6C5AA5ull,
       0x6558218568AB9702ull, 0x2AEF7DAD5B6E2F84ull, 0x1521B62829076170ull,
       0xECDD4775619F1510ull, 0x13CCA830EB61BD96ull, 0x0334FE1EAA0363CFull,
       0xB5735C904C70A239ull, 0xD59E9E0BCBAADE14ull, 0xEECC86BC60622CA7ull});

  std::vector<int> output(14);

  {
    abel::exponential_distribution<double> dist;
    std::generate(std::begin(output), std::end(output),
                  [&] { return static_cast<int>(10000.0 * dist(urbg)); });

    EXPECT_EQ(14, urbg.invocations());
    EXPECT_THAT(output,
                testing::ElementsAre(0, 71913, 14375, 5039, 1835, 861, 25936,
                                     804, 126, 12337, 17984, 27002, 0, 71913));
  }

  urbg.reset();
  {
    abel::exponential_distribution<float> dist;
    std::generate(std::begin(output), std::end(output),
                  [&] { return static_cast<int>(10000.0f * dist(urbg)); });

    EXPECT_EQ(14, urbg.invocations());
    EXPECT_THAT(output,
                testing::ElementsAre(0, 71913, 14375, 5039, 1835, 861, 25936,
                                     804, 126, 12337, 17984, 27002, 0, 71913));
  }
}

TEST(ExponentialDistributionTest, AlgorithmBounds) {
  // Relies on abel::uniform_real_distribution, so some of these comments
  // reference that.
  abel::exponential_distribution<double> dist;

  {
    // This returns the smallest value >0 from abel::uniform_real_distribution.
    abel::random_internal::sequence_urbg urbg({0x0000000000000001ull});
    double a = dist(urbg);
    EXPECT_EQ(a, 5.42101086242752217004e-20);
  }

  {
    // This returns a value very near 0.5 from abel::uniform_real_distribution.
    abel::random_internal::sequence_urbg urbg({0x7fffffffffffffefull});
    double a = dist(urbg);
    EXPECT_EQ(a, 0.693147180559945175204);
  }

  {
    // This returns the largest value <1 from abel::uniform_real_distribution.
    // WolframAlpha: ~39.1439465808987766283058547296341915292187253
    abel::random_internal::sequence_urbg urbg({0xFFFFFFFFFFFFFFeFull});
    double a = dist(urbg);
    EXPECT_EQ(a, 36.7368005696771007251);
  }
  {
    // This *ALSO* returns the largest value <1.
    abel::random_internal::sequence_urbg urbg({0xFFFFFFFFFFFFFFFFull});
    double a = dist(urbg);
    EXPECT_EQ(a, 36.7368005696771007251);
  }
}

}  // namespace
