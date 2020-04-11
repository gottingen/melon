//

#include <abel/asl/optional.h>

// This test is a no-op when abel::optional is an alias for std::optional.
#if !defined(ABEL_USES_STD_OPTIONAL)

#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include <abel/base/profile.h>
#include <abel/log/abel_logging.h>
#include <abel/asl/type_traits.h>
#include <abel/asl/string_view.h>

struct Hashable {
};

namespace std {
    template<>
    struct hash<Hashable> {
        size_t operator()(const Hashable &) { return 0; }
    };
}  // namespace std

struct NonHashable {
};

namespace {

    std::string TypeQuals(std::string &) { return "&"; }

    std::string TypeQuals(std::string &&) { return "&&"; }

    std::string TypeQuals(const std::string &) { return "c&"; }

    std::string TypeQuals(const std::string &&) { return "c&&"; }

    struct StructorListener {
        int construct0 = 0;
        int construct1 = 0;
        int construct2 = 0;
        int listinit = 0;
        int copy = 0;
        int move = 0;
        int copy_assign = 0;
        int move_assign = 0;
        int destruct = 0;
        int volatile_copy = 0;
        int volatile_move = 0;
        int volatile_copy_assign = 0;
        int volatile_move_assign = 0;
    };

// Suppress MSVC warnings.
// 4521: multiple copy constructors specified
// 4522: multiple assignment operators specified
// We wrote multiple of them to test that the correct overloads are selected.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4521)
#pragma warning( disable : 4522)
#endif

    struct Listenable {
        static StructorListener *listener;

        Listenable() { ++listener->construct0; }

        explicit Listenable(int /*unused*/) { ++listener->construct1; }

        Listenable(int /*unused*/, int /*unused*/) { ++listener->construct2; }

        Listenable(std::initializer_list<int> /*unused*/) { ++listener->listinit; }

        Listenable(const Listenable & /*unused*/) { ++listener->copy; }

        Listenable(const volatile Listenable & /*unused*/) {
            ++listener->volatile_copy;
        }

        Listenable(volatile Listenable && /*unused*/) { ++listener->volatile_move; }

        Listenable(Listenable && /*unused*/) { ++listener->move; }

        Listenable &operator=(const Listenable & /*unused*/) {
            ++listener->copy_assign;
            return *this;
        }

        Listenable &operator=(Listenable && /*unused*/) {
            ++listener->move_assign;
            return *this;
        }

        // use void return type instead of volatile T& to work around GCC warning
        // when the assignment's returned reference is ignored.
        void operator=(const volatile Listenable & /*unused*/) volatile {
            ++listener->volatile_copy_assign;
        }

        void operator=(volatile Listenable && /*unused*/) volatile {
            ++listener->volatile_move_assign;
        }

        ~Listenable() { ++listener->destruct; }
    };

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    StructorListener *Listenable::listener = nullptr;

// ABEL_HAVE_NO_CONSTEXPR_INITIALIZER_LIST is defined to 1 when the standard
// library implementation doesn't marked initializer_list's default constructor
// constexpr. The C++11 standard doesn't specify constexpr on it, but C++14
// added it. However, libstdc++ 4.7 marked it constexpr.
#if defined(_LIBCPP_VERSION) && \
    (_LIBCPP_STD_VER <= 11 || defined(_LIBCPP_HAS_NO_CXX14_CONSTEXPR))
#define ABEL_HAVE_NO_CONSTEXPR_INITIALIZER_LIST 1
#endif

    struct ConstexprType {
        enum CtorTypes {
            kCtorDefault,
            kCtorInt,
            kCtorInitializerList,
            kCtorConstChar
        };

        constexpr ConstexprType() : x(kCtorDefault) {}

        constexpr explicit ConstexprType(int i) : x(kCtorInt) {}

#ifndef ABEL_HAVE_NO_CONSTEXPR_INITIALIZER_LIST
        constexpr ConstexprType(std::initializer_list<int> il)
            : x(kCtorInitializerList) {}
#endif

        constexpr ConstexprType(const char *)  // NOLINT(runtime/explicit)
                : x(kCtorConstChar) {}

        int x;
    };

    struct Copyable {
        Copyable() {}

        Copyable(const Copyable &) {}

        Copyable &operator=(const Copyable &) { return *this; }
    };

    struct MoveableThrow {
        MoveableThrow() {}

        MoveableThrow(MoveableThrow &&) {}

        MoveableThrow &operator=(MoveableThrow &&) { return *this; }
    };

    struct MoveableNoThrow {
        MoveableNoThrow() {}

        MoveableNoThrow(MoveableNoThrow &&) noexcept {}

        MoveableNoThrow &operator=(MoveableNoThrow &&) noexcept { return *this; }
    };

    struct NonMovable {
        NonMovable() {}

        NonMovable(const NonMovable &) = delete;

        NonMovable &operator=(const NonMovable &) = delete;

        NonMovable(NonMovable &&) = delete;

        NonMovable &operator=(NonMovable &&) = delete;
    };

    struct NoDefault {
        NoDefault() = delete;

        NoDefault(const NoDefault &) {}

        NoDefault &operator=(const NoDefault &) { return *this; }
    };

    struct ConvertsFromInPlaceT {
        ConvertsFromInPlaceT(abel::in_place_t) {}  // NOLINT
    };

    TEST(optionalTest, DefaultConstructor) {
        abel::optional<int> empty;
        EXPECT_FALSE(empty);
        constexpr abel::optional<int> cempty;
        static_assert(!cempty.has_value(), "");
        EXPECT_TRUE(
                std::is_nothrow_default_constructible<abel::optional<int>>::value);
    }

    TEST(optionalTest, nulloptConstructor) {
        abel::optional<int> empty(abel::nullopt);
        EXPECT_FALSE(empty);
        constexpr abel::optional<int> cempty{abel::nullopt};
        static_assert(!cempty.has_value(), "");
        EXPECT_TRUE((std::is_nothrow_constructible<abel::optional<int>,
                abel::nullopt_t>::value));
    }

    TEST(optionalTest, CopyConstructor) {
        {
            abel::optional<int> empty, opt42 = 42;
            abel::optional<int> empty_copy(empty);
            EXPECT_FALSE(empty_copy);
            abel::optional<int> opt42_copy(opt42);
            EXPECT_TRUE(opt42_copy);
            EXPECT_EQ(42, *opt42_copy);
        }
        {
            abel::optional<const int> empty, opt42 = 42;
            abel::optional<const int> empty_copy(empty);
            EXPECT_FALSE(empty_copy);
            abel::optional<const int> opt42_copy(opt42);
            EXPECT_TRUE(opt42_copy);
            EXPECT_EQ(42, *opt42_copy);
        }
        {
            abel::optional<volatile int> empty, opt42 = 42;
            abel::optional<volatile int> empty_copy(empty);
            EXPECT_FALSE(empty_copy);
            abel::optional<volatile int> opt42_copy(opt42);
            EXPECT_TRUE(opt42_copy);
            EXPECT_EQ(42, *opt42_copy);
        }
        // test copyablility
        EXPECT_TRUE(std::is_copy_constructible<abel::optional<int>>::value);
        EXPECT_TRUE(std::is_copy_constructible<abel::optional<Copyable>>::value);
        EXPECT_FALSE(
                std::is_copy_constructible<abel::optional<MoveableThrow>>::value);
        EXPECT_FALSE(
                std::is_copy_constructible<abel::optional<MoveableNoThrow>>::value);
        EXPECT_FALSE(std::is_copy_constructible<abel::optional<NonMovable>>::value);

        EXPECT_FALSE(
                abel::is_trivially_copy_constructible<abel::optional<Copyable>>::value);
#if defined(ABEL_USES_STD_OPTIONAL) && defined(__GLIBCXX__)
        // libstdc++ std::optional implementation (as of 7.2) has a bug: when T is
        // trivially copyable, optional<T> is not trivially copyable (due to one of
        // its base class is unconditionally nontrivial).
#define ABEL_GLIBCXX_OPTIONAL_TRIVIALITY_BUG 1
#endif
#ifndef ABEL_GLIBCXX_OPTIONAL_TRIVIALITY_BUG
        EXPECT_TRUE(
                abel::is_trivially_copy_constructible<abel::optional<int>>::value);
        EXPECT_TRUE(
                abel::is_trivially_copy_constructible<abel::optional<const int>>::value);
#ifndef _MSC_VER
        // See defect report "Trivial copy/move constructor for class with volatile
        // member" at
        // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#2094
        // A class with non-static data member of volatile-qualified type should still
        // have a trivial copy constructor if the data member is trivial.
        // Also a cv-qualified scalar type should be trivially copyable.
        EXPECT_TRUE(abel::is_trivially_copy_constructible<
                            abel::optional<volatile int>>
                            ::value);
#endif  // _MSC_VER
#endif  // ABEL_GLIBCXX_OPTIONAL_TRIVIALITY_BUG

        // constexpr copy constructor for trivially copyable types
        {
            constexpr abel::optional<int> o1;
            constexpr abel::optional<int> o2 = o1;
            static_assert(!o2, "");
        }
        {
            constexpr abel::optional<int> o1 = 42;
            constexpr abel::optional<int> o2 = o1;
            static_assert(o2, "");
            static_assert(*o2 == 42, "");
        }
        {
            struct TrivialCopyable {
                constexpr TrivialCopyable() : x(0) {}

                constexpr explicit TrivialCopyable(int i) : x(i) {}

                int x;
            };
            constexpr abel::optional<TrivialCopyable> o1(42);
            constexpr abel::optional<TrivialCopyable> o2 = o1;
            static_assert(o2, "");
            static_assert((*o2).x == 42, "");
#ifndef ABEL_GLIBCXX_OPTIONAL_TRIVIALITY_BUG
            EXPECT_TRUE(abel::is_trivially_copy_constructible<
                                abel::optional<TrivialCopyable>>
                                ::value);
            EXPECT_TRUE(abel::is_trivially_copy_constructible<
                                abel::optional<const TrivialCopyable>>
                                ::value);
#endif
            // When testing with VS 2017 15.3, there seems to be a bug in MSVC
            // std::optional when T is volatile-qualified. So skipping this test.
            // Bug report:
            // https://connect.microsoft.com/VisualStudio/feedback/details/3142534
#if defined(ABEL_USES_STD_OPTIONAL) && defined(_MSC_VER) && _MSC_VER >= 1911
#define ABEL_MSVC_OPTIONAL_VOLATILE_COPY_BUG 1
#endif
#ifndef ABEL_MSVC_OPTIONAL_VOLATILE_COPY_BUG
            EXPECT_FALSE(std::is_copy_constructible<
                                 abel::optional<volatile TrivialCopyable>>
                                 ::value);
#endif
        }
    }

