cmake_minimum_required(VERSION 2.8)

project(abel CXX)

list(APPEND CMAKE_MODULE_PATH
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_SOURCE_DIR}/cmake/modules)
include(project_profile)
include(abel_module)
#include(compiler)

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(abel)

if (ENABLE_EXAMPLE)
    add_subdirectory(example)
endif (ENABLE_EXAMPLE)


if (ENABLE_BENCHMARK)
    include_directories(${PROJECT_SOURCE_DIR}/deps/benchmark-1.5.0/include)
    add_subdirectory(deps/benchmark-1.5.0)
    add_subdirectory(benchmark)
endif (ENABLE_BENCHMARK)


if (ENABLE_TESTING)
    enable_testing()
    include_directories(${PROJECT_SOURCE_DIR}/deps/googletest-1.10.x/googletest/include)
    include_directories(${PROJECT_SOURCE_DIR}/deps/googletest-1.10.x/googlemock/include)
    add_subdirectory(deps/googletest-1.10.x)
    add_subdirectory(test)
endif (ENABLE_TESTING)

if (ABEL_PACKAGE_GEN)
    include(abel_package)
endif ()
