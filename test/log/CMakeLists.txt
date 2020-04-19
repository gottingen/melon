
file(GLOB META_SRC "*.cc")

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})
    set(TEST_NAME log_${SRC_NAME})
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