
file(GLOB META_SRC "*.cc")

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})
    set(TEST_NAME asl_${SRC_NAME})

    if (APPLE)
        carbin_cc_test(
                NAME
                ${TEST_NAME}
                SRCS
                ${SRC_NAME}.cc
                COPTS
                ${CARBIN_TEST_COPTS}
                DEPS
                abel::abel
                gtest
                gtest_main
                gmock
                pthread
                testing_static
        )
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        carbin_cc_test(
                NAME
                ${TEST_NAME}
                SRCS
                ${SRC_NAME}.cc
                COPTS
                ${CARBIN_TEST_COPTS}
                DEPS
                abel::abel
                gtest
                gtest_main
                gmock
                pthread
                testing_static
                rt
        )
    endif ()
endforeach (fl ${SRC})