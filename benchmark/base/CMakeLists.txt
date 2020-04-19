
file(GLOB SRC "*.cc")

foreach (fl ${SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" TEST_NAME ${fl})
    set(EXE_NAME time_${TEST_NAME})
    carbin_cc_benchmark(
            NAME
            ${EXE_NAME}
            SRCS
            ${TEST_NAME}.cc
            COPTS
            ${CARBIN_TEST_COPTS}
            DEPS
            abel::abel
            benchmark::benchmark
            benchmark::benchmark_main
            testing_static
    )
endforeach (fl ${SRC})