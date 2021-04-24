// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <array>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "abel/utility/span.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "abel/base/profile.h"
#include "testing/exception_testing.h"
#include "abel/container/fixed_array.h"
#include "abel/container/inlined_vector.h"
#include "testing/hash_testing.h"
#include "abel/strings/str_cat.h"

namespace {

MATCHER_P(DataIs, data,
          abel::string_cat("data() ", negation ? "isn't " : "is ",
                           testing::PrintToString(data))) {
    return arg.data() == data;
}

template<typename T>
auto SpanIs(T data, size_t size)
-> decltype(testing::AllOf(DataIs(data), testing::SizeIs(size))) {
    return testing::AllOf(DataIs(data), testing::SizeIs(size));
}

template<typename Container>
auto SpanIs(const Container &c) -> decltype(SpanIs(c.data(), c.size())) {
    return SpanIs(c.data(), c.size());
}

std::vector<int> MakeRamp(int len, int offset = 0) {
    std::vector<int> v(len);
    std::iota(v.begin(), v.end(), offset);
    return v;
}

TEST(IntSpan, EmptyCtors) {
    abel::span<int> s;
    EXPECT_THAT(s, SpanIs(nullptr, 0));
}

TEST(IntSpan, PtrLenCtor) {
    int a[] = {1, 2, 3};
    abel::span<int> s(&a[0], 2);
    EXPECT_THAT(s, SpanIs(a, 2));
}

TEST(IntSpan, ArrayCtor) {
    int a[] = {1, 2, 3};
    abel::span<int> s(a);
    EXPECT_THAT(s, SpanIs(a, 3));

    EXPECT_TRUE((std::is_constructible<abel::span<const int>, int[3]>::value));
    EXPECT_TRUE(
            (std::is_constructible<abel::span<const int>, const int[3]>::value));
    EXPECT_FALSE((std::is_constructible<abel::span<int>, const int[3]>::value));
    EXPECT_TRUE((std::is_convertible<int[3], abel::span<const int>>::value));
    EXPECT_TRUE(
            (std::is_convertible<const int[3], abel::span<const int>>::value));
}

template<typename T>
void TakesGenericSpan(abel::span<T>) {}

TEST(IntSpan, ContainerCtor) {
    std::vector<int> empty;
    abel::span<int> s_empty(empty);
    EXPECT_THAT(s_empty, SpanIs(empty));

    std::vector<int> filled{1, 2, 3};
    abel::span<int> s_filled(filled);
    EXPECT_THAT(s_filled, SpanIs(filled));

    abel::span<int> s_from_span(filled);
    EXPECT_THAT(s_from_span, SpanIs(s_filled));

    abel::span<const int> const_filled = filled;
    EXPECT_THAT(const_filled, SpanIs(filled));

    abel::span<const int> const_from_span = s_filled;
    EXPECT_THAT(const_from_span, SpanIs(s_filled));

    EXPECT_TRUE(
            (std::is_convertible<std::vector<int> &, abel::span<const int>>::value));
    EXPECT_TRUE(
            (std::is_convertible<abel::span<int> &, abel::span<const int>>::value));

    TakesGenericSpan(abel::span<int>(filled));
}

// A struct supplying shallow data() const.
struct ContainerWithShallowConstData {
    std::vector<int> storage;

    int *data() const { return const_cast<int *>(storage.data()); }

    int size() const { return storage.size(); }
};

TEST(IntSpan, ShallowConstness) {
    const ContainerWithShallowConstData c{MakeRamp(20)};
    abel::span<int> s(
            c);  // We should be able to do this even though data() is const.
    s[0] = -1;
    EXPECT_EQ(c.storage[0], -1);
}

TEST(CharSpan, StringCtor) {
    std::string empty = "";
    abel::span<char> s_empty(empty);
    EXPECT_THAT(s_empty, SpanIs(empty));

    std::string abc = "abc";
    abel::span<char> s_abc(abc);
    EXPECT_THAT(s_abc, SpanIs(abc));

    abel::span<const char> s_const_abc = abc;
    EXPECT_THAT(s_const_abc, SpanIs(abc));

    EXPECT_FALSE((std::is_constructible<abel::span<int>, std::string>::value));
    EXPECT_FALSE(
            (std::is_constructible<abel::span<const int>, std::string>::value));
    EXPECT_TRUE(
            (std::is_convertible<std::string, abel::span<const char>>::value));
}

TEST(IntSpan, FromConstPointer) {
    EXPECT_TRUE((std::is_constructible<abel::span<const int *const>,
            std::vector<int *>>::value));
    EXPECT_TRUE((std::is_constructible<abel::span<const int *const>,
            std::vector<const int *>>::value));
    EXPECT_FALSE((
                         std::is_constructible<abel::span<const int *>, std::vector<int *>>::value));
    EXPECT_FALSE((
                         std::is_constructible<abel::span<int *>, std::vector<const int *>>::value));
}

struct TypeWithMisleadingData {
    int &data() { return i; }