    TEST(optionalTest, MoveConstructor) {
        abel::optional<int> empty, opt42 = 42;
        abel::optional<int> empty_move(std::move(empty));
        EXPECT_FALSE(empty_move);
        abel::optional<int> opt42_move(std::move(opt42));
        EXPECT_TRUE(opt42_move);
        EXPECT_EQ(42, opt42_move);
        // test movability
        EXPECT_TRUE(std::is_move_constructible<abel::optional<int>>::value);
        EXPECT_TRUE(std::is_move_constructible<abel::optional<Copyable>>::value);
        EXPECT_TRUE(std::is_move_constructible<abel::optional<MoveableThrow>>::value);
        EXPECT_TRUE(
                std::is_move_constructible<abel::optional<MoveableNoThrow>>::value);
        EXPECT_FALSE(std::is_move_constructible<abel::optional<NonMovable>>::value);
        // test noexcept
        EXPECT_TRUE(std::is_nothrow_move_constructible<abel::optional<int>>::value);
#ifndef ABEL_USES_STD_OPTIONAL
        EXPECT_EQ(
                abel::default_allocator_is_nothrow::value,
                std::is_nothrow_move_constructible<abel::optional<MoveableThrow>>::value);
#endif
        EXPECT_TRUE(std::is_nothrow_move_constructible<
                            abel::optional<MoveableNoThrow>>
                            ::value);
    }

    TEST(optionalTest, Destructor) {
        struct Trivial {
        };

        struct NonTrivial {
            NonTrivial(const NonTrivial &) {}

            NonTrivial &operator=(const NonTrivial &) { return *this; }

            ~NonTrivial() {}
        };

        EXPECT_TRUE(std::is_trivially_destructible<abel::optional<int>>::value);
        EXPECT_TRUE(std::is_trivially_destructible<abel::optional<Trivial>>::value);
        EXPECT_FALSE(
                std::is_trivially_destructible<abel::optional<NonTrivial>>::value);
    }

    TEST(optionalTest, InPlaceConstructor) {
        constexpr abel::optional<ConstexprType> opt0{abel::in_place_t()};
        static_assert(opt0, "");
        static_assert((*opt0).x == ConstexprType::kCtorDefault, "");
        constexpr abel::optional<ConstexprType> opt1{abel::in_place_t(), 1};
        static_assert(opt1, "");
        static_assert((*opt1).x == ConstexprType::kCtorInt, "");
#ifndef ABEL_HAVE_NO_CONSTEXPR_INITIALIZER_LIST
        constexpr abel::optional<ConstexprType> opt2{abel::in_place_t(), {1, 2}};
        static_assert(opt2, "");
        static_assert((*opt2).x == ConstexprType::kCtorInitializerList, "");
#endif

        EXPECT_FALSE((std::is_constructible<abel::optional<ConvertsFromInPlaceT>,
                abel::in_place_t>::value));
        EXPECT_FALSE((std::is_constructible<abel::optional<ConvertsFromInPlaceT>,
                const abel::in_place_t &>::value));
        EXPECT_TRUE(
                (std::is_constructible<abel::optional<ConvertsFromInPlaceT>,
                        abel::in_place_t, abel::in_place_t>::value));

        EXPECT_FALSE((std::is_constructible<abel::optional<NoDefault>,
                abel::in_place_t>::value));
        EXPECT_FALSE((std::is_constructible<abel::optional<NoDefault>,
                abel::in_place_t &&>::value));
    }

// template<U=T> optional(U&&);
    TEST(optionalTest, ValueConstructor) {
        constexpr abel::optional<int> opt0(0);
        static_assert(opt0, "");
        static_assert(*opt0 == 0, "");
        EXPECT_TRUE((std::is_convertible<int, abel::optional<int>>::value));
        // Copy initialization ( = "abc") won't work due to optional(optional&&)
        // is not constexpr. Use list initialization instead. This invokes
        // abel::optional<ConstexprType>::abel::optional<U>(U&&), with U = const char
        // (&) [4], which direct-initializes the ConstexprType value held by the
        // optional via ConstexprType::ConstexprType(const char*).
        constexpr abel::optional<ConstexprType> opt1 = {"abc"};
        static_assert(opt1, "");
        static_assert(ConstexprType::kCtorConstChar == (*opt1).x, "");
        EXPECT_TRUE(
                (std::is_convertible<const char *, abel::optional<ConstexprType>>::value));
        // direct initialization
        constexpr abel::optional<ConstexprType> opt2{2};
        static_assert(opt2, "");
        static_assert(ConstexprType::kCtorInt == (*opt2).x, "");
        EXPECT_FALSE(
                (std::is_convertible<int, abel::optional<ConstexprType>>::value));

        // this invokes abel::optional<int>::optional(int&&)
        // NOTE: this has different behavior than assignment, e.g.
        // "opt3 = {};" clears the optional rather than setting the value to 0
        // According to C++17 standard N4659 [over.ics.list] 16.3.3.1.5, (9.2)- "if
        // the initializer list has no elements, the implicit conversion is the
        // identity conversion", so `optional(int&&)` should be a better match than
        // `optional(optional&&)` which is a user-defined conversion.
        // Note: GCC 7 has a bug with this overload selection when compiled with
        // `-std=c++17`.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 7 && \
    __cplusplus == 201703L
#define ABEL_GCC7_OVER_ICS_LIST_BUG 1
#endif
#ifndef ABEL_GCC7_OVER_ICS_LIST_BUG
        constexpr abel::optional<int> opt3({});
        static_assert(opt3, "");
        static_assert(*opt3 == 0, "");
#endif

        // this invokes the move constructor with a default constructed optional
        // because non-template function is a better match than template function.
        abel::optional<ConstexprType> opt4({});
        EXPECT_FALSE(opt4);
    }

    struct Implicit {
    };

    struct Explicit {
    };

    struct Convert {
        Convert(const Implicit &)  // NOLINT(runtime/explicit)
                : implicit(true), move(false) {}

        Convert(Implicit &&)  // NOLINT(runtime/explicit)
                : implicit(true), move(true) {}

        explicit Convert(const Explicit &) : implicit(false), move(false) {}

        explicit Convert(Explicit &&) : implicit(false), move(true) {}

        bool implicit;
        bool move;
    };

    struct ConvertFromOptional {
        ConvertFromOptional(const Implicit &)  // NOLINT(runtime/explicit)
                : implicit(true), move(false), from_optional(false) {}

