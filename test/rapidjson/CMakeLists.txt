
file(GLOB RAPIDJSON_SRC "*.cc")

carbin_cc_test(
        NAME
        rapidjson_test
        SRCS
        ${RAPIDJSON_SRC}
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS ${TEST_DEPS}
)