    int size() { return 1; }

    int i;
};

struct TypeWithMisleadingSize {
    int *data() { return &i; }

    const char *size() { return "1"; }

    int i;
};

TEST(IntSpan, EvilTypes) {
    EXPECT_FALSE(
            (std::is_constructible<abel::span<int>, TypeWithMisleadingData &>::value));
    EXPECT_FALSE(
            (std::is_constructible<abel::span<int>, TypeWithMisleadingSize &>::value));
}

struct Base {
    int *data() { return &i; }

    int size() { return 1; }

    int i;
};

struct Derived : Base {
};

TEST(IntSpan, SpanOfDerived) {
    EXPECT_TRUE((std::is_constructible<abel::span<int>, Base &>::value));
    EXPECT_TRUE((std::is_constructible<abel::span<int>, Derived &>::value));
    EXPECT_FALSE(
            (std::is_constructible<abel::span<Base>, std::vector<Derived>>::value));
}

void TestInitializerList(abel::span<const int> s, const std::vector<int> &v) {
    EXPECT_TRUE(abel::equal(s.begin(), s.end(), v.begin(), v.end()));
}

TEST(ConstIntSpan, InitializerListConversion) {
    TestInitializerList({}, {});
    TestInitializerList({1}, {1});
    TestInitializerList({1, 2, 3}, {1, 2, 3});

    EXPECT_FALSE((std::is_constructible<abel::span<int>,
            std::initializer_list<int>>::value));
    EXPECT_FALSE((
                         std::is_convertible<abel::span<int>, std::initializer_list<int>>::value));
}

TEST(IntSpan, Data) {
    int i;
    abel::span<int> s(&i, 1);
    EXPECT_EQ(&i, s.data());
}

TEST(IntSpan, SizeLengthEmpty) {
    abel::span<int> empty;
    EXPECT_EQ(empty.size(), 0);
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.size(), empty.length());

    auto v = MakeRamp(10);
    abel::span<int> s(v);
    EXPECT_EQ(s.size(), 10);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.size(), s.length());
}

TEST(IntSpan, ElementAccess) {
    auto v = MakeRamp(10);
    abel::span<int> s(v);
    for (size_t i = 0; i < s.size(); ++i) {
        EXPECT_EQ(s[i], s.at(i));
    }

    EXPECT_EQ(s.front(), s[0]);
    EXPECT_EQ(s.back(), s[9]);
}

TEST(IntSpan, AtThrows) {
    auto v = MakeRamp(10);
    abel::span<int> s(v);

    EXPECT_EQ(s.at(9), 9);
    ABEL_BASE_INTERNAL_EXPECT_FAIL(s.at(10), std::out_of_range,
                                   "failed bounds check");
}

TEST(IntSpan, RemovePrefixAndSuffix) {
    auto v = MakeRamp(20, 1);
    abel::span<int> s(v);
    EXPECT_EQ(s.size(), 20);

    s.remove_suffix(0);
    s.remove_prefix(0);
    EXPECT_EQ(s.size(), 20);

    s.remove_prefix(1);
    EXPECT_EQ(s.size(), 19);
    EXPECT_EQ(s[0], 2);

    s.remove_suffix(1);
    EXPECT_EQ(s.size(), 18);
    EXPECT_EQ(s.back(), 19);

    s.remove_prefix(7);
    EXPECT_EQ(s.size(), 11);
    EXPECT_EQ(s[0], 9);

    s.remove_suffix(11);
    EXPECT_EQ(s.size(), 0);

    EXPECT_EQ(v, MakeRamp(20, 1));
}