        ConvertFromOptional(Implicit &&)  // NOLINT(runtime/explicit)
                : implicit(true), move(true), from_optional(false) {}

        ConvertFromOptional(
                const abel::optional<Implicit> &)  // NOLINT(runtime/explicit)
                : implicit(true), move(false), from_optional(true) {}

        ConvertFromOptional(abel::optional<Implicit> &&)  // NOLINT(runtime/explicit)
                : implicit(true), move(true), from_optional(true) {}

        explicit ConvertFromOptional(const Explicit &)
                : implicit(false), move(false), from_optional(false) {}

        explicit ConvertFromOptional(Explicit &&)
                : implicit(false), move(true), from_optional(false) {}

        explicit ConvertFromOptional(const abel::optional<Explicit> &)
                : implicit(false), move(false), from_optional(true) {}

        explicit ConvertFromOptional(abel::optional<Explicit> &&)
                : implicit(false), move(true), from_optional(true) {}

        bool implicit;
        bool move;
        bool from_optional;
    };

    TEST(optionalTest, ConvertingConstructor) {
        abel::optional<Implicit> i_empty;
        abel::optional<Implicit> i(abel::in_place);
        abel::optional<Explicit> e_empty;
        abel::optional<Explicit> e(abel::in_place);
        {
            // implicitly constructing abel::optional<Convert> from
            // abel::optional<Implicit>
            abel::optional<Convert> empty = i_empty;
            EXPECT_FALSE(empty);
            abel::optional<Convert> opt_copy = i;
            EXPECT_TRUE(opt_copy);
            EXPECT_TRUE(opt_copy->implicit);
            EXPECT_FALSE(opt_copy->move);
            abel::optional<Convert> opt_move = abel::optional<Implicit>(abel::in_place);
            EXPECT_TRUE(opt_move);
            EXPECT_TRUE(opt_move->implicit);
            EXPECT_TRUE(opt_move->move);
        }
        {
            // explicitly constructing abel::optional<Convert> from
            // abel::optional<Explicit>
            abel::optional<Convert> empty(e_empty);
            EXPECT_FALSE(empty);
            abel::optional<Convert> opt_copy(e);
            EXPECT_TRUE(opt_copy);
            EXPECT_FALSE(opt_copy->implicit);
            EXPECT_FALSE(opt_copy->move);
            EXPECT_FALSE((std::is_convertible<const abel::optional<Explicit> &,
                    abel::optional<Convert>>::value));
            abel::optional<Convert> opt_move{abel::optional<Explicit>(abel::in_place)};
            EXPECT_TRUE(opt_move);
            EXPECT_FALSE(opt_move->implicit);
            EXPECT_TRUE(opt_move->move);
            EXPECT_FALSE((std::is_convertible<abel::optional<Explicit> &&,
                    abel::optional<Convert>>::value));
        }
        {
            // implicitly constructing abel::optional<ConvertFromOptional> from
            // abel::optional<Implicit> via
            // ConvertFromOptional(abel::optional<Implicit>&&) check that
            // ConvertFromOptional(Implicit&&) is NOT called
            static_assert(
                    std::is_convertible<abel::optional<Implicit>,
                            abel::optional<ConvertFromOptional>>::value,
                    "");
            abel::optional<ConvertFromOptional> opt0 = i_empty;
            EXPECT_TRUE(opt0);
            EXPECT_TRUE(opt0->implicit);
            EXPECT_FALSE(opt0->move);
            EXPECT_TRUE(opt0->from_optional);
            abel::optional<ConvertFromOptional> opt1 = abel::optional<Implicit>();
            EXPECT_TRUE(opt1);
            EXPECT_TRUE(opt1->implicit);
            EXPECT_TRUE(opt1->move);
            EXPECT_TRUE(opt1->from_optional);
        }
        {
            // implicitly constructing abel::optional<ConvertFromOptional> from
            // abel::optional<Explicit> via
            // ConvertFromOptional(abel::optional<Explicit>&&) check that
            // ConvertFromOptional(Explicit&&) is NOT called
            abel::optional<ConvertFromOptional> opt0(e_empty);
            EXPECT_TRUE(opt0);
            EXPECT_FALSE(opt0->implicit);
            EXPECT_FALSE(opt0->move);
            EXPECT_TRUE(opt0->from_optional);
            EXPECT_FALSE(
                    (std::is_convertible<const abel::optional<Explicit> &,
                            abel::optional<ConvertFromOptional>>::value));
            abel::optional<ConvertFromOptional> opt1{abel::optional<Explicit>()};
            EXPECT_TRUE(opt1);
            EXPECT_FALSE(opt1->implicit);
            EXPECT_TRUE(opt1->move);
            EXPECT_TRUE(opt1->from_optional);
            EXPECT_FALSE(
                    (std::is_convertible<abel::optional<Explicit> &&,
                            abel::optional<ConvertFromOptional>>::value));
        }
    }

    TEST(optionalTest, StructorBasic) {
        StructorListener listener;
        Listenable::listener = &listener;
        {
            abel::optional<Listenable> empty;
            EXPECT_FALSE(empty);
            abel::optional<Listenable> opt0(abel::in_place);
            EXPECT_TRUE(opt0);
            abel::optional<Listenable> opt1(abel::in_place, 1);
            EXPECT_TRUE(opt1);
            abel::optional<Listenable> opt2(abel::in_place, 1, 2);
            EXPECT_TRUE(opt2);
        }
        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(1, listener.construct1);
        EXPECT_EQ(1, listener.construct2);
        EXPECT_EQ(3, listener.destruct);
    }

    TEST(optionalTest, CopyMoveStructor) {
        StructorListener listener;
        Listenable::listener = &listener;
        abel::optional<Listenable> original(abel::in_place);
        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(0, listener.copy);
        EXPECT_EQ(0, listener.move);
        abel::optional<Listenable> copy(original);
        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(1, listener.copy);
        EXPECT_EQ(0, listener.move);
        abel::optional<Listenable> move(std::move(original));
        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(1, listener.copy);
        EXPECT_EQ(1, listener.move);
    }

    TEST(optionalTest, ListInit) {
        StructorListener listener;
        Listenable::listener = &listener;
        abel::optional<Listenable> listinit1(abel::in_place, {1});
        abel::optional<Listenable> listinit2(abel::in_place, {1, 2});
        EXPECT_EQ(2, listener.listinit);
    }

    TEST(optionalTest, AssignFromNullopt) {
        abel::optional<int> opt(1);
        opt = abel::nullopt;
        EXPECT_FALSE(opt);

        StructorListener listener;
        Listenable::listener = &listener;
        abel::optional<Listenable> opt1(abel::in_place);
        opt1 = abel::nullopt;
        EXPECT_FALSE(opt1);
        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(1, listener.destruct);

        EXPECT_TRUE((
                            std::is_nothrow_assignable<abel::optional<int>, abel::nullopt_t>::value));
        EXPECT_TRUE((std::is_nothrow_assignable<abel::optional<Listenable>,
                abel::nullopt_t>::value));
    }

