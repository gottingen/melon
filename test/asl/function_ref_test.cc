//

#include <abel/asl/functional.h>
#include <memory>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testing/test_instance_tracker.h>
#include <abel/memory/memory.h>

namespace abel {

    namespace {

        void RunFun(FunctionRef<void()> f) { f(); }

        TEST(FunctionRefTest, Lambda) {
            bool ran = false;
            RunFun([&] { ran = true; });
            EXPECT_TRUE(ran);
        }

        int Function() { return 1337; }

        TEST(FunctionRefTest, Function1) {
            FunctionRef<int()> ref(&Function);
            EXPECT_EQ(1337, ref());
        }

        TEST(FunctionRefTest, Function2) {
            FunctionRef<int()> ref(Function);
            EXPECT_EQ(1337, ref());
        }

        int NoExceptFunction() noexcept { return 1337; }

// TODO(jdennett): Add a test for noexcept member functions.
        TEST(FunctionRefTest, NoExceptFunction) {
            FunctionRef<int()> ref(NoExceptFunction);
            EXPECT_EQ(1337, ref());
        }

        TEST(FunctionRefTest, ForwardsArgs) {
            auto l = [](std::unique_ptr<int> i) { return *i; };
            FunctionRef<int(std::unique_ptr<int>)> ref(l);
            EXPECT_EQ(42, ref(abel::make_unique<int>(42)));
        }

        TEST(FunctionRef, ReturnMoveOnly) {
            auto l = [] { return abel::make_unique<int>(29); };
            FunctionRef<std::unique_ptr<int>()> ref(l);
            EXPECT_EQ(29, *ref());
        }

        TEST(FunctionRef, ManyArgs) {
            auto l = [](int a, int b, int c) { return a + b + c; };
            FunctionRef<int(int, int, int)> ref(l);
            EXPECT_EQ(6, ref(1, 2, 3));
        }

        TEST(FunctionRef, VoidResultFromNonVoidFunctor) {
            bool ran = false;
            auto l = [&]() -> int {
                ran = true;
                return 2;
            };
            FunctionRef<void()> ref(l);
            ref();
            EXPECT_TRUE(ran);
        }

        TEST(FunctionRef, CastFromDerived) {
            struct Base {
            };
            struct Derived : public Base {
            };

            Derived d;
            auto l1 = [&](Base *b) { EXPECT_EQ(&d, b); };
            FunctionRef<void(Derived *)> ref1(l1);
            ref1(&d);

            auto l2 = [&]() -> Derived * { return &d; };
            FunctionRef<Base *()> ref2(l2);
            EXPECT_EQ(&d, ref2());
        }

        TEST(FunctionRef, VoidResultFromNonVoidFuncton) {
            FunctionRef<void()> ref(Function);
            ref();
        }

        TEST(FunctionRef, MemberPtr) {
            struct S {
                int i;
            };

            S s{1100111};
            auto mem_ptr = &S::i;
            FunctionRef<int(const S &s)> ref(mem_ptr);
            EXPECT_EQ(1100111, ref(s));
        }

        TEST(FunctionRef, MemberFun) {
            struct S {
                int i;

                int get_i() const { return i; }
            };

            S s{22};
            auto mem_fun_ptr = &S::get_i;
            FunctionRef<int(const S &s)> ref(mem_fun_ptr);
            EXPECT_EQ(22, ref(s));
        }

        TEST(FunctionRef, MemberFunRefqualified) {
            struct S {
                int i;

                int get_i() &&{ return i; }
            };
            auto mem_fun_ptr = &S::get_i;
            S s{22};
            FunctionRef<int(S &&s)> ref(mem_fun_ptr);
            EXPECT_EQ(22, ref(std::move(s)));
        }

#if !defined(_WIN32) && defined(GTEST_HAS_DEATH_TEST)

        TEST(FunctionRef, MemberFunRefqualifiedNull) {
            struct S {
                int i;

                int get_i() &&{ return i; }
            };
            auto mem_fun_ptr = &S::get_i;
            mem_fun_ptr = nullptr;
            EXPECT_DEBUG_DEATH({ FunctionRef<int(S &&s)> ref(mem_fun_ptr); }, "");
        }

        TEST(FunctionRef, NullMemberPtrAssertFails) {
            struct S {
                int i;
            };
            using MemberPtr = int S::*;
            MemberPtr mem_ptr = nullptr;
            EXPECT_DEBUG_DEATH({ FunctionRef<int(const S &s)> ref(mem_ptr); }, "");
        }

#endif  // GTEST_HAS_DEATH_TEST

