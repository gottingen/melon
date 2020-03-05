
file(GLOB SRC "*.cc")

foreach (fl ${SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" TEST_NAME ${fl})
    add_executable(${TEST_NAME}
            ${TEST_NAME}.cc
            )

    target_link_libraries(${TEST_NAME}
            benchmark
            benchmark_main
            abel_static
            pthread
            )
    add_test(
            NAME ${TEST_NAME}
            COMMAND ${TEST_NAME}
    )
endforeach (fl ${SRC})

