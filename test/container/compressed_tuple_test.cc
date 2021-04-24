// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/container/internal/compressed_tuple.h"

#include <memory>
#include <string>
#include <any>
#include <optional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "testing/test_instance_tracker.h"
#include "abel/memory/memory.h"
#include "abel/utility/utility.h"

// These are declared at global scope purely so that error messages
// are smaller and easier to understand.
enum class CallType {
    kConstRef, kConstMove
};

template<int>
struct Empty {
    constexpr CallType value() const &{ return CallType::kConstRef; }

    constexpr CallType value() const &&{ return CallType::kConstMove; }
};

template<typename T>
struct NotEmpty {
    T value;
};

template<typename T, typename U>
struct TwoValues {
    T value1;
    U value2;
};

namespace abel {

    namespace container_internal {
        namespace {

            using abel::test_internal::CopyableMovableInstance;
            using abel::test_internal::InstanceTracker;

            TEST(CompressedTupleTest, Sizeof) {
                EXPECT_EQ(sizeof(int), sizeof(compressed_tuple<int>));
                EXPECT_EQ(sizeof(int), sizeof(compressed_tuple<int, Empty<0>>));
                EXPECT_EQ(sizeof(int), sizeof(compressed_tuple<int, Empty<0>, Empty<1>>));
                EXPECT_EQ(sizeof(int),
                          sizeof(compressed_tuple<int, Empty<0>, Empty<1>, Empty<2>>));

                EXPECT_EQ(sizeof(TwoValues<int, double>),
                          sizeof(compressed_tuple<int, NotEmpty<double>>));
                EXPECT_EQ(sizeof(TwoValues<int, double>),
                          sizeof(compressed_tuple<int, Empty<0>, NotEmpty<double>>));
                EXPECT_EQ(sizeof(TwoValues<int, double>),
                          sizeof(compressed_tuple<int, Empty<0>, NotEmpty<double>, Empty<1>>));
            }

            TEST(CompressedTupleTest, OneMoveOnRValueConstructionTemp) {
                InstanceTracker tracker;
                compressed_tuple<CopyableMovableInstance> x1(CopyableMovableInstance(1));
                EXPECT_EQ(tracker.instances(), 1);
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_LE(tracker.moves(), 1);
                EXPECT_EQ(x1.get<0>().value(), 1);
            }

            TEST(CompressedTupleTest, OneMoveOnRValueConstructionMove) {
                InstanceTracker tracker;

                CopyableMovableInstance i1(1);
                compressed_tuple<CopyableMovableInstance> x1(std::move(i1));
                EXPECT_EQ(tracker.instances(), 2);
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_LE(tracker.moves(), 1);
                EXPECT_EQ(x1.get<0>().value(), 1);
            }

            TEST(CompressedTupleTest, OneMoveOnRValueConstructionMixedTypes) {
                InstanceTracker tracker;
                CopyableMovableInstance i1(1);
                CopyableMovableInstance i2(2);
                Empty<0> empty;
                compressed_tuple<CopyableMovableInstance, CopyableMovableInstance &, Empty<0>>
                        x1(std::move(i1), i2, empty);
                EXPECT_EQ(x1.get<0>().value(), 1);
                EXPECT_EQ(x1.get<1>().value(), 2);
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_EQ(tracker.moves(), 1);
            }

            struct IncompleteType;

            compressed_tuple<CopyableMovableInstance, IncompleteType &, Empty<0>>
            MakeWithIncomplete(CopyableMovableInstance i1,
                               IncompleteType &t,  // NOLINT
                               Empty<0> empty) {
                return compressed_tuple<CopyableMovableInstance, IncompleteType &, Empty<0>>{
                        std::move(i1), t, empty};
            }

            struct IncompleteType {
            };
            TEST(CompressedTupleTest, OneMoveOnRValueConstructionWithIncompleteType) {
                InstanceTracker tracker;
                CopyableMovableInstance i1(1);
                Empty<0> empty;
                struct DerivedType : IncompleteType {
                    int value = 0;
                };
                DerivedType fd;
                fd.value = 7;

                compressed_tuple<CopyableMovableInstance, IncompleteType &, Empty<0>> x1 =
                        MakeWithIncomplete(std::move(i1), fd, empty);

                EXPECT_EQ(x1.get<0>().value(), 1);
                EXPECT_EQ(static_cast<DerivedType &>(x1.get<1>()).value, 7);

                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_EQ(tracker.moves(), 2);
            }

