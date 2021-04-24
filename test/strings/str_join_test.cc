// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

// Unit tests for all join.h functions

#include "abel/strings/str_join.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"
#include "abel/base/profile.h"
#include "abel/memory/memory.h"
#include "abel/strings/str_cat.h"
#include "abel/strings/str_split.h"

namespace {

    TEST(string_join, APIExamples) {
        {
            // Collection of strings
            std::vector<std::string> v = {"foo", "bar", "baz"};
            EXPECT_EQ("foo-bar-baz", abel::string_join(v, "-"));
        }

        {
            // Collection of std::string_view
            std::vector<std::string_view> v = {"foo", "bar", "baz"};
            EXPECT_EQ("foo-bar-baz", abel::string_join(v, "-"));
        }

        {
            // Collection of const char*
            std::vector<const char *> v = {"foo", "bar", "baz"};
            EXPECT_EQ("foo-bar-baz", abel::string_join(v, "-"));
        }

        {
            // Collection of non-const char*
            std::string a = "foo", b = "bar", c = "baz";
            std::vector<char *> v = {&a[0], &b[0], &c[0]};
            EXPECT_EQ("foo-bar-baz", abel::string_join(v, "-"));
        }

        {
            // Collection of ints
            std::vector<int> v = {1, 2, 3, -4};
            EXPECT_EQ("1-2-3--4", abel::string_join(v, "-"));
        }

        {
            // Literals passed as a std::initializer_list
            std::string s = abel::string_join({"a", "b", "c"}, "-");
            EXPECT_EQ("a-b-c", s);
        }
        {
            // Join a std::tuple<T...>.
            std::string s = abel::string_join(std::make_tuple(123, "abc", 0.456), "-");
            EXPECT_EQ("123-abc-0.456", s);
        }

        {
            // Collection of unique_ptrs
            std::vector<std::unique_ptr<int>> v;
            v.emplace_back(new int(1));
            v.emplace_back(new int(2));
            v.emplace_back(new int(3));
            EXPECT_EQ("1-2-3", abel::string_join(v, "-"));
        }

        {
            // Array of ints
            const int a[] = {1, 2, 3, -4};
            EXPECT_EQ("1-2-3--4", abel::string_join(a, a + ABEL_ARRAY_SIZE(a), "-"));
        }

        {
            // Collection of pointers
            int x = 1, y = 2, z = 3;
            std::vector<int *> v = {&x, &y, &z};
            EXPECT_EQ("1-2-3", abel::string_join(v, "-"));
        }

        {
            // Collection of pointers to pointers
            int x = 1, y = 2, z = 3;
            int *px = &x, *py = &y, *pz = &z;
            std::vector<int **> v = {&px, &py, &pz};
            EXPECT_EQ("1-2-3", abel::string_join(v, "-"));
        }

        {
            // Collection of pointers to std::string
            std::string a("a"), b("b");
            std::vector<std::string *> v = {&a, &b};
            EXPECT_EQ("a-b", abel::string_join(v, "-"));
        }

        {
            // A std::map, which is a collection of std::pair<>s.
            std::map<std::string, int> m = {{"a", 1},
                                            {"b", 2},
                                            {"c", 3}};
            EXPECT_EQ("a=1,b=2,c=3", abel::string_join(m, ",", abel::pair_formatter("=")));
        }

        {
            // Shows abel:: string_split and abel::string_join working together. This example is
            // equivalent to s/=/-/g.
            const std::string s = "a=b=c=d";
            EXPECT_EQ("a-b-c-d", abel::string_join(abel::string_split(s, "="), "-"));
        }

        //
        // A few examples of edge cases
        //

        {
            // Empty range yields an empty std::string.
            std::vector<std::string> v;
            EXPECT_EQ("", abel::string_join(v, "-"));
        }

        {
            // A range of 1 element gives a std::string with that element but no
            // separator.
            std::vector<std::string> v = {"foo"};
            EXPECT_EQ("foo", abel::string_join(v, "-"));
        }

        {
            // A range with a single empty std::string element
            std::vector<std::string> v = {""};
            EXPECT_EQ("", abel::string_join(v, "-"));
        }

        {
            // A range with 2 elements, one of which is an empty std::string
            std::vector<std::string> v = {"a", ""};
            EXPECT_EQ("a-", abel::string_join(v, "-"));
        }

        {
            // A range with 2 empty elements.
            std::vector<std::string> v = {"", ""};
            EXPECT_EQ("-", abel::string_join(v, "-"));
        }

        {
            // A std::vector of bool.
            std::vector<bool> v = {true, false, true};
            EXPECT_EQ("1-0-1", abel::string_join(v, "-"));
        }
    }

