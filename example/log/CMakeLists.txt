
file(GLOB META_SRC "*.cc")

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})
    set(TEST_NAME example_log_${SRC_NAME})
    if(APPLE)
        carbin_cc_binary(
                NAME
                ${TEST_NAME}
                SRCS
                ${SRC_NAME}.cc
                COPTS
                ${CARBIN_TEST_COPTS}
                DEPS
                abel::abel
                pthread
        )
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        carbin_cc_binary(
                NAME
                ${TEST_NAME}
                SRCS
                ${SRC_NAME}.cc
                COPTS
                ${CARBIN_TEST_COPTS}
                DEPS
                abel::abel
                pthread
                rt
        )
    endif()

endforeach (fl ${SRC})