
file(GLOB RAPIDJSON_SRC "*.cc")

carbin_cc_test(
        NAME rapidjson_test
        SOURCES ${RAPIDJSON_SRC}
        PUBLIC_LINKED_TARGETS
        ${TEST_LINKS}
        PRIVATE_COMPILE_OPTIONS ${CARBIN_TEST_COPTS}
        VERBOSE
)
