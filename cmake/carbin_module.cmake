include(project_profile)
include(carbin_error)
include(carbin_debug)
include(carbin_cxx_flags)
include(carbin_outof_source)
include(carbin_platform)
include(carbin_pkg_dump)

CARBIN_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(${CARBIN_STD} COMPILER_SUPPORTS_CARBIN_STD)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)


if (COMPILER_SUPPORTS_CXX20)
    carbin_print("compiler is check ok, supports c++20")
elseif (COMPILER_SUPPORTS_CXX17)
    carbin_print("compiler is check ok, supports c++17")
elseif (COMPILER_SUPPORTS_CXX14)
    carbin_print("compiler is check ok, supports c++14")
elseif (COMPILER_SUPPORTS_CXX11)
    carbin_print("compiler is check ok, supports c++11")
else ()
    carbin_error("compiler must support c++11")
endif ()

if (COMPILER_SUPPORTS_CARBIN_STD)
    carbin_print("compiler is check ok, you have enable ${CARBIN_STD}")
else ()
    carbin_error("compiler must support ${CARBIN_STD}")
endif()


string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CARBIN_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

carbin_print("end set up compiler flags")