//
// Created by liyinbin on 2020/3/3.
//

#include <abel/stats/dens.h>
#include "test/stats/stats_test_util.h"

TEST(pdf_binom, all) {
    int n_trials = 4;
    double prob_par = 0.6;

    //

    std::vector<int> inp_vals    = { 3,       2,       1 };
    std::vector<double> exp_vals = { 0.3456,  0.3456,  0.1536 };

    //
    // scalar tests

    STATS_TEST_EXPECTED_VAL(pdf_binom,inp_vals[0],exp_vals[0],false,n_trials,prob_par);
    STATS_TEST_EXPECTED_VAL(pdf_binom,inp_vals[1],exp_vals[1],false,n_trials,prob_par);
    STATS_TEST_EXPECTED_VAL(pdf_binom,inp_vals[2],exp_vals[2],false,n_trials,prob_par);
    STATS_TEST_EXPECTED_VAL(pdf_binom,inp_vals[1],exp_vals[1],true,n_trials,prob_par);

    STATS_TEST_EXPECTED_VAL(pdf_binom,1,TEST_NAN,false,2,TEST_NAN);                                    // NaN inputs

    STATS_TEST_EXPECTED_VAL(pdf_binom,-1.0,0.0,false,n_trials,prob_par);                               // x < 0 or x > n_trials
    STATS_TEST_EXPECTED_VAL(pdf_binom,n_trials+1,0.0,false,n_trials,prob_par);

    STATS_TEST_EXPECTED_VAL(pdf_binom,0,TEST_NAN,false,-1,0.5);                                        // n_trials < 0
    STATS_TEST_EXPECTED_VAL(pdf_binom,0,TEST_NAN,false,1,-0.1);                                        // p < 0
    STATS_TEST_EXPECTED_VAL(pdf_binom,0,TEST_NAN,false,1,1.1);                                         // p > 1

    STATS_TEST_EXPECTED_VAL(pdf_binom,0,1,false,0,0.5);                                                // n_trials == 0
    STATS_TEST_EXPECTED_VAL(pdf_binom,1,0,false,0,0.5);

    STATS_TEST_EXPECTED_VAL(pdf_binom,1,prob_par,false,1,prob_par);                                    // n_trials == 1

}