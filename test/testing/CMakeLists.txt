
FILE(GLOB TESTING_SRC "*.cc")


carbin_cc_library(
        NAME
        testing_static
        SRCS
        ${TESTING_SRC}
        DEPS
        gtest
        gmock
        abel::abel
        pthread
)