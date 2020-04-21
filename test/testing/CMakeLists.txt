
FILE(GLOB TESTING_SRC "*.cc")


carbin_cc_library(
        NAME
        testing_static
        SRCS
        ${TESTING_SRC}
        DEPS
        "${GMOCK_LIBRARY}"
        "${GTEST_LIBRARY}"
        abel::abel
        pthread
)