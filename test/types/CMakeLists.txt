
file(GLOB META_SRC "*.cc")

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})
    set(TEST_NAME types_${SRC_NAME})
    add_executable(${TEST_NAME}
            ${SRC_NAME}.cc
            )

    target_link_libraries(${TEST_NAME}
            gtest
            gmock
            gtest_main
            testing_static
            abel_static
            pthread
            )

    add_test(
            NAME ${TEST_NAME}
            COMMAND ${TEST_NAME}
    )
endforeach (fl ${SRC})