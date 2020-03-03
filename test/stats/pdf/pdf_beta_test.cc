//
// Created by liyinbin on 2020/3/2.
//


#include <abel/stats/dens.h>
#include "test/stats/stats_test_util.h"

TEST(pdf_bern, all) {
    // parameters

    double a_par = 5.0;
    double b_par = 4.0;

    //

    std::vector<double> inp_vals = { 0.1,       0.5,     0.97 };
    std::vector<double> exp_vals = { 0.020412,  2.1875,  0.006692814 };

    //
    // scalar tests

    STATS_TEST_EXPECTED_VAL(pdf_beta,inp_vals[0],exp_vals[0],false,a_par,b_par);
    STATS_TEST_EXPECTED_VAL(pdf_beta,inp_vals[1],exp_vals[1],false,a_par,b_par);
    STATS_TEST_EXPECTED_VAL(pdf_beta,inp_vals[2],exp_vals[2],false,a_par,b_par);
    STATS_TEST_EXPECTED_VAL(pdf_beta,inp_vals[1],exp_vals[1],true,a_par,b_par);

    STATS_TEST_EXPECTED_VAL(pdf_beta,TEST_NAN,TEST_NAN,false,2,3);                                     // NaN inputs
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_NAN,false,TEST_NAN,3);
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_NAN,false,1.0,TEST_NAN);
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_NAN,false,TEST_NAN,TEST_NAN);
    STATS_TEST_EXPECTED_VAL(pdf_beta,TEST_NAN,TEST_NAN,false,TEST_NAN,TEST_NAN);

    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_NAN,false,-1.0,1.0);                                     // bad parameter value cases (a or b < 0)
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_NAN,false,1.0,-1.0);
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_NAN,false,-1.0,-1.0);

    STATS_TEST_EXPECTED_VAL(pdf_beta,-0.1,0.0,false,a_par,b_par);                                      // x < 0 or x > 1 cases
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.1,0.0,false,a_par,b_par);

    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,TEST_POSINF,false,0.0,0.0);                                   // a and b == 0 cases
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,TEST_POSINF,false,0.0,0.0);
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,0.0,false,0.0,0.0);

    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,TEST_POSINF,false,0.0,1.0);                                   // a == 0 or b == +Inf
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,0.0,false,0.0,1.0);
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,TEST_POSINF,false,0.0,TEST_POSINF);
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,0.0,false,0.0,TEST_POSINF);

    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,0.0,false,1.0,0.0);                                           // a == +Inf or b == 0
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,TEST_POSINF,false,1.0,0.0);
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,0.0,false,TEST_POSINF,1.0);
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,TEST_POSINF,false,TEST_POSINF,1.0);

    STATS_TEST_EXPECTED_VAL(pdf_beta,0.5,TEST_POSINF,false,TEST_POSINF,TEST_POSINF);                   // a == +Inf and b == +Inf
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.1,0.0,false,TEST_POSINF,TEST_POSINF);

    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,TEST_POSINF,false,0.5,1.0);                                   // x == 0 and a < 1.0
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,b_par,false,1.0,b_par);                                       // x == 0 and a == 1.0
    STATS_TEST_EXPECTED_VAL(pdf_beta,0.0,0.0,false,1.1,b_par);                                         // x == 0 and a > 1.0

    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,TEST_POSINF,false,1.0,0.5);                                   // x == 1 and b < 1.0
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,a_par,false,a_par,1.0);                                       // x == 1 and b == 1.0
    STATS_TEST_EXPECTED_VAL(pdf_beta,1.0,0.0,false,1.0,1.1);                                           // x == 1 and b > 1.0


}