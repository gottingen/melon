//
// Created by liyinbin on 2020/3/1.
//

#ifndef ABEL_STATS_TEST_UTIL_H
#define ABEL_STATS_TEST_UTIL_H

#include <abel/math/log.h>
#include <algorithm>
#include <cmath>
#include <ios>
#include <iostream>
#include <iomanip>
#include <string>
#include <gtest/gtest.h>
#include <limits>

#ifndef STATS_TEST_INPUT_TYPE
#define STATS_TEST_INPUT_TYPE 0 // switch between d/p (0) and q (1) cases (log_form input)
#endif


#if STATS_TEST_INPUT_TYPE == 0
#ifndef TEST_STRIP_FN_ARGS
#define TEST_STRIP_FN_ARGS(fn, x, lf, ...) fn(x,__VA_ARGS__,lf)
#endif
#ifndef TEST_PRINT_LOG_INPUT
#define TEST_PRINT_LOG_INPUT(lf) << "," << ((lf) ? "true" : "false")
#endif
#else
#ifndef TEST_STRIP_FN_ARGS
#define TEST_STRIP_FN_ARGS(fn,x,lf,...) fn(x,__VA_ARGS__)
#endif
#ifndef TEST_PRINT_LOG_INPUT
#define TEST_PRINT_LOG_INPUT(lf)
#endif
#endif



#ifndef TEST_ERR_TOL
#ifdef _WIN32
#define TEST_ERR_TOL 1e-06
#else
#define TEST_ERR_TOL 1e-06
#endif
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


#ifndef TEST_VAL_TYPES_V
#define VAL_IS_INF(val) std::isinf(static_cast<long double>(val))
    #define VAL_IS_NAN(val) std::isnan(static_cast<long double>(val))
#else
#define VAL_IS_INF(val) false
#define VAL_IS_NAN(val) false
#endif


#define STATS_TEST_EXPECTED_VAL(fn_eval, val_inp, expected_val, \
                                log_form, ...)                                                      \
{                                                                                                   \
                                                                                                    \
    auto check_val = abel::log_if(expected_val,log_form);                                                 \
                                                                                                    \
    auto f_val = TEST_STRIP_FN_ARGS(abel::fn_eval,val_inp,log_form,__VA_ARGS__);                   \
    auto err_val = std::abs(f_val - check_val) / (1 + std::abs(check_val));                         \
                                                                                                    \
    bool test_success = false;                                                                      \
                                                                                                    \
    if (VAL_IS_NAN(expected_val) && VAL_IS_NAN(f_val)) {                                            \
        test_success = true;                                                                        \
    } else if(!VAL_IS_NAN(f_val) && VAL_IS_INF(f_val) && f_val == check_val) {                      \
        test_success = true;                                                                        \
    } else if(err_val < TEST_ERR_TOL) {                                                             \
        test_success = true;                                                                        \
    }                                                                                               \
    EXPECT_TRUE(test_success)<<check_val <<" " <<expected_val << " "<<f_val;                                     \
}

#endif //ABEL_STATS_TEST_UTIL_H