TEST(IntSpan, Subspan) {
    std::vector<int> empty;
    EXPECT_EQ(abel::make_span(empty).subspan(), empty);
    EXPECT_THAT(abel::make_span(empty).subspan(0, 0), SpanIs(empty));
    EXPECT_THAT(abel::make_span(empty).subspan(0, abel::span<const int>::npos),
                SpanIs(empty));

    auto ramp = MakeRamp(10);
    EXPECT_THAT(abel::make_span(ramp).subspan(), SpanIs(ramp));
    EXPECT_THAT(abel::make_span(ramp).subspan(0, 10), SpanIs(ramp));
    EXPECT_THAT(abel::make_span(ramp).subspan(0, abel::span<const int>::npos),
                SpanIs(ramp));
    EXPECT_THAT(abel::make_span(ramp).subspan(0, 3), SpanIs(ramp.data(), 3));
    EXPECT_THAT(abel::make_span(ramp).subspan(5, abel::span<const int>::npos),
                SpanIs(ramp.data() + 5, 5));
    EXPECT_THAT(abel::make_span(ramp).subspan(3, 3), SpanIs(ramp.data() + 3, 3));
    EXPECT_THAT(abel::make_span(ramp).subspan(10, 5), SpanIs(ramp.data() + 10, 0));

#ifdef ABEL_HAVE_EXCEPTIONS
    EXPECT_THROW(abel::make_span(ramp).subspan(11, 5), std::out_of_range);
#else
    EXPECT_DEATH_IF_SUPPORTED(abel::make_span(ramp).subspan(11, 5), "");
#endif
}

TEST(IntSpan, First) {
    std::vector<int> empty;
    EXPECT_THAT(abel::make_span(empty).first(0), SpanIs(empty));

    auto ramp = MakeRamp(10);
    EXPECT_THAT(abel::make_span(ramp).first(0), SpanIs(ramp.data(), 0));
    EXPECT_THAT(abel::make_span(ramp).first(10), SpanIs(ramp));
    EXPECT_THAT(abel::make_span(ramp).first(3), SpanIs(ramp.data(), 3));

#ifdef ABEL_HAVE_EXCEPTIONS
    EXPECT_THROW(abel::make_span(ramp).first(11), std::out_of_range);
#else
    EXPECT_DEATH_IF_SUPPORTED(abel::make_span(ramp).first(11), "");
#endif
}

TEST(IntSpan, Last) {
    std::vector<int> empty;
    EXPECT_THAT(abel::make_span(empty).last(0), SpanIs(empty));

    auto ramp = MakeRamp(10);
    EXPECT_THAT(abel::make_span(ramp).last(0), SpanIs(ramp.data() + 10, 0));
    EXPECT_THAT(abel::make_span(ramp).last(10), SpanIs(ramp));
    EXPECT_THAT(abel::make_span(ramp).last(3), SpanIs(ramp.data() + 7, 3));

#ifdef ABEL_HAVE_EXCEPTIONS
    EXPECT_THROW(abel::make_span(ramp).last(11), std::out_of_range);
#else
    EXPECT_DEATH_IF_SUPPORTED(abel::make_span(ramp).last(11), "");
#endif
}

TEST(IntSpan, make_spanPtrLength) {
    std::vector<int> empty;
    auto s_empty = abel::make_span(empty.data(), empty.size());
    EXPECT_THAT(s_empty, SpanIs(empty));

    std::array<int, 3> a{{1, 2, 3}};
    auto s = abel::make_span(a.data(), a.size());
    EXPECT_THAT(s, SpanIs(a));

    EXPECT_THAT(abel::make_const_span(empty.data(), empty.size()), SpanIs(s_empty));
    EXPECT_THAT(abel::make_const_span(a.data(), a.size()), SpanIs(s));
}

TEST(IntSpan, make_spanTwoPtrs) {
    std::vector<int> empty;
    auto s_empty = abel::make_span(empty.data(), empty.data());
    EXPECT_THAT(s_empty, SpanIs(empty));

    std::vector<int> v{1, 2, 3};
    auto s = abel::make_span(v.data(), v.data() + 1);
    EXPECT_THAT(s, SpanIs(v.data(), 1));

    EXPECT_THAT(abel::make_const_span(empty.data(), empty.data()), SpanIs(s_empty));
    EXPECT_THAT(abel::make_const_span(v.data(), v.data() + 1), SpanIs(s));
}

