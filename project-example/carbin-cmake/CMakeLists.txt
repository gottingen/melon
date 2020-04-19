

carbin_cc_library(
        NAME
        bar
        HDRS
        "bar.h"
        SRCS
        "bar.cc"
        COPTS
        ${CARBIN_DEFAULT_COPTS}
        PUBLIC
)