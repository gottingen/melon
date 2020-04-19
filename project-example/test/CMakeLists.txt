
find_package(GTest REQUIRED)

carbin_cc_test(
        NAME
        bar_test
        SRCS
        "bar_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        carbin-cmake::bar
        GTest::Main
        GTest::GTest
)