TEST(IntSpan, make_spanContainer) {
    std::vector<int> empty;
    auto s_empty = abel::make_span(empty);
    EXPECT_THAT(s_empty, SpanIs(empty));

    std::vector<int> v{1, 2, 3};
    auto s = abel::make_span(v);
    EXPECT_THAT(s, SpanIs(v));

    EXPECT_THAT(abel::make_const_span(empty), SpanIs(s_empty));
    EXPECT_THAT(abel::make_const_span(v), SpanIs(s));

    EXPECT_THAT(abel::make_span(s), SpanIs(s));
    EXPECT_THAT(abel::make_const_span(s), SpanIs(s));
}

TEST(CharSpan, make_spanString) {
    std::string empty = "";
    auto s_empty = abel::make_span(empty);
    EXPECT_THAT(s_empty, SpanIs(empty));

    std::string str = "abc";
    auto s_str = abel::make_span(str);
    EXPECT_THAT(s_str, SpanIs(str));

    EXPECT_THAT(abel::make_const_span(empty), SpanIs(s_empty));
    EXPECT_THAT(abel::make_const_span(str), SpanIs(s_str));
}

TEST(IntSpan, make_spanArray) {
    int a[] = {1, 2, 3};
    auto s = abel::make_span(a);
    EXPECT_THAT(s, SpanIs(a, 3));

    const int ca[] = {1, 2, 3};
    auto s_ca = abel::make_span(ca);
    EXPECT_THAT(s_ca, SpanIs(ca, 3));

    EXPECT_THAT(abel::make_const_span(a), SpanIs(s));
    EXPECT_THAT(abel::make_const_span(ca), SpanIs(s_ca));
}

// Compile-asserts that the argument has the expected decayed type.
template<typename Expected, typename T>
void CheckType(const T & /* value */) {
    testing::StaticAssertTypeEq<Expected, T>();
}

TEST(IntSpan, make_spanTypes) {
    std::vector<int> vec;
    const std::vector<int> cvec;
    int a[1];
    const int ca[] = {1};
    int *ip = a;
    const int *cip = ca;
    std::string s = "";
    const std::string cs = "";
    CheckType<abel::span<int>>(abel::make_span(vec));
    CheckType<abel::span<const int>>(abel::make_span(cvec));
    CheckType<abel::span<int>>(abel::make_span(ip, ip + 1));
    CheckType<abel::span<int>>(abel::make_span(ip, 1));
    CheckType<abel::span<const int>>(abel::make_span(cip, cip + 1));
    CheckType<abel::span<const int>>(abel::make_span(cip, 1));
    CheckType<abel::span<int>>(abel::make_span(a));
    CheckType<abel::span<int>>(abel::make_span(a, a + 1));
    CheckType<abel::span<int>>(abel::make_span(a, 1));
    CheckType<abel::span<const int>>(abel::make_span(ca));
    CheckType<abel::span<const int>>(abel::make_span(ca, ca + 1));
    CheckType<abel::span<const int>>(abel::make_span(ca, 1));
    CheckType<abel::span<char>>(abel::make_span(s));
    CheckType<abel::span<const char>>(abel::make_span(cs));
}

TEST(ConstIntSpan, make_const_spanTypes) {
    std::vector<int> vec;
    const std::vector<int> cvec;
    int array[1];
    const int carray[] = {0};
    int *ptr = array;
    const int *cptr = carray;
    std::string s = "";
    std::string cs = "";
    CheckType<abel::span<const int>>(abel::make_const_span(vec));
    CheckType<abel::span<const int>>(abel::make_const_span(cvec));
    CheckType<abel::span<const int>>(abel::make_const_span(ptr, ptr + 1));
    CheckType<abel::span<const int>>(abel::make_const_span(ptr, 1));
    CheckType<abel::span<const int>>(abel::make_const_span(cptr, cptr + 1));
    CheckType<abel::span<const int>>(abel::make_const_span(cptr, 1));
    CheckType<abel::span<const int>>(abel::make_const_span(array));
    CheckType<abel::span<const int>>(abel::make_const_span(carray));
    CheckType<abel::span<const char>>(abel::make_const_span(s));
    CheckType<abel::span<const char>>(abel::make_const_span(cs));
}

