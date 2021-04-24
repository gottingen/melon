// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/base/throw_delegate.h"
#include <functional>
#include <new>
#include <stdexcept>
#include "abel/base/profile.h"
#include "gtest/gtest.h"

namespace {

    using abel::throw_std_logic_error;
    using abel::throw_std_invalid_argument;
    using abel::throw_std_domain_error;
    using abel::throw_std_length_error;
    using abel::throw_std_out_of_range;
    using abel::throw_std_runtime_error;
    using abel::throw_std_range_error;
    using abel::throw_std_overflow_error;
    using abel::throw_std_underflow_error;
    using abel::throw_std_bad_function_call;
    using abel::throw_std_bad_alloc;

    constexpr const char *what_arg = "The quick brown fox jumps over the lazy dog";

    template<typename E>
    void expect_throw_char(void (*f)(const char *)) {
#ifdef ABEL_HAVE_EXCEPTIONS
        try {
            f(what_arg);
            FAIL() << "Didn't throw";
        } catch (const E &e) {
            EXPECT_STREQ(e.what(), what_arg);
        }
#else
        EXPECT_DEATH_IF_SUPPORTED(f(what_arg), what_arg);
#endif
    }

    template<typename E>
    void expect_throw_string(void (*f)(const std::string &)) {
#ifdef ABEL_HAVE_EXCEPTIONS
        try {
            f(what_arg);
            FAIL() << "Didn't throw";
        } catch (const E &e) {
            EXPECT_STREQ(e.what(), what_arg);
        }
#else
        EXPECT_DEATH_IF_SUPPORTED(f(what_arg), what_arg);
#endif
    }

    template<typename E>
    void expect_throw_no_what(void (*f)()) {
#ifdef ABEL_HAVE_EXCEPTIONS
        try {
            f();
            FAIL() << "Didn't throw";
        } catch (const E &e) {
        }
#else
        EXPECT_DEATH_IF_SUPPORTED(f(), "");
#endif
    }

    TEST(ThrowHelper, Test) {
        // Not using EXPECT_THROW because we want to check the .what() message too.
        expect_throw_char<std::logic_error>(throw_std_logic_error);
        expect_throw_char<std::invalid_argument>(throw_std_invalid_argument);
        expect_throw_char<std::domain_error>(throw_std_domain_error);
        expect_throw_char<std::length_error>(throw_std_length_error);
        expect_throw_char<std::out_of_range>(throw_std_out_of_range);
        expect_throw_char<std::runtime_error>(throw_std_runtime_error);
        expect_throw_char<std::range_error>(throw_std_range_error);
        expect_throw_char<std::overflow_error>(throw_std_overflow_error);
        expect_throw_char<std::underflow_error>(throw_std_underflow_error);

        expect_throw_string<std::logic_error>(throw_std_logic_error);
        expect_throw_string<std::invalid_argument>(throw_std_invalid_argument);
        expect_throw_string<std::domain_error>(throw_std_domain_error);
        expect_throw_string<std::length_error>(throw_std_length_error);
        expect_throw_string<std::out_of_range>(throw_std_out_of_range);
        expect_throw_string<std::runtime_error>(throw_std_runtime_error);
        expect_throw_string<std::range_error>(throw_std_range_error);
        expect_throw_string<std::overflow_error>(throw_std_overflow_error);
        expect_throw_string<std::underflow_error>(throw_std_underflow_error);

        expect_throw_no_what<std::bad_function_call>(throw_std_bad_function_call);
        expect_throw_no_what<std::bad_alloc>(throw_std_bad_alloc);
    }

}  // namespace
