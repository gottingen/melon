//
//
#include <testing/bit_gen_ref.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/random/internal/sequence_urbg.h>
#include <abel/random/random.h>

namespace abel {


class ConstBitGen : public abel::random_internal::MockingBitGenBase {
  bool CallImpl(const std::type_info&, void*, void* result) override {
    *static_cast<int*>(result) = 42;
    return true;
  }
};

namespace random_internal {
template <>
struct DistributionCaller<ConstBitGen> {
  template <typename DistrT, typename FormatT, typename... Args>
  static typename DistrT::result_type Call(ConstBitGen* gen, Args&&... args) {
    return gen->template Call<DistrT, FormatT>(std::forward<Args>(args)...);
  }
};
}  // namespace random_internal

namespace {
int FnTest(abel::BitGenRef gen_ref) { return abel::Uniform(gen_ref, 1, 7); }

template <typename T>
class BitGenRefTest : public testing::Test {};

using BitGenTypes =
    ::testing::Types<abel::BitGen, abel::InsecureBitGen, std::mt19937,
                     std::mt19937_64, std::minstd_rand>;
TYPED_TEST_SUITE(BitGenRefTest, BitGenTypes);

TYPED_TEST(BitGenRefTest, BasicTest) {
  TypeParam gen;
  auto x = FnTest(gen);
  EXPECT_NEAR(x, 4, 3);
}

TYPED_TEST(BitGenRefTest, Copyable) {
  TypeParam gen;
  abel::BitGenRef gen_ref(gen);
  FnTest(gen_ref);  // Copy
}

TEST(BitGenRefTest, PassThroughEquivalence) {
  // sequence_urbg returns 64-bit results.
  abel::random_internal::sequence_urbg urbg(
      {0x0003eb76f6f7f755ull, 0xFFCEA50FDB2F953Bull, 0xC332DDEFBE6C5AA5ull,
       0x6558218568AB9702ull, 0x2AEF7DAD5B6E2F84ull, 0x1521B62829076170ull,
       0xECDD4775619F1510ull, 0x13CCA830EB61BD96ull, 0x0334FE1EAA0363CFull,
       0xB5735C904C70A239ull, 0xD59E9E0BCBAADE14ull, 0xEECC86BC60622CA7ull});

  std::vector<uint64_t> output(12);

  {
    abel::BitGenRef view(urbg);
    for (auto& v : output) {
      v = view();
    }
  }

  std::vector<uint64_t> expected(
      {0x0003eb76f6f7f755ull, 0xFFCEA50FDB2F953Bull, 0xC332DDEFBE6C5AA5ull,
       0x6558218568AB9702ull, 0x2AEF7DAD5B6E2F84ull, 0x1521B62829076170ull,
       0xECDD4775619F1510ull, 0x13CCA830EB61BD96ull, 0x0334FE1EAA0363CFull,
       0xB5735C904C70A239ull, 0xD59E9E0BCBAADE14ull, 0xEECC86BC60622CA7ull});

  EXPECT_THAT(output, testing::Eq(expected));
}

TEST(BitGenRefTest, MockingBitGenBaseOverrides) {
  ConstBitGen const_gen;
  EXPECT_EQ(FnTest(const_gen), 42);

  abel::BitGenRef gen_ref(const_gen);
  EXPECT_EQ(FnTest(gen_ref), 42);  // Copy
}
}  // namespace

}  // namespace abel