    TEST(string_join, CustomFormatter) {
        std::vector<std::string> v{"One", "Two", "Three"};
        {
            std::string joined =
                    abel::string_join(v, "", [](std::string *out, const std::string &in) {
                        abel::string_append(out, "(", in, ")");
                    });
            EXPECT_EQ("(One)(Two)(Three)", joined);
        }
        {
            class ImmovableFormatter {
            public:
                void operator()(std::string *out, const std::string &in) {
                    abel::string_append(out, "(", in, ")");
                }

                ImmovableFormatter() {}

                ImmovableFormatter(const ImmovableFormatter &) = delete;
            };
            EXPECT_EQ("(One)(Two)(Three)", abel::string_join(v, "", ImmovableFormatter()));
        }
        {
            class OverloadedFormatter {
            public:
                void operator()(std::string *out, const std::string &in) {
                    abel::string_append(out, "(", in, ")");
                }

                void operator()(std::string *out, const std::string &in) const {
                    abel::string_append(out, "[", in, "]");
                }
            };
            EXPECT_EQ("(One)(Two)(Three)", abel::string_join(v, "", OverloadedFormatter()));
            const OverloadedFormatter fmt = {};
            EXPECT_EQ("[One][Two][Three]", abel::string_join(v, "", fmt));
        }
    }

//
// Tests the Formatters
//

    TEST(alpha_num_formatter, FormatterAPI) {
        // Not an exhaustive test. See strings/strcat_test.h for the exhaustive test
        // of what alpha_num can convert.
        auto f = abel::alpha_num_formatter();
        std::string s;
        f(&s, "Testing: ");
        f(&s, static_cast<int>(1));
        f(&s, static_cast<int16_t>(2));
        f(&s, static_cast<int64_t>(3));
        f(&s, static_cast<float>(4));
        f(&s, static_cast<double>(5));
        f(&s, static_cast<unsigned>(6));
        f(&s, static_cast<size_t>(7));
        f(&s, std::string_view(" OK"));
        EXPECT_EQ("Testing: 1234567 OK", s);
    }

// Make sure people who are mistakenly using std::vector<bool> even though
// they're not memory-constrained can use abel::alpha_num_formatter().
    TEST(alpha_num_formatter, VectorOfBool) {
        auto f = abel::alpha_num_formatter();
        std::string s;
        std::vector<bool> v = {true, false, true};
        f(&s, *v.cbegin());
        f(&s, *v.begin());
        f(&s, v[1]);
        EXPECT_EQ("110", s);
    }

    TEST(alpha_num_formatter, alpha_num) {
        auto f = abel::alpha_num_formatter();
        std::string s;
        f(&s, abel::alpha_num("hello"));
        EXPECT_EQ("hello", s);
    }

    struct StreamableType {
        std::string contents;
    };

    inline std::ostream &operator<<(std::ostream &os, const StreamableType &t) {
        os << "Streamable:" << t.contents;
        return os;
    }

    TEST(stream_formatter, FormatterAPI) {
        auto f = abel::stream_formatter();
        std::string s;
        f(&s, "Testing: ");
        f(&s, static_cast<int>(1));
        f(&s, static_cast<int16_t>(2));
        f(&s, static_cast<int64_t>(3));
        f(&s, static_cast<float>(4));
        f(&s, static_cast<double>(5));
        f(&s, static_cast<unsigned>(6));
        f(&s, static_cast<size_t>(7));
        f(&s, std::string_view(" OK "));
        StreamableType streamable = {"object"};
        f(&s, streamable);
        EXPECT_EQ("Testing: 1234567 OK Streamable:object", s);
    }

// A dummy formatter that wraps each element in parens. Used in some tests
// below.
    struct TestingParenFormatter {
        template<typename T>
        void operator()(std::string *s, const T &t) {
            abel::string_append(s, "(", t, ")");
        }
    };

    TEST(pair_formatter, FormatterAPI) {
        {
            // Tests default pair_formatter(sep) that uses alpha_num_formatter for the
            // 'first' and 'second' members.
            const auto f = abel::pair_formatter("=");
            std::string s;
            f(&s, std::make_pair("a", "b"));
            f(&s, std::make_pair(1, 2));
            EXPECT_EQ("a=b1=2", s);
        }

        {
            // Tests using a custom formatter for the 'first' and 'second' members.
            auto f = abel::pair_formatter(TestingParenFormatter(), "=",
                                          TestingParenFormatter());
            std::string s;
            f(&s, std::make_pair("a", "b"));
            f(&s, std::make_pair(1, 2));
            EXPECT_EQ("(a)=(b)(1)=(2)", s);
        }
    }

