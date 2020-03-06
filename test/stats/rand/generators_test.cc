//

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>
#include <abel/stats/random/distributions.h>
#include <abel/stats/random/random.h>

namespace {

    template<typename URBG>
    void TestUniform(URBG *gen) {
        // [a, b) default-semantics, inferred types.
        abel::uniform(*gen, 0, 100);     // int
        abel::uniform(*gen, 0, 1.0);     // Promoted to double
        abel::uniform(*gen, 0.0f, 1.0);  // Promoted to double
        abel::uniform(*gen, 0.0, 1.0);   // double
        abel::uniform(*gen, -1, 1L);     // Promoted to long

        // Roll a die.
        abel::uniform(abel::IntervalClosedClosed, *gen, 1, 6);

        // Get a fraction.
        abel::uniform(abel::IntervalOpenOpen, *gen, 0.0, 1.0);

        // Assign a value to a random element.
        std::vector<int> elems = {10, 20, 30, 40, 50};
        elems[abel::uniform(*gen, 0u, elems.size())] = 5;
        elems[abel::uniform<size_t>(*gen, 0, elems.size())] = 3;

        // Choose some epsilon around zero.
        abel::uniform(abel::IntervalOpenOpen, *gen, -1.0, 1.0);

        // (a, b) semantics, inferred types.
        abel::uniform(abel::IntervalOpenOpen, *gen, 0, 1.0);  // Promoted to double

        // Explict overriding of types.
        abel::uniform<int>(*gen, 0, 100);
        abel::uniform<int8_t>(*gen, 0, 100);
        abel::uniform<int16_t>(*gen, 0, 100);
        abel::uniform<uint16_t>(*gen, 0, 100);
        abel::uniform<int32_t>(*gen, 0, 1 << 10);
        abel::uniform<uint32_t>(*gen, 0, 1 << 10);
        abel::uniform<int64_t>(*gen, 0, 1 << 10);
        abel::uniform<uint64_t>(*gen, 0, 1 << 10);

        abel::uniform<float>(*gen, 0.0, 1.0);
        abel::uniform<float>(*gen, 0, 1);
        abel::uniform<float>(*gen, -1, 1);
        abel::uniform<double>(*gen, 0.0, 1.0);

        abel::uniform<float>(*gen, -1.0, 0);
        abel::uniform<double>(*gen, -1.0, 0);

        // Tagged
        abel::uniform<double>(abel::IntervalClosedClosed, *gen, 0, 1);
        abel::uniform<double>(abel::IntervalClosedOpen, *gen, 0, 1);
        abel::uniform<double>(abel::IntervalOpenOpen, *gen, 0, 1);
        abel::uniform<double>(abel::IntervalOpenClosed, *gen, 0, 1);
        abel::uniform<double>(abel::IntervalClosedClosed, *gen, 0, 1);
        abel::uniform<double>(abel::IntervalOpenOpen, *gen, 0, 1);

        abel::uniform<int>(abel::IntervalClosedClosed, *gen, 0, 100);
        abel::uniform<int>(abel::IntervalClosedOpen, *gen, 0, 100);
        abel::uniform<int>(abel::IntervalOpenOpen, *gen, 0, 100);
        abel::uniform<int>(abel::IntervalOpenClosed, *gen, 0, 100);
        abel::uniform<int>(abel::IntervalClosedClosed, *gen, 0, 100);
        abel::uniform<int>(abel::IntervalOpenOpen, *gen, 0, 100);

        // With *generator as an R-value reference.
        abel::uniform<int>(URBG(), 0, 100);
        abel::uniform<double>(URBG(), 0.0, 1.0);
    }

    template<typename URBG>
    void TestExponential(URBG *gen) {
        abel::Exponential<float>(*gen);
        abel::Exponential<double>(*gen);
        abel::Exponential<double>(URBG());
    }

    template<typename URBG>
    void TestPoisson(URBG *gen) {
        // [rand.dist.pois] Indicates that the std::poisson_distribution
        // is parameterized by IntType, however MSVC does not allow 8-bit
        // types.
        abel::Poisson<int>(*gen);
        abel::Poisson<int16_t>(*gen);
        abel::Poisson<uint16_t>(*gen);
        abel::Poisson<int32_t>(*gen);
        abel::Poisson<uint32_t>(*gen);
        abel::Poisson<int64_t>(*gen);
        abel::Poisson<uint64_t>(*gen);
        abel::Poisson<uint64_t>(URBG());
    }

    template<typename URBG>
    void TestBernoulli(URBG *gen) {
        abel::Bernoulli(*gen, 0.5);
        abel::Bernoulli(*gen, 0.5);
    }

    template<typename URBG>
    void TestZipf(URBG *gen) {
        abel::Zipf<int>(*gen, 100);
        abel::Zipf<int8_t>(*gen, 100);
        abel::Zipf<int16_t>(*gen, 100);
        abel::Zipf<uint16_t>(*gen, 100);
        abel::Zipf<int32_t>(*gen, 1 << 10);
        abel::Zipf<uint32_t>(*gen, 1 << 10);
        abel::Zipf<int64_t>(*gen, 1 << 10);
        abel::Zipf<uint64_t>(*gen, 1 << 10);
        abel::Zipf<uint64_t>(URBG(), 1 << 10);
    }

    template<typename URBG>
    void TestGaussian(URBG *gen) {
        abel::Gaussian<float>(*gen, 1.0, 1.0);
        abel::Gaussian<double>(*gen, 1.0, 1.0);
        abel::Gaussian<double>(URBG(), 1.0, 1.0);
    }

    template<typename URBG>
    void TestLogNormal(URBG *gen) {
        abel::LogUniform<int>(*gen, 0, 100);
        abel::LogUniform<int8_t>(*gen, 0, 100);
        abel::LogUniform<int16_t>(*gen, 0, 100);
        abel::LogUniform<uint16_t>(*gen, 0, 100);
        abel::LogUniform<int32_t>(*gen, 0, 1 << 10);
        abel::LogUniform<uint32_t>(*gen, 0, 1 << 10);
        abel::LogUniform<int64_t>(*gen, 0, 1 << 10);
        abel::LogUniform<uint64_t>(*gen, 0, 1 << 10);
        abel::LogUniform<uint64_t>(URBG(), 0, 1 << 10);
    }

    template<typename URBG>
    void CompatibilityTest() {
        URBG gen;

        TestUniform(&gen);
        TestExponential(&gen);
        TestPoisson(&gen);
        TestBernoulli(&gen);
        TestZipf(&gen);
        TestGaussian(&gen);
        TestLogNormal(&gen);
    }

    TEST(std_mt19937_64, Compatibility) {
        // Validate with std::mt19937_64
        CompatibilityTest<std::mt19937_64>();
    }

    TEST(bit_gen, Compatibility) {
        // Validate with abel::bit_gen
        CompatibilityTest<abel::bit_gen>();
    }

    TEST(insecure_bit_gen, Compatibility) {
        // Validate with abel::insecure_bit_gen
        CompatibilityTest<abel::insecure_bit_gen>();
    }

}  // namespace
