include(carbin_error)
include(carbin_debug)
include(carbin_cc_library)
include(carbin_cc_test)
include(carbin_cc_binary)
include(carbin_check)

if (NOT CARBIN_PREFIX)
    set(CARBIN_PREFIX ${PROJECT_SOURCE_DIR}/carbin)
    carbin_print("CARBIN_PREFIX not set, set to ${CARBIN_PREFIX}")
endif ()

include_directories(${PROJECT_SOURCE_DIR})
carbin_print("add include dir ${PROJECT_SOURCE_DIR}")
include_directories(${PROJECT_BINARY_DIR})
carbin_print("add include dir ${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_PREFIX_PATH ${CARBIN_PREFIX})

include(project_profile)
include(carbin_outof_source)
include(carbin_platform)
include(carbin_pkg_dump)

CARBIN_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

if (NOT DEV_MODE AND ${PROJECT_VERSION} MATCHES "0.0.0")
    carbin_error("PROJECT_VERSION must be set in file project_profile or set -DDEV_MODE=true for develop debug")
endif()


set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
