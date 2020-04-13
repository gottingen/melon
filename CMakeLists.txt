cmake_minimum_required(VERSION 2.8)

# modify this
project(abel CXX)

list(APPEND CMAKE_MODULE_PATH
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_SOURCE_DIR}/cmake/modules)

include(carbin_debug)

if (${PROJECT_NAME} MATCHES "carbin")
    carbin_error("project name must be set")
endif ()


if (NOT DEFINED ENV{CARBIN_ROOT})
    set(ENV{CARBIN_ROOT} ${PROJECT_SOURCE_DIR}/carbin)
    carbin_print("env CARBIN_ROOT not set, set to $ENV{CARBIN_ROOT}")
endif ()

if (NOT DEFINED ENV{CARBIN_LIB_PATH})
    set(ENV{CARBIN_LIB_PATH} ${PROJECT_SOURCE_DIR}/carbin/lib)
    carbin_print("env CARBIN_LIB_PATH not set, set to $ENV{CARBIN_LIB_PATH}")
endif ()

if (NOT DEFINED ENV{CARBIN_INCLUDE_PATH})
    set(ENV{CARBIN_INCLUDE_PATH} ${PROJECT_SOURCE_DIR}/carbin/include)
    carbin_print("env CARBIN_INCLUDE_PATH not set, set to $ENV{CARBIN_INCLUDE_PATH}")
endif ()

include(carbin_module)

include_directories(${PROJECT_SOURCE_DIR})
link_directories($ENV{CARBIN_LIB_PATH})
include_directories($ENV{CARBIN_INCLUDE_PATH})


add_subdirectory(abel)

if (ENABLE_EXAMPLE)
    add_subdirectory(example)
endif (ENABLE_EXAMPLE)


if (ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif (ENABLE_BENCHMARK)


if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif (ENABLE_TESTING)

if (ABEL_PACKAGE_GEN)
    include(abel_package)
endif ()

