//
// Created by liyinbin on 2020/3/1.
//

#include <abel/stats/dens.h>
#include "test/stats/stats_test_util.h"

TEST(pdf_bern, all) {
    double prob_par = 0.4;

//

    std::vector<double> inp_vals = {1, 0, 1};
    std::vector<double> exp_vals = {prob_par, 1 - prob_par, prob_par};

//
// scalar tests

    STATS_TEST_EXPECTED_VAL(pdf_bern, inp_vals[0], exp_vals[0], false, prob_par);
    STATS_TEST_EXPECTED_VAL(pdf_bern, inp_vals[1], exp_vals[1], false, prob_par);
    STATS_TEST_EXPECTED_VAL(pdf_bern, inp_vals[2], exp_vals[2], true, prob_par);

// STATS_TEST_EXPECTED_VAL(pdf_bern,TEST_NAN,TEST_NAN,false,0.5);                                     // NaN inputs
    STATS_TEST_EXPECTED_VAL(pdf_bern, 1, TEST_NAN, false, TEST_NAN);

    STATS_TEST_EXPECTED_VAL(pdf_bern, 1, TEST_NAN, false,
                            -0.1);                                           // bad parameter values
    STATS_TEST_EXPECTED_VAL(pdf_bern, 1, TEST_NAN, false, 1.1);

    STATS_TEST_EXPECTED_VAL(pdf_bern, -1, 0.0, false, prob_par);                                           // x < 0 or > 1
    STATS_TEST_EXPECTED_VAL(pdf_bern, 2, 0.0, false, prob_par);

}