TEST(IntSpan, Equality) {
    const int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {1, 2, 3, 4, 5};
    std::vector<int> vec1(std::begin(arr1), std::end(arr1));
    std::vector<int> vec2 = vec1;
    std::vector<int> other_vec = {2, 4, 6, 8, 10};
    // These two slices are from different vectors, but have the same size and
    // have the same elements (right now).  They should compare equal. Test both
    // == and !=.
    const abel::span<const int> from1 = vec1;
    const abel::span<const int> from2 = vec2;
    EXPECT_EQ(from1, from1);
    EXPECT_FALSE(from1 != from1);
    EXPECT_EQ(from1, from2);
    EXPECT_FALSE(from1 != from2);

    // These two slices have different underlying vector values. They should be
    // considered not equal. Test both == and !=.
    const abel::span<const int> from_other = other_vec;
    EXPECT_NE(from1, from_other);
    EXPECT_FALSE(from1 == from_other);

    // Comparison between a vector and its slice should be equal. And vice-versa.
    // This ensures implicit conversion to span works on both sides of ==.
    EXPECT_EQ(vec1, from1);
    EXPECT_FALSE(vec1 != from1);
    EXPECT_EQ(from1, vec1);
    EXPECT_FALSE(from1 != vec1);

    // This verifies that abel::span<T> can be compared freely with
    // abel::span<const T>.
    const abel::span<int> mutable_from1(vec1);
    const abel::span<int> mutable_from2(vec2);
    EXPECT_EQ(from1, mutable_from1);
    EXPECT_EQ(mutable_from1, from1);
    EXPECT_EQ(mutable_from1, mutable_from2);
    EXPECT_EQ(mutable_from2, mutable_from1);

    // Comparison between a vector and its slice should be equal for mutable
    // Spans as well.
    EXPECT_EQ(vec1, mutable_from1);
    EXPECT_FALSE(vec1 != mutable_from1);
    EXPECT_EQ(mutable_from1, vec1);
    EXPECT_FALSE(mutable_from1 != vec1);

    // Comparison between convertible-to-span-of-const and span-of-mutable. Arrays
    // are used because they're the only value type which converts to a
    // span-of-mutable. EXPECT_TRUE is used instead of EXPECT_EQ to avoid
    // array-to-pointer decay.
    EXPECT_TRUE(arr1 == mutable_from1);
    EXPECT_FALSE(arr1 != mutable_from1);
    EXPECT_TRUE(mutable_from1 == arr1);
    EXPECT_FALSE(mutable_from1 != arr1);

    // Comparison between convertible-to-span-of-mutable and span-of-const
    EXPECT_TRUE(arr2 == from1);
    EXPECT_FALSE(arr2 != from1);
    EXPECT_TRUE(from1 == arr2);
    EXPECT_FALSE(from1 != arr2);

    // With a different size, the array slices should not be equal.
    EXPECT_NE(from1, abel::span<const int>(from1).subspan(0, from1.size() - 1));

    // With different contents, the array slices should not be equal.
    ++vec2.back();
    EXPECT_NE(from1, from2);
}

class IntSpanOrderComparisonTest : public testing::Test {
  public:
    IntSpanOrderComparisonTest()
            : arr_before_{1, 2, 3},
              arr_after_{1, 2, 4},
              carr_after_{1, 2, 4},
              vec_before_(std::begin(arr_before_), std::end(arr_before_)),
              vec_after_(std::begin(arr_after_), std::end(arr_after_)),
              before_(vec_before_),
              after_(vec_after_),
              cbefore_(vec_before_),
              cafter_(vec_after_) {}

  protected:
    int arr_before_[3], arr_after_[3];
    const int carr_after_[3];
    std::vector<int> vec_before_, vec_after_;
    abel::span<int> before_, after_;
    abel::span<const int> cbefore_, cafter_;
};

TEST_F(IntSpanOrderComparisonTest, CompareSpans) {
    EXPECT_TRUE(cbefore_ < cafter_);
    EXPECT_TRUE(cbefore_ <= cafter_);
    EXPECT_TRUE(cafter_ > cbefore_);
    EXPECT_TRUE(cafter_ >= cbefore_);

    EXPECT_FALSE(cbefore_ > cafter_);
    EXPECT_FALSE(cafter_ < cbefore_);

    EXPECT_TRUE(before_ < after_);
    EXPECT_TRUE(before_ <= after_);
    EXPECT_TRUE(after_ > before_);
    EXPECT_TRUE(after_ >= before_);

    EXPECT_FALSE(before_ > after_);
    EXPECT_FALSE(after_ < before_);

    EXPECT_TRUE(cbefore_ < after_);
    EXPECT_TRUE(cbefore_ <= after_);
    EXPECT_TRUE(after_ > cbefore_);
    EXPECT_TRUE(after_ >= cbefore_);

    EXPECT_FALSE(cbefore_ > after_);
    EXPECT_FALSE(after_ < cbefore_);
}

