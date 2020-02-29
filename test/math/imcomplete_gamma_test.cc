//
// Created by liyinbin on 2020/2/29.
//



#include "math_test.h"
#include <abel/math/incomplete_gamma.h>

TEST(incomplete_gamma, all) {

    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.26424111765711527644L,    2.0L,    1.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.42759329552912134220L,    1.5L,    1.0L);

    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.80085172652854419439L,    2.0L,    3.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.95957231800548714595L,    2.0L,    5.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.99876590195913317327L,    2.0L,    9.0L);

    MATH_TEST_EXPECT(abel::incomplete_gamma,                    1.0L,    0.0L,    9.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma,                    0.0L,    2.0L,    0.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma,                    0.0L,    0.0L,    0.0L);

    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.47974821959920432857L,   11.5L,   11.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.75410627202774938027L,   15.5L,   18.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.43775501395596266851L,   19.0L,   18.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.43939261060849132967L,   20.0L,   19.0L);

    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.58504236243630125536L,   38.0L,   39.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.46422093592347296598L,   56.0L,   55.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.55342727426212001696L,   98.0L,   99.0L);

    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.47356929040257916830L,  102.0L,  101.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.48457398488934400049L,  298.0L,  297.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.48467673854389853316L,  302.0L,  301.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.48807306199561317772L,  498.0L,  497.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.48812074555706047585L,  502.0L,  501.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.51881664512582725823L,  798.0L,  799.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.49059834200005758564L,  801.0L,  800.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.52943655952003709775L,  997.0L,  999.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, 0.45389544705967349580L, 1005.0L, 1001.0L);

    //

    MATH_TEST_EXPECT(abel::incomplete_gamma, TEST_NAN, -0.1, 0.0);

    MATH_TEST_EXPECT(abel::incomplete_gamma, TEST_NAN, TEST_NAN, TEST_NAN);
    MATH_TEST_EXPECT(abel::incomplete_gamma, TEST_NAN, TEST_NAN,     3.0L);
    MATH_TEST_EXPECT(abel::incomplete_gamma, TEST_NAN,     2.0L, TEST_NAN);


}