            TEST(CompressedTupleTest,
                 OneMoveOnRValueConstructionMixedTypes_BraceInitPoisonPillExpected) {
                InstanceTracker tracker;
                CopyableMovableInstance i1(1);
                CopyableMovableInstance i2(2);
                compressed_tuple<CopyableMovableInstance, CopyableMovableInstance &, Empty<0>>
                        x1(std::move(i1), i2, {});  // NOLINT
                EXPECT_EQ(x1.get<0>().value(), 1);
                EXPECT_EQ(x1.get<1>().value(), 2);
                EXPECT_EQ(tracker.instances(), 3);
                // We are forced into the `const Ts&...` constructor (invoking copies)
                // because we need it to deduce the type of `{}`.
                // std::tuple also has this behavior.
                // Note, this test is proof that this is expected behavior, but it is not
                // _desired_ behavior.
                EXPECT_EQ(tracker.copies(), 1);
                EXPECT_EQ(tracker.moves(), 0);
            }

            TEST(CompressedTupleTest, OneCopyOnLValueConstruction) {
                InstanceTracker tracker;
                CopyableMovableInstance i1(1);

                compressed_tuple<CopyableMovableInstance> x1(i1);
                EXPECT_EQ(tracker.copies(), 1);
                EXPECT_EQ(tracker.moves(), 0);

                tracker.ResetCopiesMovesSwaps();

                CopyableMovableInstance i2(2);
                const CopyableMovableInstance &i2_ref = i2;
                compressed_tuple<CopyableMovableInstance> x2(i2_ref);
                EXPECT_EQ(tracker.copies(), 1);
                EXPECT_EQ(tracker.moves(), 0);
            }

            TEST(CompressedTupleTest, OneMoveOnRValueAccess) {
                InstanceTracker tracker;
                CopyableMovableInstance i1(1);
                compressed_tuple<CopyableMovableInstance> x(std::move(i1));
                tracker.ResetCopiesMovesSwaps();

                CopyableMovableInstance i2 = std::move(x).get<0>();
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_EQ(tracker.moves(), 1);
            }

            TEST(CompressedTupleTest, OneCopyOnLValueAccess) {
                InstanceTracker tracker;

                compressed_tuple<CopyableMovableInstance> x(CopyableMovableInstance(0));
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_EQ(tracker.moves(), 1);

                CopyableMovableInstance t = x.get<0>();
                EXPECT_EQ(tracker.copies(), 1);
                EXPECT_EQ(tracker.moves(), 1);
            }

            TEST(CompressedTupleTest, ZeroCopyOnRefAccess) {
                InstanceTracker tracker;

                compressed_tuple<CopyableMovableInstance> x(CopyableMovableInstance(0));
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_EQ(tracker.moves(), 1);

                CopyableMovableInstance &t1 = x.get<0>();
                const CopyableMovableInstance &t2 = x.get<0>();
                EXPECT_EQ(tracker.copies(), 0);
                EXPECT_EQ(tracker.moves(), 1);
                EXPECT_EQ(t1.value(), 0);
                EXPECT_EQ(t2.value(), 0);
            }

            TEST(CompressedTupleTest, Access) {
                struct S {
                    std::string x;
                };
                compressed_tuple<int, Empty<0>, S> x(7, {}, S{"ABC"});
                EXPECT_EQ(sizeof(x), sizeof(TwoValues<int, S>));
                EXPECT_EQ(7, x.get<0>());
                EXPECT_EQ("ABC", x.get<2>().x);
            }

            TEST(CompressedTupleTest, NonClasses) {
                compressed_tuple<int, const char *> x(7, "ABC");
                EXPECT_EQ(7, x.get<0>());
                EXPECT_STREQ("ABC", x.get<1>());
            }

