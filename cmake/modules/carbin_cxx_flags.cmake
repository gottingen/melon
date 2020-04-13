
include(carbin_debug)
include(carbin_error)

include(carbin_clang_flags)
include(carbin_gnuc_flags)

carbin_print("start set up compiler flags")
if (WIN32)
    carbin_error("current not supoort win32")
elseif (APPLE)
    carbin_print("compiler is AppleClang")
    set(CARBIN_CXX_FLAGS ${CARBIN_CLANG_CXX_FLAGS})
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    carbin_print("compiler is gnuc")
    set(CARBIN_CXX_FLAGS ${CARBIN_GNUC_CXX_FLAGS})
else ()
    carbin_error("do not known compiler and platform")
endif ()
