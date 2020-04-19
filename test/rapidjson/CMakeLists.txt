
file(GLOB RAPIDJSON_SRC "*.cc")

carbin_cc_test(
        NAME
        rapidjson_test
        SRCS
        ${RAPIDJSON_SRC}
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        abel::abel
        gtest
        testing_static
        gtest_main
        gmock
        pthread
)
