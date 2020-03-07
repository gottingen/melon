
file(GLOB SRC "*.cc")

foreach (fl ${SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" TEST_NAME ${fl})
    set(EXE_NAME stats_${TEST_NAME})
    add_executable(${EXE_NAME}
            ${TEST_NAME}.cc
            )

    target_link_libraries(${EXE_NAME}
            benchmark
            benchmark_main
            testing_static
            abel_static
            pthread
            )
    add_test(
            NAME ${EXE_NAME}
            COMMAND ${EXE_NAME}
    )
endforeach (fl ${SRC})