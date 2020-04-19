
file(GLOB SRC "*.cc")

carbin_check_target(abel::testing_static)
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
            benchmark::benchmark_main
            benchmark::benchmark
            testing_static
    )
endforeach (fl ${SRC})