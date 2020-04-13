cmake_minimum_required(VERSION 2.8)

# modify this
project(abel CXX)

list(APPEND CMAKE_MODULE_PATH
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_SOURCE_DIR}/cmake/modules)

include(carbin_module)
#include(compiler)

if (${PROJECT_NAME} MATCHES "carbin")
    carbin_error("project name must be set")
endif ()

include_directories(${PROJECT_SOURCE_DIR})
if ($ENV{CARBIN_LIB_PATH} MATCHES "")
    set(ENV{CARBIN_LIB_PATH}/${PROJECT_SOURCE_DIR}/carbin/lib)
endif ()
link_directories($ENV{CARBIN_LIB_PATH})

if ($ENV{CARBIN_INCLUDE_PATH} MATCHES "")
    set(ENV{CARBIN_INCLUDE_PATH} ${PROJECT_SOURCE_DIR}/carbin/include)
endif ()
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

