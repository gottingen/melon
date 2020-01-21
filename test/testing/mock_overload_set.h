

#ifndef ABEL_TESTING_INTERNAL_MOCK_OVERLOAD_SET_H_
#define ABEL_TESTING_INTERNAL_MOCK_OVERLOAD_SET_H_

#include <type_traits>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testing/mocking_bit_gen.h>

namespace abel {

namespace random_internal {

template <typename DistrT, typename Fn>
struct MockSingleOverload;

// MockSingleOverload
//
// MockSingleOverload hooks in to gMock's `ON_CALL` and `EXPECT_CALL` macros.
// EXPECT_CALL(mock_single_overload, Call(...))` will expand to a call to
// `mock_single_overload.gmock_Call(...)`. Because expectations are stored on
// the MockingBitGen (an argument passed inside `Call(...)`), this forwards to
// arguments to Mocking::Register.
template <typename DistrT, typename Ret, typename... Args>
struct MockSingleOverload<DistrT, Ret(MockingBitGen&, Args...)> {
  static_assert(std::is_same<typename DistrT::result_type, Ret>::value,
                "Overload signature must have return type matching the "
                "distributions result type.");
  auto gmock_Call(
      abel::MockingBitGen& gen,  // NOLINT(google-runtime-references)
      const ::testing::Matcher<Args>&... args)
      -> decltype(gen.Register<DistrT, Args...>(args...)) {
    return gen.Register<DistrT, Args...>(args...);
  }
};

template <typename DistrT, typename Ret, typename Arg, typename... Args>
struct MockSingleOverload<DistrT, Ret(Arg, MockingBitGen&, Args...)> {
  static_assert(std::is_same<typename DistrT::result_type, Ret>::value,
                "Overload signature must have return type matching the "
                "distributions result type.");
  auto gmock_Call(
      const ::testing::Matcher<Arg>& arg,
      abel::MockingBitGen& gen,  // NOLINT(google-runtime-references)
      const ::testing::Matcher<Args>&... args)
      -> decltype(gen.Register<DistrT, Arg, Args...>(arg, args...)) {
    return gen.Register<DistrT, Arg, Args...>(arg, args...);
  }
};

// MockOverloadSet
//
// MockOverloadSet takes a distribution and a collection of signatures and
// performs overload resolution amongst all the overloads. This makes
// `EXPECT_CALL(mock_overload_set, Call(...))` expand and do overload resolution
// correctly.
template <typename DistrT, typename... Signatures>
struct MockOverloadSet;

template <typename DistrT, typename Sig>
struct MockOverloadSet<DistrT, Sig> : public MockSingleOverload<DistrT, Sig> {
  using MockSingleOverload<DistrT, Sig>::gmock_Call;
};

template <typename DistrT, typename FirstSig, typename... Rest>
struct MockOverloadSet<DistrT, FirstSig, Rest...>
    : public MockSingleOverload<DistrT, FirstSig>,
      public MockOverloadSet<DistrT, Rest...> {
  using MockSingleOverload<DistrT, FirstSig>::gmock_Call;
  using MockOverloadSet<DistrT, Rest...>::gmock_Call;
};

}  // namespace random_internal

}  // namespace abel
#endif  // ABEL_TESTING_INTERNAL_MOCK_OVERLOAD_SET_H_
