
FILE(GLOB TESTING_SRC "*.cc")

carbin_cc_library(
        NAMESPACE abel
        NAME testing
        SOURCES ${TESTING_SRC}
        PUBLIC_LINKED_TARGETS ${ABEL_DYLINK} abel::abel
        PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
)