    TEST(optionalTest, CopyAssignment) {
        const abel::optional<int> empty, opt1 = 1, opt2 = 2;
        abel::optional<int> empty_to_opt1, opt1_to_opt2, opt2_to_empty;

        EXPECT_FALSE(empty_to_opt1);
        empty_to_opt1 = empty;
        EXPECT_FALSE(empty_to_opt1);
        empty_to_opt1 = opt1;
        EXPECT_TRUE(empty_to_opt1);
        EXPECT_EQ(1, empty_to_opt1.value());

        EXPECT_FALSE(opt1_to_opt2);
        opt1_to_opt2 = opt1;
        EXPECT_TRUE(opt1_to_opt2);
        EXPECT_EQ(1, opt1_to_opt2.value());
        opt1_to_opt2 = opt2;
        EXPECT_TRUE(opt1_to_opt2);
        EXPECT_EQ(2, opt1_to_opt2.value());

        EXPECT_FALSE(opt2_to_empty);
        opt2_to_empty = opt2;
        EXPECT_TRUE(opt2_to_empty);
        EXPECT_EQ(2, opt2_to_empty.value());
        opt2_to_empty = empty;
        EXPECT_FALSE(opt2_to_empty);

        EXPECT_FALSE(abel::is_copy_assignable<abel::optional<const int>>::value);
        EXPECT_TRUE(abel::is_copy_assignable<abel::optional<Copyable>>::value);
        EXPECT_FALSE(abel::is_copy_assignable<abel::optional<MoveableThrow>>::value);
        EXPECT_FALSE(
                abel::is_copy_assignable<abel::optional<MoveableNoThrow>>::value);
        EXPECT_FALSE(abel::is_copy_assignable<abel::optional<NonMovable>>::value);

        EXPECT_TRUE(abel::is_trivially_copy_assignable<int>::value);
        EXPECT_TRUE(abel::is_trivially_copy_assignable<volatile int>::value);

        struct Trivial {
            int i;
        };
        struct NonTrivial {
            NonTrivial &operator=(const NonTrivial &) { return *this; }

            int i;
        };

        EXPECT_TRUE(abel::is_trivially_copy_assignable<Trivial>::value);
        EXPECT_FALSE(abel::is_copy_assignable<const Trivial>::value);
        EXPECT_FALSE(abel::is_copy_assignable<volatile Trivial>::value);
        EXPECT_TRUE(abel::is_copy_assignable<NonTrivial>::value);
        EXPECT_FALSE(abel::is_trivially_copy_assignable<NonTrivial>::value);

        // std::optional doesn't support volatile nontrivial types.
#ifndef ABEL_USES_STD_OPTIONAL
        {
            StructorListener listener;
            Listenable::listener = &listener;

            abel::optional<volatile Listenable> empty, set(abel::in_place);
            EXPECT_EQ(1, listener.construct0);
            abel::optional<volatile Listenable> empty_to_empty, empty_to_set,
                    set_to_empty(abel::in_place), set_to_set(abel::in_place);
            EXPECT_EQ(3, listener.construct0);
            empty_to_empty = empty;  // no effect
            empty_to_set = set;      // copy construct
            set_to_empty = empty;    // destruct
            set_to_set = set;        // copy assign
            EXPECT_EQ(1, listener.volatile_copy);
            EXPECT_EQ(0, listener.volatile_move);
            EXPECT_EQ(1, listener.destruct);
            EXPECT_EQ(1, listener.volatile_copy_assign);
        }
#endif  // ABEL_USES_STD_OPTIONAL
    }

    TEST(optionalTest, MoveAssignment) {
        {
            StructorListener listener;
            Listenable::listener = &listener;

            abel::optional<Listenable> empty1, empty2, set1(abel::in_place),
                    set2(abel::in_place);
            EXPECT_EQ(2, listener.construct0);
            abel::optional<Listenable> empty_to_empty, empty_to_set,
                    set_to_empty(abel::in_place), set_to_set(abel::in_place);
            EXPECT_EQ(4, listener.construct0);
            empty_to_empty = std::move(empty1);
            empty_to_set = std::move(set1);
            set_to_empty = std::move(empty2);
            set_to_set = std::move(set2);
            EXPECT_EQ(0, listener.copy);
            EXPECT_EQ(1, listener.move);
            EXPECT_EQ(1, listener.destruct);
            EXPECT_EQ(1, listener.move_assign);
        }
        // std::optional doesn't support volatile nontrivial types.
#ifndef ABEL_USES_STD_OPTIONAL
        {
            StructorListener listener;
            Listenable::listener = &listener;

            abel::optional<volatile Listenable> empty1, empty2, set1(abel::in_place),
                    set2(abel::in_place);
            EXPECT_EQ(2, listener.construct0);
            abel::optional<volatile Listenable> empty_to_empty, empty_to_set,
                    set_to_empty(abel::in_place), set_to_set(abel::in_place);
            EXPECT_EQ(4, listener.construct0);
            empty_to_empty = std::move(empty1);  // no effect
            empty_to_set = std::move(set1);      // move construct
            set_to_empty = std::move(empty2);    // destruct
            set_to_set = std::move(set2);        // move assign
            EXPECT_EQ(0, listener.volatile_copy);
            EXPECT_EQ(1, listener.volatile_move);
            EXPECT_EQ(1, listener.destruct);
            EXPECT_EQ(1, listener.volatile_move_assign);
        }
#endif  // ABEL_USES_STD_OPTIONAL
        EXPECT_FALSE(abel::is_move_assignable<abel::optional<const int>>::value);
        EXPECT_TRUE(abel::is_move_assignable<abel::optional<Copyable>>::value);
        EXPECT_TRUE(abel::is_move_assignable<abel::optional<MoveableThrow>>::value);
        EXPECT_TRUE(abel::is_move_assignable<abel::optional<MoveableNoThrow>>::value);
        EXPECT_FALSE(abel::is_move_assignable<abel::optional<NonMovable>>::value);

        EXPECT_FALSE(
                std::is_nothrow_move_assignable<abel::optional<MoveableThrow>>::value);
        EXPECT_TRUE(
                std::is_nothrow_move_assignable<abel::optional<MoveableNoThrow>>::value);
    }

    struct NoConvertToOptional {
        // disable implicit conversion from const NoConvertToOptional&
        // to abel::optional<NoConvertToOptional>.
        NoConvertToOptional(const NoConvertToOptional &) = delete;
    };

    struct CopyConvert {
        CopyConvert(const NoConvertToOptional &);

        CopyConvert &operator=(const CopyConvert &) = delete;

        CopyConvert &operator=(const NoConvertToOptional &);
    };

    struct CopyConvertFromOptional {
        CopyConvertFromOptional(const NoConvertToOptional &);

        CopyConvertFromOptional(const abel::optional<NoConvertToOptional> &);

        CopyConvertFromOptional &operator=(const CopyConvertFromOptional &) = delete;

        CopyConvertFromOptional &operator=(const NoConvertToOptional &);

        CopyConvertFromOptional &operator=(
                const abel::optional<NoConvertToOptional> &);
    };

    struct MoveConvert {
        MoveConvert(NoConvertToOptional &&);

        MoveConvert &operator=(const MoveConvert &) = delete;

        MoveConvert &operator=(NoConvertToOptional &&);
    };

    struct MoveConvertFromOptional {
        MoveConvertFromOptional(NoConvertToOptional &&);

        MoveConvertFromOptional(abel::optional<NoConvertToOptional> &&);

        MoveConvertFromOptional &operator=(const MoveConvertFromOptional &) = delete;

        MoveConvertFromOptional &operator=(NoConvertToOptional &&);

