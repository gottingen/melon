
file(GLOB META_SRC "*.cc")

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})
    set(TEST_NAME atomic_${SRC_NAME})
    add_executable(${TEST_NAME}
            ${SRC_NAME}.cc
            )

        if(APPLE)
        target_link_libraries(${TEST_NAME}
                gtest
                gmock
                gtest_main
                testing_static
                abel_static
                pthread
                )
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(${TEST_NAME}
                gtest
                gmock
                gtest_main
                testing_static
                abel_static
                pthread
                rt
                )
    endif()

    add_test(
            NAME ${TEST_NAME}
            COMMAND ${TEST_NAME}
    )
endforeach (fl ${SRC})