            TEST(CompressedTupleTest, MixClassAndNonClass) {
                compressed_tuple<int, const char *, Empty<0>, NotEmpty<double>> x(7, "ABC", {},
                                                                                 {1.25});
                struct Mock {
                    int v;
                    const char *p;
                    double d;
                };
                EXPECT_EQ(sizeof(x), sizeof(Mock));
                EXPECT_EQ(7, x.get<0>());
                EXPECT_STREQ("ABC", x.get<1>());
                EXPECT_EQ(1.25, x.get<3>().value);
            }

            TEST(CompressedTupleTest, Nested) {
                compressed_tuple<int, compressed_tuple<int>,
                        compressed_tuple<int, compressed_tuple<int>>>
                        x(1, compressed_tuple<int>(2),
                          compressed_tuple<int, compressed_tuple<int>>(3, compressed_tuple<int>(4)));
                EXPECT_EQ(1, x.get<0>());
                EXPECT_EQ(2, x.get<1>().get<0>());
                EXPECT_EQ(3, x.get<2>().get<0>());
                EXPECT_EQ(4, x.get<2>().get<1>().get<0>());

                compressed_tuple<Empty<0>, Empty<0>,
                        compressed_tuple<Empty<0>, compressed_tuple<Empty<0>>>>
                        y;
                std::set<Empty<0> *> empties{&y.get<0>(), &y.get<1>(), &y.get<2>().get<0>(),
                                             &y.get<2>().get<1>().get<0>()};
#ifdef _MSC_VER
                // MSVC has a bug where many instances of the same base class are layed out in
                // the same address when using __declspec(empty_bases).
                // This will be fixed in a future version of MSVC.
                int expected = 1;
#else
                int expected = 4;
#endif
                EXPECT_EQ(expected, sizeof(y));
                EXPECT_EQ(expected, empties.size());
                EXPECT_EQ(sizeof(y), sizeof(Empty<0>) * empties.size());

                EXPECT_EQ(4 * sizeof(char),
                          sizeof(compressed_tuple<compressed_tuple<char, char>,
                                  compressed_tuple<char, char>>));
                EXPECT_TRUE((std::is_empty<compressed_tuple<Empty<0>, Empty<1>>>::value));

                // Make sure everything still works when things are nested.
                struct CT_Empty : compressed_tuple<Empty<0>> {
                };
                compressed_tuple<Empty<0>, CT_Empty> nested_empty;
                auto contained = nested_empty.get<0>();
                auto nested = nested_empty.get<1>().get<0>();
                EXPECT_TRUE((std::is_same<decltype(contained), decltype(nested)>::value));
            }

            TEST(CompressedTupleTest, Reference) {
                int i = 7;
                std::string s = "Very long std::string that goes in the heap";
                compressed_tuple<int, int &, std::string, std::string &> x(i, i, s, s);

                // Sanity check. We should have not moved from `s`
                EXPECT_EQ(s, "Very long std::string that goes in the heap");

                EXPECT_EQ(x.get<0>(), x.get<1>());
                EXPECT_NE(&x.get<0>(), &x.get<1>());
                EXPECT_EQ(&x.get<1>(), &i);

                EXPECT_EQ(x.get<2>(), x.get<3>());
                EXPECT_NE(&x.get<2>(), &x.get<3>());
                EXPECT_EQ(&x.get<3>(), &s);
            }

            TEST(CompressedTupleTest, NoElements) {
                compressed_tuple<> x;
                static_cast<void>(x);  // Silence -Wunused-variable.
                EXPECT_TRUE(std::is_empty<compressed_tuple<>>::value);
            }

            TEST(CompressedTupleTest, MoveOnlyElements) {
                compressed_tuple<std::unique_ptr<std::string>> str_tup(
                        abel::make_unique<std::string>("str"));

                compressed_tuple<compressed_tuple<std::unique_ptr<std::string>>,
                        std::unique_ptr<int>>
                        x(std::move(str_tup), abel::make_unique<int>(5));

                EXPECT_EQ(*x.get<0>().get<0>(), "str");
                EXPECT_EQ(*x.get<1>(), 5);

                std::unique_ptr<std::string> x0 = std::move(x.get<0>()).get<0>();
                std::unique_ptr<int> x1 = std::move(x).get<1>();

                EXPECT_EQ(*x0, "str");
                EXPECT_EQ(*x1, 5);
            }

