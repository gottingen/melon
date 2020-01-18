cmake_minimum_required(VERSION 2.8)

project(abel CXX)

option(BUILD_TEST "build unit test case, default on" ON)
option(BENCH_MARK "build unit test case, default on" OFF)

set(CMAKE_INSTALL_PREFIX /usr/local)
include(cmake/compiler.cmake)
include_directories(${PROJECT_SOURCE_DIR})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(abel)

if (BENCH_MARK)
    include_directories(${PROJECT_SOURCE_DIR}/deps/benchmark-1.5.0/include)
    add_subdirectory(deps/benchmark-1.5.0)
    add_subdirectory(benchmark)
endif (BENCH_MARK)


if (BUILD_TEST)
    enable_testing()
    include_directories(${PROJECT_SOURCE_DIR}/deps/googletest-1.10.x/googletest/include)
    include_directories(${PROJECT_SOURCE_DIR}/deps/googletest-1.10.x/googlemock/include)
    add_subdirectory(deps/googletest-1.10.x)
    add_subdirectory(test)
endif (BUILD_TEST)


