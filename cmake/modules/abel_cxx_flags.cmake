
include(abel_debug)
include(abel_error)

include(abel_clang_flags)
include(abel_gnuc_flags)

abel_print("start set up compiler flags")
if (WIN32)
    abel_error("current not supoort win32")
elseif (APPLE)
    abel_print("compiler is AppleClang")
    set(ABEL_CXX_FLAGS ${ABEL_CLANG_CXX_FLAGS})
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    abel_print("compiler is gnuc")
    set(ABEL_CXX_FLAGS ${ABEL_GNUC_CXX_FLAGS})
else ()
    abel_error("do not known compiler and platform")
endif ()