        MoveConvertFromOptional &operator=(abel::optional<NoConvertToOptional> &&);
    };

// template <typename U = T> abel::optional<T>& operator=(U&& v);
    TEST(optionalTest, ValueAssignment) {
        abel::optional<int> opt;
        EXPECT_FALSE(opt);
        opt = 42;
        EXPECT_TRUE(opt);
        EXPECT_EQ(42, opt.value());
        opt = abel::nullopt;
        EXPECT_FALSE(opt);
        opt = 42;
        EXPECT_TRUE(opt);
        EXPECT_EQ(42, opt.value());
        opt = 43;
        EXPECT_TRUE(opt);
        EXPECT_EQ(43, opt.value());
        opt = {};  // this should clear optional
        EXPECT_FALSE(opt);

        opt = {44};
        EXPECT_TRUE(opt);
        EXPECT_EQ(44, opt.value());

        // U = const NoConvertToOptional&
        EXPECT_TRUE((std::is_assignable<abel::optional<CopyConvert> &,
                const NoConvertToOptional &>::value));
        // U = const abel::optional<NoConvertToOptional>&
        EXPECT_TRUE((std::is_assignable<abel::optional<CopyConvertFromOptional> &,
                const NoConvertToOptional &>::value));
        // U = const NoConvertToOptional& triggers SFINAE because
        // std::is_constructible_v<MoveConvert, const NoConvertToOptional&> is false
        EXPECT_FALSE((std::is_assignable<abel::optional<MoveConvert> &,
                const NoConvertToOptional &>::value));
        // U = NoConvertToOptional
        EXPECT_TRUE((std::is_assignable<abel::optional<MoveConvert> &,
                NoConvertToOptional &&>::value));
        // U = const NoConvertToOptional& triggers SFINAE because
        // std::is_constructible_v<MoveConvertFromOptional, const
        // NoConvertToOptional&> is false
        EXPECT_FALSE((std::is_assignable<abel::optional<MoveConvertFromOptional> &,
                const NoConvertToOptional &>::value));
        // U = NoConvertToOptional
        EXPECT_TRUE((std::is_assignable<abel::optional<MoveConvertFromOptional> &,
                NoConvertToOptional &&>::value));
        // U = const abel::optional<NoConvertToOptional>&
        EXPECT_TRUE(
                (std::is_assignable<abel::optional<CopyConvertFromOptional> &,
                        const abel::optional<NoConvertToOptional> &>::value));
        // U = abel::optional<NoConvertToOptional>
        EXPECT_TRUE(
                (std::is_assignable<abel::optional<MoveConvertFromOptional> &,
                        abel::optional<NoConvertToOptional> &&>::value));
    }

// template <typename U> abel::optional<T>& operator=(const abel::optional<U>&
// rhs); template <typename U> abel::optional<T>& operator=(abel::optional<U>&&
// rhs);
    TEST(optionalTest, ConvertingAssignment) {
        abel::optional<int> opt_i;
        abel::optional<char> opt_c('c');
        opt_i = opt_c;
        EXPECT_TRUE(opt_i);
        EXPECT_EQ(*opt_c, *opt_i);
        opt_i = abel::optional<char>();
        EXPECT_FALSE(opt_i);
        opt_i = abel::optional<char>('d');
        EXPECT_TRUE(opt_i);
        EXPECT_EQ('d', *opt_i);

        abel::optional<std::string> opt_str;
        abel::optional<const char *> opt_cstr("abc");
        opt_str = opt_cstr;
        EXPECT_TRUE(opt_str);
        EXPECT_EQ(std::string("abc"), *opt_str);
        opt_str = abel::optional<const char *>();
        EXPECT_FALSE(opt_str);
        opt_str = abel::optional<const char *>("def");
        EXPECT_TRUE(opt_str);
        EXPECT_EQ(std::string("def"), *opt_str);

        // operator=(const abel::optional<U>&) with U = NoConvertToOptional
        EXPECT_TRUE(
                (std::is_assignable<abel::optional<CopyConvert>,
                        const abel::optional<NoConvertToOptional> &>::value));
        // operator=(const abel::optional<U>&) with U = NoConvertToOptional
        // triggers SFINAE because
        // std::is_constructible_v<MoveConvert, const NoConvertToOptional&> is false
        EXPECT_FALSE(
                (std::is_assignable<abel::optional<MoveConvert> &,
                        const abel::optional<NoConvertToOptional> &>::value));
        // operator=(abel::optional<U>&&) with U = NoConvertToOptional
        EXPECT_TRUE(
                (std::is_assignable<abel::optional<MoveConvert> &,
                        abel::optional<NoConvertToOptional> &&>::value));
        // operator=(const abel::optional<U>&) with U = NoConvertToOptional triggers
        // SFINAE because std::is_constructible_v<MoveConvertFromOptional, const
        // NoConvertToOptional&> is false. operator=(U&&) with U = const
        // abel::optional<NoConverToOptional>& triggers SFINAE because
        // std::is_constructible<MoveConvertFromOptional,
        // abel::optional<NoConvertToOptional>&&> is true.
        EXPECT_FALSE(
                (std::is_assignable<abel::optional<MoveConvertFromOptional> &,
                        const abel::optional<NoConvertToOptional> &>::value));
    }

    TEST(optionalTest, ResetAndHasValue) {
        StructorListener listener;
        Listenable::listener = &listener;
        abel::optional<Listenable> opt;
        EXPECT_FALSE(opt);
        EXPECT_FALSE(opt.has_value());
        opt.emplace();
        EXPECT_TRUE(opt);
        EXPECT_TRUE(opt.has_value());
        opt.reset();
        EXPECT_FALSE(opt);
        EXPECT_FALSE(opt.has_value());
        EXPECT_EQ(1, listener.destruct);
        opt.reset();
        EXPECT_FALSE(opt);
        EXPECT_FALSE(opt.has_value());

        constexpr abel::optional<int> empty;
        static_assert(!empty.has_value(), "");
        constexpr abel::optional<int> nonempty(1);
        static_assert(nonempty.has_value(), "");
    }

    TEST(optionalTest, Emplace) {
        StructorListener listener;
        Listenable::listener = &listener;
        abel::optional<Listenable> opt;
        EXPECT_FALSE(opt);
        opt.emplace(1);
        EXPECT_TRUE(opt);
        opt.emplace(1, 2);
        EXPECT_EQ(1, listener.construct1);
        EXPECT_EQ(1, listener.construct2);
        EXPECT_EQ(1, listener.destruct);

        abel::optional<std::string> o;
        EXPECT_TRUE((std::is_same<std::string &, decltype(o.emplace("abc"))>::value));
        std::string &ref = o.emplace("abc");
        EXPECT_EQ(&ref, &o.value());
    }

    TEST(optionalTest, ListEmplace) {
        StructorListener listener;
        Listenable::listener = &listener;
        abel::optional<Listenable> opt;
        EXPECT_FALSE(opt);
        opt.emplace({1});
        EXPECT_TRUE(opt);
        opt.emplace({1, 2});
        EXPECT_EQ(2, listener.listinit);
        EXPECT_EQ(1, listener.destruct);

        abel::optional<Listenable> o;
        EXPECT_TRUE((std::is_same<Listenable &, decltype(o.emplace({1}))>::value));
        Listenable &ref = o.emplace({1});
        EXPECT_EQ(&ref, &o.value());
    }

    TEST(optionalTest, Swap) {
        abel::optional<int> opt_empty, opt1 = 1, opt2 = 2;
        EXPECT_FALSE(opt_empty);
        EXPECT_TRUE(opt1);
        EXPECT_EQ(1, opt1.value());
        EXPECT_TRUE(opt2);
        EXPECT_EQ(2, opt2.value());
        swap(opt_empty, opt1);
        EXPECT_FALSE(opt1);
        EXPECT_TRUE(opt_empty);
        EXPECT_EQ(1, opt_empty.value());
        EXPECT_TRUE(opt2);
        EXPECT_EQ(2, opt2.value());
        swap(opt_empty, opt1);
        EXPECT_FALSE(opt_empty);
        EXPECT_TRUE(opt1);
        EXPECT_EQ(1, opt1.value());
        EXPECT_TRUE(opt2);
        EXPECT_EQ(2, opt2.value());
        swap(opt1, opt2);
        EXPECT_FALSE(opt_empty);
        EXPECT_TRUE(opt1);
        EXPECT_EQ(2, opt1.value());
        EXPECT_TRUE(opt2);
        EXPECT_EQ(1, opt2.value());

        EXPECT_TRUE(noexcept(opt1.swap(opt2)));
        EXPECT_TRUE(noexcept(swap(opt1, opt2)));
    }

    template<int v>
    struct DeletedOpAddr {
        int value = v;

        constexpr DeletedOpAddr() = default;

        constexpr const DeletedOpAddr<v> *operator&() const = delete;  // NOLINT
        DeletedOpAddr<v> *operator&() = delete;                        // NOLINT
    };

// The static_assert featuring a constexpr call to operator->() is commented out
// to document the fact that the current implementation of abel::optional<T>
// expects such usecases to be malformed and not compile.
    TEST(optionalTest, OperatorAddr) {
        constexpr int v = -1;
        {  // constexpr
            constexpr abel::optional<DeletedOpAddr<v>> opt(abel::in_place_t{});
            static_assert(opt.has_value(), "");
            // static_assert(opt->value == v, "");
            static_assert((*opt).value == v, "");
        }
        {  // non-constexpr
            const abel::optional<DeletedOpAddr<v>> opt(abel::in_place_t{});
            EXPECT_TRUE(opt.has_value());
            EXPECT_TRUE(opt->value == v);
            EXPECT_TRUE((*opt).value == v);
        }
    }

    TEST(optionalTest, PointerStuff) {
        abel::optional<std::string> opt(abel::in_place, "foo");
        EXPECT_EQ("foo", *opt);
        const auto &opt_const = opt;
        EXPECT_EQ("foo", *opt_const);
        EXPECT_EQ(opt->size(), 3);
        EXPECT_EQ(opt_const->size(), 3);

        constexpr abel::optional<ConstexprType> opt1(1);
        static_assert((*opt1).x == ConstexprType::kCtorInt, "");
    }

// gcc has a bug pre 4.9.1 where it doesn't do correct overload resolution
// when overloads are const-qualified and *this is an raluve.
// Skip that test to make the build green again when using the old compiler.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59296 is fixed in 4.9.1.
#if defined(__GNUC__) && !defined(__clang__)
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 40901
#define ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG
#endif
#endif

// MSVC has a bug with "cv-qualifiers in class construction", fixed in 2017. See
// https://docs.microsoft.com/en-us/cpp/cpp-conformance-improvements-2017#bug-fixes
// The compiler some incorrectly ingores the cv-qualifier when generating a
// class object via a constructor call. For example:
//
// class optional {
//   constexpr T&& value() &&;
//   constexpr const T&& value() const &&;
// }
//
// using COI = const abel::optional<int>;
// static_assert(2 == COI(2).value(), "");  // const &&
//
// This should invoke the "const &&" overload but since it ignores the const
// qualifier it finds the "&&" overload the best candidate.
#if defined(_MSC_VER) && _MSC_VER < 1910
#define ABEL_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG
#endif