    TEST(dereference_formatter, FormatterAPI) {
        {
            // Tests wrapping the default alpha_num_formatter.
            const abel::strings_internal::dereference_formatter_impl<
                    abel::strings_internal::alpha_num_formatter_impl>
                    f;
            int x = 1, y = 2, z = 3;
            std::string s;
            f(&s, &x);
            f(&s, &y);
            f(&s, &z);
            EXPECT_EQ("123", s);
        }

        {
            // Tests wrapping std::string's default formatter.
            abel::strings_internal::dereference_formatter_impl<
                    abel::strings_internal::default_formatter<std::string>::Type>
                    f;

            std::string x = "x";
            std::string y = "y";
            std::string z = "z";
            std::string s;
            f(&s, &x);
            f(&s, &y);
            f(&s, &z);
            EXPECT_EQ(s, "xyz");
        }

        {
            // Tests wrapping a custom formatter.
            auto f = abel::dereference_formatter(TestingParenFormatter());
            int x = 1, y = 2, z = 3;
            std::string s;
            f(&s, &x);
            f(&s, &y);
            f(&s, &z);
            EXPECT_EQ("(1)(2)(3)", s);
        }

        {
            abel::strings_internal::dereference_formatter_impl<
                    abel::strings_internal::alpha_num_formatter_impl>
                    f;
            auto x = std::unique_ptr<int>(new int(1));
            auto y = std::unique_ptr<int>(new int(2));
            auto z = std::unique_ptr<int>(new int(3));
            std::string s;
            f(&s, x);
            f(&s, y);
            f(&s, z);
            EXPECT_EQ("123", s);
        }
    }

//
// Tests the interfaces for the 4 public Join function overloads. The semantics
// of the algorithm is covered in the above APIExamples test.
//
    TEST(string_join, PublicAPIOverloads) {
        std::vector<std::string> v = {"a", "b", "c"};

        // Iterators + formatter
        EXPECT_EQ("a-b-c",
                  abel::string_join(v.begin(), v.end(), "-", abel::alpha_num_formatter()));
        // Range + formatter
        EXPECT_EQ("a-b-c", abel::string_join(v, "-", abel::alpha_num_formatter()));
        // Iterators, no formatter
        EXPECT_EQ("a-b-c", abel::string_join(v.begin(), v.end(), "-"));
        // Range, no formatter
        EXPECT_EQ("a-b-c", abel::string_join(v, "-"));
    }

    TEST(string_join, Array) {
        const std::string_view a[] = {"a", "b", "c"};
        EXPECT_EQ("a-b-c", abel::string_join(a, "-"));
    }

    TEST(string_join, InitializerList) {
        { EXPECT_EQ("a-b-c", abel::string_join({"a", "b", "c"}, "-")); }

        {
            auto a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", abel::string_join(a, "-"));
        }

        {
            std::initializer_list<const char *> a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", abel::string_join(a, "-"));
        }

        {
            std::initializer_list<std::string> a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", abel::string_join(a, "-"));
        }

        {
            std::initializer_list<std::string_view> a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", abel::string_join(a, "-"));
        }

        {
            // Tests initializer_list with a non-default formatter
            auto a = {"a", "b", "c"};
            TestingParenFormatter f;
            EXPECT_EQ("(a)-(b)-(c)", abel::string_join(a, "-", f));
        }

        {
            // initializer_list of ints
            EXPECT_EQ("1-2-3", abel::string_join({1, 2, 3}, "-"));
        }

        {
            // Tests initializer_list of ints with a non-default formatter
            auto a = {1, 2, 3};
            TestingParenFormatter f;
            EXPECT_EQ("(1)-(2)-(3)", abel::string_join(a, "-", f));
        }
    }

    TEST(string_join, Tuple) {
        EXPECT_EQ("", abel::string_join(std::make_tuple(), "-"));
        EXPECT_EQ("hello", abel::string_join(std::make_tuple("hello"), "-"));

        int x(10);
        std::string y("hello");
        double z(3.14);
        EXPECT_EQ("10-hello-3.14", abel::string_join(std::make_tuple(x, y, z), "-"));

        // Faster! Faster!!
        EXPECT_EQ("10-hello-3.14",
                  abel::string_join(std::make_tuple(x, std::cref(y), z), "-"));

        struct TestFormatter {
            char buffer[128];

            void operator()(std::string *out, int v) {
                snprintf(buffer, sizeof(buffer), "%#.8x", v);
                out->append(buffer);
            }

            void operator()(std::string *out, double v) {
                snprintf(buffer, sizeof(buffer), "%#.0f", v);
                out->append(buffer);
            }

            void operator()(std::string *out, const std::string &v) {
                snprintf(buffer, sizeof(buffer), "%.4s", v.c_str());
                out->append(buffer);
            }
        };
        EXPECT_EQ("0x0000000a-hell-3.",
                  abel::string_join(std::make_tuple(x, y, z), "-", TestFormatter()));
        EXPECT_EQ(
                "0x0000000a-hell-3.",
                abel::string_join(std::make_tuple(x, std::cref(y), z), "-", TestFormatter()));
        EXPECT_EQ("0x0000000a-hell-3.",
                  abel::string_join(std::make_tuple(&x, &y, &z), "-",
                                    abel::dereference_formatter(TestFormatter())));
        EXPECT_EQ("0x0000000a-hell-3.",
                  abel::string_join(std::make_tuple(abel::make_unique<int>(x),
                                                    abel::make_unique<std::string>(y),
                                                    abel::make_unique<double>(z)),
                                    "-", abel::dereference_formatter(TestFormatter())));
        EXPECT_EQ("0x0000000a-hell-3.",
                  abel::string_join(std::make_tuple(abel::make_unique<int>(x), &y, &z),
                                    "-", abel::dereference_formatter(TestFormatter())));
    }

}  // namespace