TEST_F(IntSpanOrderComparisonTest, SpanOfConstAndContainer) {
    EXPECT_TRUE(cbefore_ < vec_after_);
    EXPECT_TRUE(cbefore_ <= vec_after_);
    EXPECT_TRUE(vec_after_ > cbefore_);
    EXPECT_TRUE(vec_after_ >= cbefore_);

    EXPECT_FALSE(cbefore_ > vec_after_);
    EXPECT_FALSE(vec_after_ < cbefore_);

    EXPECT_TRUE(arr_before_ < cafter_);
    EXPECT_TRUE(arr_before_ <= cafter_);
    EXPECT_TRUE(cafter_ > arr_before_);
    EXPECT_TRUE(cafter_ >= arr_before_);

    EXPECT_FALSE(arr_before_ > cafter_);
    EXPECT_FALSE(cafter_ < arr_before_);
}

TEST_F(IntSpanOrderComparisonTest, SpanOfMutableAndContainer) {
    EXPECT_TRUE(vec_before_ < after_);
    EXPECT_TRUE(vec_before_ <= after_);
    EXPECT_TRUE(after_ > vec_before_);
    EXPECT_TRUE(after_ >= vec_before_);

    EXPECT_FALSE(vec_before_ > after_);
    EXPECT_FALSE(after_ < vec_before_);

    EXPECT_TRUE(before_ < carr_after_);
    EXPECT_TRUE(before_ <= carr_after_);
    EXPECT_TRUE(carr_after_ > before_);
    EXPECT_TRUE(carr_after_ >= before_);

    EXPECT_FALSE(before_ > carr_after_);
    EXPECT_FALSE(carr_after_ < before_);
}

TEST_F(IntSpanOrderComparisonTest, EqualSpans) {
    EXPECT_FALSE(before_ < before_);
    EXPECT_TRUE(before_ <= before_);
    EXPECT_FALSE(before_ > before_);
    EXPECT_TRUE(before_ >= before_);
}

TEST_F(IntSpanOrderComparisonTest, Subspans) {
    auto subspan = before_.subspan(0, 1);
    EXPECT_TRUE(subspan < before_);
    EXPECT_TRUE(subspan <= before_);
    EXPECT_TRUE(before_ > subspan);
    EXPECT_TRUE(before_ >= subspan);

    EXPECT_FALSE(subspan > before_);
    EXPECT_FALSE(before_ < subspan);
}

TEST_F(IntSpanOrderComparisonTest, EmptySpans) {
    abel::span<int> empty;
    EXPECT_FALSE(empty < empty);
    EXPECT_TRUE(empty <= empty);
    EXPECT_FALSE(empty > empty);
    EXPECT_TRUE(empty >= empty);

    EXPECT_TRUE(empty < before_);
    EXPECT_TRUE(empty <= before_);
    EXPECT_TRUE(before_ > empty);
    EXPECT_TRUE(before_ >= empty);

    EXPECT_FALSE(empty > before_);
    EXPECT_FALSE(before_ < empty);
}

TEST(IntSpan, ExposesContainerTypesAndConsts) {
    abel::span<int> slice;
    CheckType<abel::span<int>::iterator>(slice.begin());
    EXPECT_TRUE((std::is_convertible<decltype(slice.begin()),
            abel::span<int>::const_iterator>::value));
    CheckType<abel::span<int>::const_iterator>(slice.cbegin());
    EXPECT_TRUE((std::is_convertible<decltype(slice.end()),
            abel::span<int>::const_iterator>::value));
    CheckType<abel::span<int>::const_iterator>(slice.cend());
    CheckType<abel::span<int>::reverse_iterator>(slice.rend());
    EXPECT_TRUE(
            (std::is_convertible<decltype(slice.rend()),
                    abel::span<int>::const_reverse_iterator>::value));
    CheckType<abel::span<int>::const_reverse_iterator>(slice.crend());
    testing::StaticAssertTypeEq<int, abel::span<int>::value_type>();
    testing::StaticAssertTypeEq<int, abel::span<const int>::value_type>();
    testing::StaticAssertTypeEq<int *, abel::span<int>::pointer>();
    testing::StaticAssertTypeEq<const int *, abel::span<const int>::pointer>();
    testing::StaticAssertTypeEq<int &, abel::span<int>::reference>();
    testing::StaticAssertTypeEq<const int &, abel::span<const int>::reference>();
    testing::StaticAssertTypeEq<const int &, abel::span<int>::const_reference>();
    testing::StaticAssertTypeEq<const int &,
            abel::span<const int>::const_reference>();
    EXPECT_EQ(static_cast<abel::span<int>::size_type>(-1), abel::span<int>::npos);
}