            TEST(CompressedTupleTest, MoveConstructionMoveOnlyElements) {
                compressed_tuple<std::unique_ptr<std::string>> base(
                        abel::make_unique<std::string>("str"));
                EXPECT_EQ(*base.get<0>(), "str");

                compressed_tuple<std::unique_ptr<std::string>> copy(std::move(base));
                EXPECT_EQ(*copy.get<0>(), "str");
            }

            TEST(CompressedTupleTest, AnyElements) {
                std::any a(std::string("str"));
                compressed_tuple<std::any, std::any &> x(std::any(5), a);
                EXPECT_EQ(std::any_cast<int>(x.get<0>()), 5);
                EXPECT_EQ(std::any_cast<std::string>(x.get<1>()), "str");

                a = 0.5f;
                EXPECT_EQ(std::any_cast<float>(x.get<1>()), 0.5);
            }

            TEST(CompressedTupleTest, Constexpr) {
                struct NonTrivialStruct {
                    constexpr NonTrivialStruct() = default;

                    constexpr int value() const { return v; }

                    int v = 5;
                };
                struct TrivialStruct {
                    TrivialStruct() = default;

                    constexpr int value() const { return v; }

                    int v;
                };
                constexpr compressed_tuple<int, double, compressed_tuple<int>, Empty<0>> x(
                        7, 1.25, compressed_tuple<int>(5), {});
                constexpr int x0 = x.get<0>();
                constexpr double x1 = x.get<1>();
                constexpr int x2 = x.get<2>().get<0>();
                constexpr CallType x3 = x.get<3>().value();

                EXPECT_EQ(x0, 7);
                EXPECT_EQ(x1, 1.25);
                EXPECT_EQ(x2, 5);
                EXPECT_EQ(x3, CallType::kConstRef);

#if !defined(__GNUC__) || defined(__clang__) || __GNUC__ > 4
                constexpr compressed_tuple<Empty<0>, TrivialStruct, int> trivial = {};
                constexpr CallType trivial0 = trivial.get<0>().value();
                constexpr int trivial1 = trivial.get<1>().value();
                constexpr int trivial2 = trivial.get<2>();

                EXPECT_EQ(trivial0, CallType::kConstRef);
                EXPECT_EQ(trivial1, 0);
                EXPECT_EQ(trivial2, 0);
#endif

                constexpr compressed_tuple<Empty<0>, NonTrivialStruct, std::optional<int>>
                        non_trivial = {};
                constexpr CallType non_trivial0 = non_trivial.get<0>().value();
                constexpr int non_trivial1 = non_trivial.get<1>().value();
                constexpr std::optional<int> non_trivial2 = non_trivial.get<2>();

                EXPECT_EQ(non_trivial0, CallType::kConstRef);
                EXPECT_EQ(non_trivial1, 5);
                EXPECT_EQ(non_trivial2, std::nullopt);

                static constexpr char data[] = "DEF";
                constexpr compressed_tuple<const char *> z(data);
                constexpr const char *z1 = z.get<0>();
                EXPECT_EQ(std::string(z1), std::string(data));

#if defined(__clang__)
                // An apparent bug in earlier versions of gcc claims these are ambiguous.
                constexpr int x2m = abel::move(x.get<2>()).get<0>();
                constexpr CallType x3m = abel::move(x).get<3>().value();
                EXPECT_EQ(x2m, 5);
                EXPECT_EQ(x3m, CallType::kConstMove);
#endif
            }

#if defined(__clang__) || defined(__GNUC__)
            TEST(CompressedTupleTest, EmptyFinalClass) {
                struct S final {
                    int f() const { return 5; }
                };
                compressed_tuple<S> x;
                EXPECT_EQ(x.get<0>().f(), 5);
            }

#endif

        }  // namespace
    }  // namespace container_internal

}  // namespace abel