    TEST(optionalTest, Value) {
        using O = abel::optional<std::string>;
        using CO = const abel::optional<std::string>;
        using OC = abel::optional<const std::string>;
        O lvalue(abel::in_place, "lvalue");
        CO clvalue(abel::in_place, "clvalue");
        OC lvalue_c(abel::in_place, "lvalue_c");
        EXPECT_EQ("lvalue", lvalue.value());
        EXPECT_EQ("clvalue", clvalue.value());
        EXPECT_EQ("lvalue_c", lvalue_c.value());
        EXPECT_EQ("xvalue", O(abel::in_place, "xvalue").value());
        EXPECT_EQ("xvalue_c", OC(abel::in_place, "xvalue_c").value());
#ifndef ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG
        EXPECT_EQ("cxvalue", CO(abel::in_place, "cxvalue").value());
#endif
        EXPECT_EQ("&", TypeQuals(lvalue.value()));
        EXPECT_EQ("c&", TypeQuals(clvalue.value()));
        EXPECT_EQ("c&", TypeQuals(lvalue_c.value()));
        EXPECT_EQ("&&", TypeQuals(O(abel::in_place, "xvalue").value()));
#if !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG) && \
    !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
        EXPECT_EQ("c&&", TypeQuals(CO(abel::in_place, "cxvalue").value()));
#endif
        EXPECT_EQ("c&&", TypeQuals(OC(abel::in_place, "xvalue_c").value()));

        // test on volatile type
        using OV = abel::optional<volatile int>;
        OV lvalue_v(abel::in_place, 42);
        EXPECT_EQ(42, lvalue_v.value());
        EXPECT_EQ(42, OV(42).value());
        EXPECT_TRUE((std::is_same<volatile int &, decltype(lvalue_v.value())>::value));
        EXPECT_TRUE((std::is_same<volatile int &&, decltype(OV(42).value())>::value));

        // test exception throw on value()
        abel::optional<int> empty;
#ifdef ABEL_HAVE_EXCEPTIONS
        EXPECT_THROW((void) empty.value(), abel::bad_optional_access);
#else
        EXPECT_DEATH((void)empty.value(), "Bad optional access");
#endif

        // test constexpr value()
        constexpr abel::optional<int> o1(1);
        static_assert(1 == o1.value(), "");  // const &
#if !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG) && \
    !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
        using COI = const abel::optional<int>;
        static_assert(2 == COI(2).value(), "");  // const &&
#endif
    }