TEST(IntSpan, IteratorsAndReferences) {
    auto accept_pointer = [](int *) {};
    auto accept_reference = [](int &) {};
    auto accept_iterator = [](abel::span<int>::iterator) {};
    auto accept_const_iterator = [](abel::span<int>::const_iterator) {};
    auto accept_reverse_iterator = [](abel::span<int>::reverse_iterator) {};
    auto accept_const_reverse_iterator =
            [](abel::span<int>::const_reverse_iterator) {};

    int a[1];
    abel::span<int> s = a;

    accept_pointer(s.data());
    accept_iterator(s.begin());
    accept_const_iterator(s.begin());
    accept_const_iterator(s.cbegin());
    accept_iterator(s.end());
    accept_const_iterator(s.end());
    accept_const_iterator(s.cend());
    accept_reverse_iterator(s.rbegin());
    accept_const_reverse_iterator(s.rbegin());
    accept_const_reverse_iterator(s.crbegin());
    accept_reverse_iterator(s.rend());
    accept_const_reverse_iterator(s.rend());
    accept_const_reverse_iterator(s.crend());

    accept_reference(s[0]);
    accept_reference(s.at(0));
    accept_reference(s.front());
    accept_reference(s.back());
}

TEST(IntSpan, IteratorsAndReferences_Const) {
    auto accept_pointer = [](int *) {};
    auto accept_reference = [](int &) {};
    auto accept_iterator = [](abel::span<int>::iterator) {};
    auto accept_const_iterator = [](abel::span<int>::const_iterator) {};
    auto accept_reverse_iterator = [](abel::span<int>::reverse_iterator) {};
    auto accept_const_reverse_iterator =
            [](abel::span<int>::const_reverse_iterator) {};

    int a[1];
    const abel::span<int> s = a;

    accept_pointer(s.data());
    accept_iterator(s.begin());
    accept_const_iterator(s.begin());
    accept_const_iterator(s.cbegin());
    accept_iterator(s.end());
    accept_const_iterator(s.end());
    accept_const_iterator(s.cend());
    accept_reverse_iterator(s.rbegin());
    accept_const_reverse_iterator(s.rbegin());
    accept_const_reverse_iterator(s.crbegin());
    accept_reverse_iterator(s.rend());
    accept_const_reverse_iterator(s.rend());
    accept_const_reverse_iterator(s.crend());

    accept_reference(s[0]);
    accept_reference(s.at(0));
    accept_reference(s.front());
    accept_reference(s.back());
}

TEST(IntSpan, NoexceptTest) {
    int a[] = {1, 2, 3};
    std::vector<int> v;
    EXPECT_TRUE(noexcept(abel::span<const int>()));
    EXPECT_TRUE(noexcept(abel::span<const int>(a, 2)));
    EXPECT_TRUE(noexcept(abel::span<const int>(a)));
    EXPECT_TRUE(noexcept(abel::span<const int>(v)));
    EXPECT_TRUE(noexcept(abel::span<int>(v)));
    EXPECT_TRUE(noexcept(abel::span<const int>({1, 2, 3})));
    EXPECT_TRUE(noexcept(abel::make_span(v)));
    EXPECT_TRUE(noexcept(abel::make_span(a)));
    EXPECT_TRUE(noexcept(abel::make_span(a, 2)));
    EXPECT_TRUE(noexcept(abel::make_span(a, a + 1)));
    EXPECT_TRUE(noexcept(abel::make_const_span(v)));
    EXPECT_TRUE(noexcept(abel::make_const_span(a)));
    EXPECT_TRUE(noexcept(abel::make_const_span(a, 2)));
    EXPECT_TRUE(noexcept(abel::make_const_span(a, a + 1)));

    abel::span<int> s(v);
    EXPECT_TRUE(noexcept(s.data()));
    EXPECT_TRUE(noexcept(s.size()));
    EXPECT_TRUE(noexcept(s.length()));
    EXPECT_TRUE(noexcept(s.empty()));
    EXPECT_TRUE(noexcept(s[0]));
    EXPECT_TRUE(noexcept(s.front()));
    EXPECT_TRUE(noexcept(s.back()));
    EXPECT_TRUE(noexcept(s.begin()));
    EXPECT_TRUE(noexcept(s.cbegin()));
    EXPECT_TRUE(noexcept(s.end()));
    EXPECT_TRUE(noexcept(s.cend()));
    EXPECT_TRUE(noexcept(s.rbegin()));
    EXPECT_TRUE(noexcept(s.crbegin()));
    EXPECT_TRUE(noexcept(s.rend()));
    EXPECT_TRUE(noexcept(s.crend()));
    EXPECT_TRUE(noexcept(s.remove_prefix(0)));
    EXPECT_TRUE(noexcept(s.remove_suffix(0)));
}

