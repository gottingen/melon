include(project_profile)
include(abel_error)
include(abel_debug)
include(abel_cxx_flags)
include(abel_outof_source)
include(abel_platform)

ABEL_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(${ABEL_STD} COMPILER_SUPPORTS_ABEL_STD)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)


if (COMPILER_SUPPORTS_CXX20)
    abel_print("compiler is check ok, supports c++20")
elseif (COMPILER_SUPPORTS_CXX17)
    abel_print("compiler is check ok, supports c++17")
elseif (COMPILER_SUPPORTS_CXX14)
    abel_print("compiler is check ok, supports c++14")
elseif (COMPILER_SUPPORTS_CXX11)
    abel_print("compiler is check ok, supports c++11")
else ()
    abel_error("compiler must support c++11")
endif ()

if (COMPILER_SUPPORTS_ABEL_STD)
    abel_print("compiler is check ok, you have enable ${ABEL_STD}")
else ()
    abel_error("compiler must support ${ABEL_STD}")
endif()


string(REPLACE ";" " " CMAKE_CXX_FLAGS "${ABEL_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

abel_print("end set up compiler flags")