//

#include <abel/memory/memory.h>

#include <abel/base/profile.h>

#ifdef ABEL_HAVE_EXCEPTIONS

#include <gtest/gtest.h>
#include <testing/exception_safety_testing.h>

namespace abel {

namespace {

constexpr int kLength = 50;
using Thrower = testing::ThrowingValue<testing::TypeSpec::kEverythingThrows>;
using ThrowerStorage =
    abel::aligned_storage_t<sizeof(Thrower), alignof(Thrower)>;
using ThrowerList = std::array<ThrowerStorage, kLength>;

TEST(MakeUnique, CheckForLeaks) {
  constexpr int kValue = 321;
  auto tester = testing::MakeExceptionSafetyTester()
                    .WithInitialValue(Thrower(kValue))
                    // Ensures make_unique does not modify the input. The real
                    // test, though, is ConstructorTracker checking for leaks.
                    .WithContracts(testing::strong_guarantee);

  EXPECT_TRUE(tester.Test([](Thrower* thrower) {
    static_cast<void>(abel::make_unique<Thrower>(*thrower));
  }));

  EXPECT_TRUE(tester.Test([](Thrower* thrower) {
    static_cast<void>(abel::make_unique<Thrower>(std::move(*thrower)));
  }));

  // Test T[n] overload
  EXPECT_TRUE(tester.Test([&](Thrower*) {
    static_cast<void>(abel::make_unique<Thrower[]>(kLength));
  }));
}

}  // namespace

}  // namespace abel

#endif  // ABEL_HAVE_EXCEPTIONS