// ConstexprTester exercises expressions in a constexpr context. Simply placing
// the expression in a constexpr function is not enough, as some compilers will
// simply compile the constexpr function as runtime code. Using template
// parameters forces compile-time execution.
template<int i>
struct ConstexprTester {
};

#define ABEL_TEST_CONSTEXPR(expr)                       \
  do {                                                  \
    ABEL_ATTRIBUTE_UNUSED ConstexprTester<(expr, 1)> t; \
  } while (0)

struct ContainerWithConstexprMethods {
    constexpr int size() const { return 1; }

    constexpr const int *data() const { return &i; }

    const int i;
};

TEST(ConstIntSpan, ConstexprTest) {
    static constexpr int a[] = {1, 2, 3};
    static constexpr int sized_arr[2] = {1, 2};
    static constexpr ContainerWithConstexprMethods c{1};
    ABEL_TEST_CONSTEXPR(abel::span<const int>());
    ABEL_TEST_CONSTEXPR(abel::span<const int>(a, 2));
    ABEL_TEST_CONSTEXPR(abel::span<const int>(sized_arr));
    ABEL_TEST_CONSTEXPR(abel::span<const int>(c));
    ABEL_TEST_CONSTEXPR(abel::make_span(&a[0], 1));
    ABEL_TEST_CONSTEXPR(abel::make_span(c));
    ABEL_TEST_CONSTEXPR(abel::make_span(a));
    ABEL_TEST_CONSTEXPR(abel::make_const_span(&a[0], 1));
    ABEL_TEST_CONSTEXPR(abel::make_const_span(c));
    ABEL_TEST_CONSTEXPR(abel::make_const_span(a));

    constexpr abel::span<const int> span = c;
    ABEL_TEST_CONSTEXPR(span.data());
    ABEL_TEST_CONSTEXPR(span.size());
    ABEL_TEST_CONSTEXPR(span.length());
    ABEL_TEST_CONSTEXPR(span.empty());
    ABEL_TEST_CONSTEXPR(span.begin());
    ABEL_TEST_CONSTEXPR(span.cbegin());
    ABEL_TEST_CONSTEXPR(span.subspan(0, 0));
    ABEL_TEST_CONSTEXPR(span.first(1));
    ABEL_TEST_CONSTEXPR(span.last(1));
    ABEL_TEST_CONSTEXPR(span[0]);
}

struct BigStruct {
    char bytes[10000];
};

TEST(span, SpanSize) {
    EXPECT_LE(sizeof(abel::span<int>), 2 * sizeof(void *));
    EXPECT_LE(sizeof(abel::span<BigStruct>), 2 * sizeof(void *));
}

TEST(span, Hash) {
    int array[] = {1, 2, 3, 4};
    int array2[] = {1, 2, 3};
    using T = abel::span<const int>;
    EXPECT_TRUE(abel::VerifyTypeImplementsAbelHashCorrectly(
            {// Empties
                    T(), T(nullptr, 0), T(array, 0), T(array2, 0),
                    // Different array with same value
                    T(array, 3), T(array2), T({1, 2, 3}),
                    // Same array, but different length
                    T(array, 1), T(array, 2),
                    // Same length, but different array
                    T(array + 1, 2), T(array + 2, 2)}));
}

}  // namespace