    TEST(optionalTest, DerefOperator) {
        using O = abel::optional<std::string>;
        using CO = const abel::optional<std::string>;
        using OC = abel::optional<const std::string>;
        O lvalue(abel::in_place, "lvalue");
        CO clvalue(abel::in_place, "clvalue");
        OC lvalue_c(abel::in_place, "lvalue_c");
        EXPECT_EQ("lvalue", *lvalue);
        EXPECT_EQ("clvalue", *clvalue);
        EXPECT_EQ("lvalue_c", *lvalue_c);
        EXPECT_EQ("xvalue", *O(abel::in_place, "xvalue"));
        EXPECT_EQ("xvalue_c", *OC(abel::in_place, "xvalue_c"));
#ifndef ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG
        EXPECT_EQ("cxvalue", *CO(abel::in_place, "cxvalue"));
#endif
        EXPECT_EQ("&", TypeQuals(*lvalue));
        EXPECT_EQ("c&", TypeQuals(*clvalue));
        EXPECT_EQ("&&", TypeQuals(*O(abel::in_place, "xvalue")));
#if !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG) && \
    !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
        EXPECT_EQ("c&&", TypeQuals(*CO(abel::in_place, "cxvalue")));
#endif
        EXPECT_EQ("c&&", TypeQuals(*OC(abel::in_place, "xvalue_c")));

        // test on volatile type
        using OV = abel::optional<volatile int>;
        OV lvalue_v(abel::in_place, 42);
        EXPECT_EQ(42, *lvalue_v);
        EXPECT_EQ(42, *OV(42));
        EXPECT_TRUE((std::is_same<volatile int &, decltype(*lvalue_v)>::value));
        EXPECT_TRUE((std::is_same<volatile int &&, decltype(*OV(42))>::value));

        constexpr abel::optional<int> opt1(1);
        static_assert(*opt1 == 1, "");
#if !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG) && \
    !defined(ABEL_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
        using COI = const abel::optional<int>;
        static_assert(*COI(2) == 2, "");
#endif
    }

    TEST(optionalTest, ValueOr) {
        abel::optional<double> opt_empty, opt_set = 1.2;
        EXPECT_EQ(42.0, opt_empty.value_or(42));
        EXPECT_EQ(1.2, opt_set.value_or(42));
        EXPECT_EQ(42.0, abel::optional<double>().value_or(42));
        EXPECT_EQ(1.2, abel::optional<double>(1.2).value_or(42));

        constexpr abel::optional<double> copt_empty, copt_set = {1.2};
        static_assert(42.0 == copt_empty.value_or(42), "");
        static_assert(1.2 == copt_set.value_or(42), "");
#ifndef ABEL_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG
        using COD = const abel::optional<double>;
        static_assert(42.0 == COD().value_or(42), "");
        static_assert(1.2 == COD(1.2).value_or(42), "");
#endif
    }

// make_optional cannot be constexpr until C++17
    TEST(optionalTest, make_optional) {
        auto opt_int = abel::make_optional(42);
        EXPECT_TRUE((std::is_same<decltype(opt_int), abel::optional<int>>::value));
        EXPECT_EQ(42, opt_int);

        StructorListener listener;
        Listenable::listener = &listener;

        abel::optional<Listenable> opt0 = abel::make_optional<Listenable>();
        EXPECT_EQ(1, listener.construct0);
        abel::optional<Listenable> opt1 = abel::make_optional<Listenable>(1);
        EXPECT_EQ(1, listener.construct1);
        abel::optional<Listenable> opt2 = abel::make_optional<Listenable>(1, 2);
        EXPECT_EQ(1, listener.construct2);
        abel::optional<Listenable> opt3 = abel::make_optional<Listenable>({1});
        abel::optional<Listenable> opt4 = abel::make_optional<Listenable>({1, 2});
        EXPECT_EQ(2, listener.listinit);

        // Constexpr tests on trivially copyable types
        // optional<T> has trivial copy/move ctors when T is trivially copyable.
        // For nontrivial types with constexpr constructors, we need copy elision in
        // C++17 for make_optional to be constexpr.
        {
            constexpr abel::optional<int> c_opt = abel::make_optional(42);
            static_assert(c_opt.value() == 42, "");
        }
        {
            struct TrivialCopyable {
                constexpr TrivialCopyable() : x(0) {}

                constexpr explicit TrivialCopyable(int i) : x(i) {}

                int x;
            };

            constexpr TrivialCopyable v;
            constexpr abel::optional<TrivialCopyable> c_opt0 = abel::make_optional(v);
            static_assert((*c_opt0).x == 0, "");
            constexpr abel::optional<TrivialCopyable> c_opt1 =
                    abel::make_optional<TrivialCopyable>();
            static_assert((*c_opt1).x == 0, "");
            constexpr abel::optional<TrivialCopyable> c_opt2 =
                    abel::make_optional<TrivialCopyable>(42);
            static_assert((*c_opt2).x == 42, "");
        }
    }

    template<typename T, typename U>
    void optionalTest_Comparisons_EXPECT_LESS(T x, U y) {
        EXPECT_FALSE(x == y);
        EXPECT_TRUE(x != y);
        EXPECT_TRUE(x < y);
        EXPECT_FALSE(x > y);
        EXPECT_TRUE(x <= y);
        EXPECT_FALSE(x >= y);
    }

    template<typename T, typename U>
    void optionalTest_Comparisons_EXPECT_SAME(T x, U y) {
        EXPECT_TRUE(x == y);
        EXPECT_FALSE(x != y);
        EXPECT_FALSE(x < y);
        EXPECT_FALSE(x > y);
        EXPECT_TRUE(x <= y);
        EXPECT_TRUE(x >= y);
    }

    template<typename T, typename U>
    void optionalTest_Comparisons_EXPECT_GREATER(T x, U y) {
        EXPECT_FALSE(x == y);
        EXPECT_TRUE(x != y);
        EXPECT_FALSE(x < y);
        EXPECT_TRUE(x > y);
        EXPECT_FALSE(x <= y);
        EXPECT_TRUE(x >= y);
    }


    template<typename T, typename U, typename V>
    void TestComparisons() {
        abel::optional<T> ae, a2{2}, a4{4};
        abel::optional<U> be, b2{2}, b4{4};
        V v3 = 3;

        // LHS: abel::nullopt, ae, a2, v3, a4
        // RHS: abel::nullopt, be, b2, v3, b4

        // optionalTest_Comparisons_EXPECT_NOT_TO_WORK(abel::nullopt,abel::nullopt);
        optionalTest_Comparisons_EXPECT_SAME(abel::nullopt, be);
        optionalTest_Comparisons_EXPECT_LESS(abel::nullopt, b2);
        // optionalTest_Comparisons_EXPECT_NOT_TO_WORK(abel::nullopt,v3);
        optionalTest_Comparisons_EXPECT_LESS(abel::nullopt, b4);

        optionalTest_Comparisons_EXPECT_SAME(ae, abel::nullopt);
        optionalTest_Comparisons_EXPECT_SAME(ae, be);
        optionalTest_Comparisons_EXPECT_LESS(ae, b2);
        optionalTest_Comparisons_EXPECT_LESS(ae, v3);
        optionalTest_Comparisons_EXPECT_LESS(ae, b4);

        optionalTest_Comparisons_EXPECT_GREATER(a2, abel::nullopt);
        optionalTest_Comparisons_EXPECT_GREATER(a2, be);
        optionalTest_Comparisons_EXPECT_SAME(a2, b2);
        optionalTest_Comparisons_EXPECT_LESS(a2, v3);
        optionalTest_Comparisons_EXPECT_LESS(a2, b4);

        // optionalTest_Comparisons_EXPECT_NOT_TO_WORK(v3,abel::nullopt);
        optionalTest_Comparisons_EXPECT_GREATER(v3, be);
        optionalTest_Comparisons_EXPECT_GREATER(v3, b2);
        optionalTest_Comparisons_EXPECT_SAME(v3, v3);
        optionalTest_Comparisons_EXPECT_LESS(v3, b4);

        optionalTest_Comparisons_EXPECT_GREATER(a4, abel::nullopt);
        optionalTest_Comparisons_EXPECT_GREATER(a4, be);
        optionalTest_Comparisons_EXPECT_GREATER(a4, b2);
        optionalTest_Comparisons_EXPECT_GREATER(a4, v3);
        optionalTest_Comparisons_EXPECT_SAME(a4, b4);
    }

    struct Int1 {
        Int1() = default;

        Int1(int i) : i(i) {}  // NOLINT(runtime/explicit)
        int i;
    };

    struct Int2 {
        Int2() = default;

        Int2(int i) : i(i) {}  // NOLINT(runtime/explicit)
        int i;
    };

// comparison between Int1 and Int2
    constexpr bool operator==(const Int1 &lhs, const Int2 &rhs) {
        return lhs.i == rhs.i;
    }

    constexpr bool operator!=(const Int1 &lhs, const Int2 &rhs) {
        return !(lhs == rhs);
    }

    constexpr bool operator<(const Int1 &lhs, const Int2 &rhs) {
        return lhs.i < rhs.i;
    }

    constexpr bool operator<=(const Int1 &lhs, const Int2 &rhs) {
        return lhs < rhs || lhs == rhs;
    }

    constexpr bool operator>(const Int1 &lhs, const Int2 &rhs) {
        return !(lhs <= rhs);
    }

    constexpr bool operator>=(const Int1 &lhs, const Int2 &rhs) {
        return !(lhs < rhs);
    }

    TEST(optionalTest, Comparisons) {
        TestComparisons<int, int, int>();
        TestComparisons<const int, int, int>();
        TestComparisons<Int1, int, int>();
        TestComparisons<int, Int2, int>();
        TestComparisons<Int1, Int2, int>();

        // compare abel::optional<std::string> with const char*
        abel::optional<std::string> opt_str = "abc";
        const char *cstr = "abc";
        EXPECT_TRUE(opt_str == cstr);
        // compare abel::optional<std::string> with abel::optional<const char*>
        abel::optional<const char *> opt_cstr = cstr;
        EXPECT_TRUE(opt_str == opt_cstr);
        // compare abel::optional<std::string> with abel::optional<abel::string_view>
        abel::optional<abel::string_view> e1;
        abel::optional<std::string> e2;
        EXPECT_TRUE(e1 == e2);
    }


    TEST(optionalTest, SwapRegression) {
        StructorListener listener;
        Listenable::listener = &listener;

        {
            abel::optional<Listenable> a;
            abel::optional<Listenable> b(abel::in_place);
            a.swap(b);
        }

        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(1, listener.move);
        EXPECT_EQ(2, listener.destruct);

        {
            abel::optional<Listenable> a(abel::in_place);
            abel::optional<Listenable> b;
            a.swap(b);
        }

        EXPECT_EQ(2, listener.construct0);
        EXPECT_EQ(2, listener.move);
        EXPECT_EQ(4, listener.destruct);
    }

    TEST(optionalTest, BigStringLeakCheck) {
        constexpr size_t n = 1 << 16;

        using OS = abel::optional<std::string>;

        OS a;
        OS b = abel::nullopt;
        OS c = std::string(n, 'c');
        std::string sd(n, 'd');
        OS d = sd;
        OS e(abel::in_place, n, 'e');
        OS f;
        f.emplace(n, 'f');

        OS ca(a);
        OS cb(b);
        OS cc(c);
        OS cd(d);
        OS ce(e);

        OS oa;
        OS ob = abel::nullopt;
        OS oc = std::string(n, 'c');
        std::string sod(n, 'd');
        OS od = sod;
        OS oe(abel::in_place, n, 'e');
        OS of;
        of.emplace(n, 'f');

        OS ma(std::move(oa));
        OS mb(std::move(ob));
        OS mc(std::move(oc));
        OS md(std::move(od));
        OS me(std::move(oe));
        OS mf(std::move(of));

        OS aa1;
        OS ab1 = abel::nullopt;
        OS ac1 = std::string(n, 'c');
        std::string sad1(n, 'd');
        OS ad1 = sad1;
        OS ae1(abel::in_place, n, 'e');
        OS af1;
        af1.emplace(n, 'f');

        OS aa2;
        OS ab2 = abel::nullopt;
        OS ac2 = std::string(n, 'c');
        std::string sad2(n, 'd');
        OS ad2 = sad2;
        OS ae2(abel::in_place, n, 'e');
        OS af2;
        af2.emplace(n, 'f');

        aa1 = af2;
        ab1 = ae2;
        ac1 = ad2;
        ad1 = ac2;
        ae1 = ab2;
        af1 = aa2;

        OS aa3;
        OS ab3 = abel::nullopt;
        OS ac3 = std::string(n, 'c');
        std::string sad3(n, 'd');
        OS ad3 = sad3;
        OS ae3(abel::in_place, n, 'e');
        OS af3;
        af3.emplace(n, 'f');

        aa3 = abel::nullopt;
        ab3 = abel::nullopt;
        ac3 = abel::nullopt;
        ad3 = abel::nullopt;
        ae3 = abel::nullopt;
        af3 = abel::nullopt;

        OS aa4;
        OS ab4 = abel::nullopt;
        OS ac4 = std::string(n, 'c');
        std::string sad4(n, 'd');
        OS ad4 = sad4;
        OS ae4(abel::in_place, n, 'e');
        OS af4;
        af4.emplace(n, 'f');

        aa4 = OS(abel::in_place, n, 'a');
        ab4 = OS(abel::in_place, n, 'b');
        ac4 = OS(abel::in_place, n, 'c');
        ad4 = OS(abel::in_place, n, 'd');
        ae4 = OS(abel::in_place, n, 'e');
        af4 = OS(abel::in_place, n, 'f');

        OS aa5;
        OS ab5 = abel::nullopt;
        OS ac5 = std::string(n, 'c');
        std::string sad5(n, 'd');
        OS ad5 = sad5;
        OS ae5(abel::in_place, n, 'e');
        OS af5;
        af5.emplace(n, 'f');

        std::string saa5(n, 'a');
        std::string sab5(n, 'a');
        std::string sac5(n, 'a');
        std::string sad52(n, 'a');
        std::string sae5(n, 'a');
        std::string saf5(n, 'a');

        aa5 = saa5;
        ab5 = sab5;
        ac5 = sac5;
        ad5 = sad52;
        ae5 = sae5;
        af5 = saf5;

        OS aa6;
        OS ab6 = abel::nullopt;
        OS ac6 = std::string(n, 'c');
        std::string sad6(n, 'd');
        OS ad6 = sad6;
        OS ae6(abel::in_place, n, 'e');
        OS af6;
        af6.emplace(n, 'f');

        aa6 = std::string(n, 'a');
        ab6 = std::string(n, 'b');
        ac6 = std::string(n, 'c');
        ad6 = std::string(n, 'd');
        ae6 = std::string(n, 'e');
        af6 = std::string(n, 'f');

        OS aa7;
        OS ab7 = abel::nullopt;
        OS ac7 = std::string(n, 'c');
        std::string sad7(n, 'd');
        OS ad7 = sad7;
        OS ae7(abel::in_place, n, 'e');
        OS af7;
        af7.emplace(n, 'f');

        aa7.emplace(n, 'A');
        ab7.emplace(n, 'B');
        ac7.emplace(n, 'C');
        ad7.emplace(n, 'D');
        ae7.emplace(n, 'E');
        af7.emplace(n, 'F');
    }

    TEST(optionalTest, MoveAssignRegression) {
        StructorListener listener;
        Listenable::listener = &listener;

        {
            abel::optional<Listenable> a;
            Listenable b;
            a = std::move(b);
        }

        EXPECT_EQ(1, listener.construct0);
        EXPECT_EQ(1, listener.move);
        EXPECT_EQ(2, listener.destruct);
    }

    TEST(optionalTest, ValueType) {
        EXPECT_TRUE((std::is_same<abel::optional<int>::value_type, int>::value));
        EXPECT_TRUE((std::is_same<abel::optional<std::string>::value_type,
                std::string>::value));
        EXPECT_FALSE(
                (std::is_same<abel::optional<int>::value_type, abel::nullopt_t>::value));
    }

    template<typename T>
    struct is_hash_enabled_for {
        template<typename U, typename = decltype(std::hash<U>()(std::declval<U>()))>
        static std::true_type test(int);

        template<typename U>
        static std::false_type test(...);

        static constexpr bool value = decltype(test<T>(0))::value;
    };

    TEST(optionalTest, Hash) {
        std::hash<abel::optional<int>> hash;
        std::set<size_t> hashcodes;
        hashcodes.insert(hash(abel::nullopt));
        for (int i = 0; i < 100; ++i) {
            hashcodes.insert(hash(i));
        }
        EXPECT_GT(hashcodes.size(), 90);

        static_assert(is_hash_enabled_for<abel::optional<int>>::value, "");
        static_assert(is_hash_enabled_for<abel::optional<Hashable>>::value, "");
        static_assert(
                abel::is_hashable<abel::optional<int>>::value, "");
        static_assert(
                abel::is_hashable<abel::optional<Hashable>>::value,
                "");
        abel::assert_hash_enabled<abel::optional<int>>();
        abel::assert_hash_enabled<abel::optional<Hashable>>();

#if ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
        static_assert(!is_hash_enabled_for<abel::optional<NonHashable>>::value, "");
        static_assert(!abel::is_hashable<
                              abel::optional<NonHashable>>::value,
                      "");
#endif

        // libstdc++ std::optional is missing remove_const_t, i.e. it's using
        // std::hash<T> rather than std::hash<std::remove_const_t<T>>.
        // Reference: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82262
#ifndef __GLIBCXX__
        static_assert(is_hash_enabled_for<abel::optional<const int>>::value, "");
        static_assert(is_hash_enabled_for<abel::optional<const Hashable>>::value, "");
        std::hash<abel::optional<const int>> c_hash;
        for (int i = 0; i < 100; ++i) {
            EXPECT_EQ(hash(i), c_hash(i));
        }
#endif
    }

    struct MoveMeNoThrow {
        MoveMeNoThrow() : x(0) {}

        [[noreturn]] MoveMeNoThrow(const MoveMeNoThrow &other) : x(other.x) {
            ABEL_RAW_LOG(FATAL, "Should not be called.");
            abort();
        }

        MoveMeNoThrow(MoveMeNoThrow &&other) noexcept : x(other.x) {}

        int x;
    };

    struct MoveMeThrow {
        MoveMeThrow() : x(0) {}

        MoveMeThrow(const MoveMeThrow &other) : x(other.x) {}

        MoveMeThrow(MoveMeThrow &&other) : x(other.x) {}

        int x;
    };

    TEST(optionalTest, NoExcept) {
        static_assert(
                std::is_nothrow_move_constructible<abel::optional<MoveMeNoThrow>>::value,
                "");
#ifndef ABEL_USES_STD_OPTIONAL
        static_assert(abel::default_allocator_is_nothrow::value ==
                      std::is_nothrow_move_constructible<
                              abel::optional<MoveMeThrow>>::value,
                      "");
#endif
        std::vector<abel::optional<MoveMeNoThrow>> v;
        for (int i = 0; i < 10; ++i) v.emplace_back();
    }

    struct AnyLike {
        AnyLike(AnyLike &&) = default;

        AnyLike(const AnyLike &) = default;

        template<typename ValueType,
                typename T = typename std::decay<ValueType>::type,
                typename std::enable_if<
                        !abel::disjunction<
                                std::is_same<AnyLike, T>,
                                abel::negation<std::is_copy_constructible<T>>>::value,
                        int>::type = 0>
        AnyLike(ValueType &&) {}  // NOLINT(runtime/explicit)

        AnyLike &operator=(AnyLike &&) = default;

        AnyLike &operator=(const AnyLike &) = default;

        template<typename ValueType,
                typename T = typename std::decay<ValueType>::type>
        typename std::enable_if<
                abel::conjunction<abel::negation<std::is_same<AnyLike, T>>,
                        std::is_copy_constructible<T>>::value,
                AnyLike &>::type
        operator=(ValueType && /* rhs */) {
            return *this;
        }
    };

    TEST(optionalTest, ConstructionConstraints) {
        EXPECT_TRUE((std::is_constructible<AnyLike, abel::optional<AnyLike>>::value));

        EXPECT_TRUE(
                (std::is_constructible<AnyLike, const abel::optional<AnyLike> &>::value));

        EXPECT_TRUE((std::is_constructible<abel::optional<AnyLike>, AnyLike>::value));
        EXPECT_TRUE(
                (std::is_constructible<abel::optional<AnyLike>, const AnyLike &>::value));

        EXPECT_TRUE((std::is_convertible<abel::optional<AnyLike>, AnyLike>::value));

        EXPECT_TRUE(
                (std::is_convertible<const abel::optional<AnyLike> &, AnyLike>::value));

        EXPECT_TRUE((std::is_convertible<AnyLike, abel::optional<AnyLike>>::value));
        EXPECT_TRUE(
                (std::is_convertible<const AnyLike &, abel::optional<AnyLike>>::value));

        EXPECT_TRUE(std::is_move_constructible<abel::optional<AnyLike>>::value);
        EXPECT_TRUE(std::is_copy_constructible<abel::optional<AnyLike>>::value);
    }

    TEST(optionalTest, AssignmentConstraints) {
        EXPECT_TRUE((std::is_assignable<AnyLike &, abel::optional<AnyLike>>::value));
        EXPECT_TRUE(
                (std::is_assignable<AnyLike &, const abel::optional<AnyLike> &>::value));
        EXPECT_TRUE((std::is_assignable<abel::optional<AnyLike> &, AnyLike>::value));
        EXPECT_TRUE(
                (std::is_assignable<abel::optional<AnyLike> &, const AnyLike &>::value));
        EXPECT_TRUE(std::is_move_assignable<abel::optional<AnyLike>>::value);
        EXPECT_TRUE(abel::is_copy_assignable<abel::optional<AnyLike>>::value);
    }

#if !defined(__EMSCRIPTEN__)
    struct NestedClassBug {
        struct Inner {
            bool dummy = false;
        };
        abel::optional<Inner> value;
    };

    TEST(optionalTest, InPlaceTSFINAEBug) {
        NestedClassBug b;
        ((void) b);
        using Inner = NestedClassBug::Inner;

        EXPECT_TRUE((std::is_default_constructible<Inner>::value));
        EXPECT_TRUE((std::is_constructible<Inner>::value));
        EXPECT_TRUE(
                (std::is_constructible<abel::optional<Inner>, abel::in_place_t>::value));

        abel::optional<Inner> o(abel::in_place);
        EXPECT_TRUE(o.has_value());
        o.emplace();
        EXPECT_TRUE(o.has_value());
    }

#endif  // !defined(__EMSCRIPTEN__)

}  // namespace

#endif  // #if !defined(ABEL_USES_STD_OPTIONAL)
