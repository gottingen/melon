
carbin_cc_binary(
        NAME
        bar_example
        SRCS
        "add_example.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        carbin-cmake::bar
)