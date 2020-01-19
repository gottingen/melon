
include(abel_debug)
include(abel_error)

include(abel_clang_flags)
include(abel_gnuc_flags)

abel_print("start set up compiler flags")
if(WIN32)
    abel_error("current not supoort win32")
elseif(APPLE)
    abel_print("compiler is AppleClang")
    set(ABEL_CXX_FLAGS ${ABEL_CLANG_CXX_FLAGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    abel_print("compiler is gnuc")
    set(ABEL_CXX_FLAGS ${ABEL_GNUC_CXX_FLAGS})
else()
    abel_error("do not known compiler and platform")
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

if(COMPILER_SUPPORTS_CXX11)
    abel_print("compiler is check ok, supports c++11")
else()
    abel_error("compiler must support c++11")
endif()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${ABEL_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

abel_print("end set up compiler flags")