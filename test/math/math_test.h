// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_MATH_TEST_H
#define ABEL_MATH_TEST_H

#include <cmath>
#include <ios>
#include <iostream>
#include <iomanip>
#include <string>
#include <functional>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef TEST_VAL_TYPES
#define TEST_VAL_TYPES long double
#define TEST_VAL_TYPES_V 1
#endif


#ifndef TEST_ERR_TOL
#ifdef _WIN32
#define TEST_ERR_TOL 1e-10
#else
#define TEST_ERR_TOL 1e-14
#endif
#endif

#ifdef TEST_VAL_TYPES_V
    #define VAL_IS_INF(val) std::isinf(static_cast<long double>(val))
    #define VAL_IS_NAN(val) std::isnan(static_cast<long double>(val))
#else
#define VAL_IS_INF(val) false
#define VAL_IS_NAN(val) false
#endif

#ifndef TEST_NAN
#define TEST_NAN std::numeric_limits<double>::quiet_NaN()
#endif

#ifndef TEST_POSINF
#define TEST_POSINF std::numeric_limits<double>::infinity()
#endif

#ifndef TEST_NEGINF
#define TEST_NEGINF -std::numeric_limits<double>::infinity()
#endif


#define MATH_TEST_EQ(abel_fn, std_fn, ...)                                           \
    {                                                                                \
    auto abel_fn_val = abel_fn(__VA_ARGS__);                                         \
    auto std_fn_val = std_fn(__VA_ARGS__);                                           \
    auto err_val = std::abs(abel_fn_val - std_fn_val) / (1 + std::abs(std_fn_val)); \
    bool test_success = false;                                                       \
    if(std::isnan(static_cast<long double>(abel_fn_val)) &&                          \
        std::isnan(static_cast<long double>(std_fn_val))) {                          \
        test_success = true;                                                         \
    } else if  (!std::isnan(static_cast<long double>(abel_fn_val)) &&               \
                std::isinf(static_cast<long double>(std_fn_val)) &&                \
                abel_fn_val == std_fn_val){                                        \
                    test_success = true;                                           \
    } else if (err_val < 1e-14) {                                                   \
        test_success = true;                                                        \
    }                                                                               \
    EXPECT_TRUE(test_success)<<err_val;}


#define MATH_TEST_EXPECT(abel_fn, expected_val, ...)                              \
    {                                                                              \
    auto f_val = abel_fn(__VA_ARGS__);                                            \
    auto err_val = std::abs(f_val - expected_val) / (1 + std::abs(expected_val)); \
    bool test_success = false;                                                    \
    if (VAL_IS_NAN(expected_val) && VAL_IS_NAN(f_val)) {                                            \
        test_success = true;                                                                        \
    } else if(!VAL_IS_NAN(f_val) && VAL_IS_INF(f_val) && f_val == expected_val) {                   \
        test_success = true;                                                                        \
    } else if(err_val < TEST_ERR_TOL) {                                                             \
        test_success = true;                                                                        \
    }                                                                                                \
    EXPECT_TRUE(test_success);                                                                      \
    }

#endif  // ABEL_MATH_TEST_H
