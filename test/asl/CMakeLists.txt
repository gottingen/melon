
file(GLOB META_SRC "*.cc")

carbin_check_target(GTest::Main)
carbin_check_target(GTest::GTest)

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})
    set(TEST_NAME asl_${SRC_NAME})

    carbin_cc_test(
            NAME
            ${TEST_NAME}
            SRCS
            ${SRC_NAME}.cc
            COPTS
            ${CARBIN_TEST_COPTS}
            DEPS ${TEST_DEPS}
    )
endforeach (fl ${SRC})