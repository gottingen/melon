//

#include <abel/types/any.h>

#include <abel/base/profile.h>

// This test is a no-op when abel::any is an alias for std::any and when
// exceptions are not enabled.
#if !defined(ABEL_USES_STD_ANY) && defined(ABEL_HAVE_EXCEPTIONS)

#include <typeinfo>
#include <vector>

#include <gtest/gtest.h>
#include <testing/exception_safety_testing.h>

using Thrower = testing::ThrowingValue<>;
using NoThrowMoveThrower =
testing::ThrowingValue<testing::TypeSpec::kNoThrowMove>;
using ThrowerList = std::initializer_list<Thrower>;
using ThrowerVec = std::vector<Thrower>;
using ThrowingAlloc = testing::ThrowingAllocator<Thrower>;
using ThrowingThrowerVec = std::vector<Thrower, ThrowingAlloc>;

namespace {

    testing::AssertionResult AnyInvariants(abel::any *a) {
        using testing::AssertionFailure;
        using testing::AssertionSuccess;

        if (a->has_value()) {
            if (a->type() == typeid(void)) {
                return AssertionFailure()
                        << "A non-empty any should not have type `void`";
            }
        } else {
            if (a->type() != typeid(void)) {
                return AssertionFailure()
                        << "An empty any should have type void, but has type "
                        << a->type().name();
            }
        }

        //  Make sure that reset() changes any to a valid state.
        a->reset();
        if (a->has_value()) {
            return AssertionFailure() << "A reset `any` should be valueless";
        }
        if (a->type() != typeid(void)) {
            return AssertionFailure() << "A reset `any` should have type() of `void`, "
                                         "but instead has type "
                                      << a->type().name();
        }
        try {
            auto unused = abel::any_cast<Thrower>(*a);
            static_cast<void>(unused);
            return AssertionFailure()
                    << "A reset `any` should not be able to be any_cast";
        } catch (const abel::bad_any_cast &) {
        } catch (...) {
            return AssertionFailure()
                    << "Unexpected exception thrown from abel::any_cast";
        }
        return AssertionSuccess();
    }

    testing::AssertionResult AnyIsEmpty(abel::any *a) {
        if (!a->has_value()) {
            return testing::AssertionSuccess();
        }
        return testing::AssertionFailure()
                << "a should be empty, but instead has value "
                << abel::any_cast<Thrower>(*a).Get();
    }

    TEST(AnyExceptionSafety, Ctors) {
        Thrower val(1);
        testing::TestThrowingCtor<abel::any>(val);

        Thrower copy(val);
        testing::TestThrowingCtor<abel::any>(copy);

        testing::TestThrowingCtor<abel::any>(abel::in_place_type_t<Thrower>(), 1);

        testing::TestThrowingCtor<abel::any>(abel::in_place_type_t<ThrowerVec>(),
                                             ThrowerList{val});

        testing::TestThrowingCtor<abel::any,
                abel::in_place_type_t<ThrowingThrowerVec>,
                ThrowerList, ThrowingAlloc>(
                abel::in_place_type_t<ThrowingThrowerVec>(), {val}, ThrowingAlloc());
    }

    TEST(AnyExceptionSafety, Assignment) {
        auto original =
                abel::any(abel::in_place_type_t<Thrower>(), 1, testing::nothrow_ctor);
        auto any_is_strong = [original](abel::any *ap) {
            return testing::AssertionResult(ap->has_value() &&
                                            abel::any_cast<Thrower>(original) ==
                                            abel::any_cast<Thrower>(*ap));
        };
        auto any_strong_tester = testing::MakeExceptionSafetyTester()
                .WithInitialValue(original)
                .WithContracts(AnyInvariants, any_is_strong);

        Thrower val(2);
        abel::any any_val(val);
        NoThrowMoveThrower mv_val(2);

        auto assign_any = [&any_val](abel::any *ap) { *ap = any_val; };
        auto assign_val = [&val](abel::any *ap) { *ap = val; };
        auto move = [&val](abel::any *ap) { *ap = std::move(val); };
        auto move_movable = [&mv_val](abel::any *ap) { *ap = std::move(mv_val); };

        EXPECT_TRUE(any_strong_tester.Test(assign_any));
        EXPECT_TRUE(any_strong_tester.Test(assign_val));
        EXPECT_TRUE(any_strong_tester.Test(move));
        EXPECT_TRUE(any_strong_tester.Test(move_movable));

        auto empty_any_is_strong = [](abel::any *ap) {
            return testing::AssertionResult{!ap->has_value()};
        };
        auto strong_empty_any_tester =
                testing::MakeExceptionSafetyTester()
                        .WithInitialValue(abel::any{})
                        .WithContracts(AnyInvariants, empty_any_is_strong);

        EXPECT_TRUE(strong_empty_any_tester.Test(assign_any));
        EXPECT_TRUE(strong_empty_any_tester.Test(assign_val));
        EXPECT_TRUE(strong_empty_any_tester.Test(move));
    }

    TEST(AnyExceptionSafety, Emplace) {
        auto initial_val =
                abel::any{abel::in_place_type_t<Thrower>(), 1, testing::nothrow_ctor};
        auto one_tester = testing::MakeExceptionSafetyTester()
                .WithInitialValue(initial_val)
                .WithContracts(AnyInvariants, AnyIsEmpty);

        auto emp_thrower = [](abel::any *ap) { ap->emplace<Thrower>(2); };
        auto emp_throwervec = [](abel::any *ap) {
            std::initializer_list<Thrower> il{Thrower(2, testing::nothrow_ctor)};
            ap->emplace<ThrowerVec>(il);
        };
        auto emp_movethrower = [](abel::any *ap) {
            ap->emplace<NoThrowMoveThrower>(2);
        };

        EXPECT_TRUE(one_tester.Test(emp_thrower));
        EXPECT_TRUE(one_tester.Test(emp_throwervec));
        EXPECT_TRUE(one_tester.Test(emp_movethrower));

        auto empty_tester = one_tester.WithInitialValue(abel::any{});

        EXPECT_TRUE(empty_tester.Test(emp_thrower));
        EXPECT_TRUE(empty_tester.Test(emp_throwervec));
    }

}  // namespace

#endif  // #if !defined(ABEL_USES_STD_ANY) && defined(ABEL_HAVE_EXCEPTIONS)
