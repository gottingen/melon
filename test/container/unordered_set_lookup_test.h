//

#ifndef ABEL_CONTAINER_INTERNAL_UNORDERED_SET_LOOKUP_TEST_H_
#define ABEL_CONTAINER_INTERNAL_UNORDERED_SET_LOOKUP_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testing/hash_generator_testing.h>
#include <test/container/hash_policy_testing.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace container_internal {

template <class UnordSet>
class LookupTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(LookupTest);

TYPED_TEST_P(LookupTest, Count) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& v : values)
    EXPECT_EQ(0, m.count(v)) << ::testing::PrintToString(v);
  m.insert(values.begin(), values.end());
  for (const auto& v : values)
    EXPECT_EQ(1, m.count(v)) << ::testing::PrintToString(v);
}

TYPED_TEST_P(LookupTest, Find) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& v : values)
    EXPECT_TRUE(m.end() == m.find(v)) << ::testing::PrintToString(v);
  m.insert(values.begin(), values.end());
  for (const auto& v : values) {
    typename TypeParam::iterator it = m.find(v);
    static_assert(std::is_same<const typename TypeParam::value_type&,
                               decltype(*it)>::value,
                  "");
    static_assert(std::is_same<const typename TypeParam::value_type*,
                               decltype(it.operator->())>::value,
                  "");
    EXPECT_TRUE(m.end() != it) << ::testing::PrintToString(v);
    EXPECT_EQ(v, *it) << ::testing::PrintToString(v);
  }
}

TYPED_TEST_P(LookupTest, EqualRange) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& v : values) {
    auto r = m.equal_range(v);
    ASSERT_EQ(0, std::distance(r.first, r.second));
  }
  m.insert(values.begin(), values.end());
  for (const auto& v : values) {
    auto r = m.equal_range(v);
    ASSERT_EQ(1, std::distance(r.first, r.second));
    EXPECT_EQ(v, *r.first);
  }
}

REGISTER_TYPED_TEST_SUITE_P(LookupTest, Count, Find, EqualRange);

}  // namespace container_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_CONTAINER_INTERNAL_UNORDERED_SET_LOOKUP_TEST_H_