        TEST(FunctionRef, CopiesAndMovesPerPassByValue) {
            abel::test_internal::InstanceTracker tracker;
            abel::test_internal::CopyableMovableInstance instance(0);
            auto l = [](abel::test_internal::CopyableMovableInstance) {};
            FunctionRef<void(abel::test_internal::CopyableMovableInstance)> ref(l);
            ref(instance);
            EXPECT_EQ(tracker.copies(), 1);
            EXPECT_EQ(tracker.moves(), 1);
        }

        TEST(FunctionRef, CopiesAndMovesPerPassByRef) {
            abel::test_internal::InstanceTracker tracker;
            abel::test_internal::CopyableMovableInstance instance(0);
            auto l = [](const abel::test_internal::CopyableMovableInstance &) {};
            FunctionRef<void(const abel::test_internal::CopyableMovableInstance &)> ref(l);
            ref(instance);
            EXPECT_EQ(tracker.copies(), 0);
            EXPECT_EQ(tracker.moves(), 0);
        }

        TEST(FunctionRef, CopiesAndMovesPerPassByValueCallByMove) {
            abel::test_internal::InstanceTracker tracker;
            abel::test_internal::CopyableMovableInstance instance(0);
            auto l = [](abel::test_internal::CopyableMovableInstance) {};
            FunctionRef<void(abel::test_internal::CopyableMovableInstance)> ref(l);
            ref(std::move(instance));
            EXPECT_EQ(tracker.copies(), 0);
            EXPECT_EQ(tracker.moves(), 2);
        }

        TEST(FunctionRef, CopiesAndMovesPerPassByValueToRef) {
            abel::test_internal::InstanceTracker tracker;
            abel::test_internal::CopyableMovableInstance instance(0);
            auto l = [](const abel::test_internal::CopyableMovableInstance &) {};
            FunctionRef<void(abel::test_internal::CopyableMovableInstance)> ref(l);
            ref(std::move(instance));
            EXPECT_EQ(tracker.copies(), 0);
            EXPECT_EQ(tracker.moves(), 1);
        }

        TEST(FunctionRef, PassByValueTypes) {
            using abel::functional_internal::Invoker;
            using abel::functional_internal::VoidPtr;
            using abel::test_internal::CopyableMovableInstance;
            struct Trivial {
                void *p[2];
            };
            struct LargeTrivial {
                void *p[3];
            };

            static_assert(std::is_same<Invoker<void, int>, void (*)(VoidPtr, int)>::value,
                          "Scalar types should be passed by value");
            static_assert(
                    std::is_same<Invoker<void, Trivial>, void (*)(VoidPtr, Trivial)>::value,
                    "Small trivial types should be passed by value");
            static_assert(std::is_same<Invoker<void, LargeTrivial>,
                                  void (*)(VoidPtr, LargeTrivial &&)>::value,
                          "Large trivial types should be passed by rvalue reference");
            static_assert(
                    std::is_same<Invoker<void, CopyableMovableInstance>,
                            void (*)(VoidPtr, CopyableMovableInstance &&)>::value,
                    "Types with copy/move ctor should be passed by rvalue reference");

            // References are passed as references.
            static_assert(
                    std::is_same<Invoker<void, int &>, void (*)(VoidPtr, int &)>::value,
                    "Reference types should be preserved");
            static_assert(
                    std::is_same<Invoker<void, CopyableMovableInstance &>,
                            void (*)(VoidPtr, CopyableMovableInstance &)>::value,
                    "Reference types should be preserved");
            static_assert(
                    std::is_same<Invoker<void, CopyableMovableInstance &&>,
                            void (*)(VoidPtr, CopyableMovableInstance &&)>::value,
                    "Reference types should be preserved");

            // Make sure the address of an object received by reference is the same as the
            // addess of the object passed by the caller.
            {
                LargeTrivial obj;
                auto test = [&obj](LargeTrivial &input) { ASSERT_EQ(&input, &obj); };
                abel::FunctionRef<void(LargeTrivial &)> ref(test);
                ref(obj);
            }

            {
                Trivial obj;
                auto test = [&obj](Trivial &input) { ASSERT_EQ(&input, &obj); };
                abel::FunctionRef<void(Trivial &)> ref(test);
                ref(obj);
            }
        }

    }  // namespace

}  // namespace abel
