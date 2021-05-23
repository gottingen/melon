cmake_minimum_required(VERSION 3.15)

# TODO modify this for you project
project(abel CXX C ASM)

# Compiler id for Apple Clang is now AppleClang.
cmake_policy(SET CMP0025 NEW)

# if command can use IN_LIST
cmake_policy(SET CMP0057 NEW)

# Project version variables are the empty string if version is unspecified
cmake_policy(SET CMP0048 NEW)

set(CMAKE_MACOSX_RPATH 1)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/copts)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/recipes)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/recipes)

include(carbin_print)

# TODO modyify this for "-std=c++[x]"
set(CMAKE_CXX_STANDARD 17)



set(PROJECT_DESCRIPTION "carbin c++ lib")
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 5)
set(PROJECT_VERSION_PATCH 3)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

option(CARBIN_STATUS_PRINT "cmake toolchain print" ON)
option(CARBIN_STATUS_DEBUG "cmake toolchain debug info" ON)

option(ENABLE_TESTING "enable unit test" OFF)
option(CARBIN_PACKAGE_GEN "enable package gen" OFF)
option(ENABLE_BENCHMARK "enable benchmark" OFF)
option(ENABLE_EXAMPLE "enable benchmark" ON)
option(ENABLE_SUMMARY "enable summary output" ON)
# it is too many
option(ENABLE_SUMMARY_CXX_FLAG "enable cxx flags" OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(PACKAGE_INSTALL_PREFIX "/usr/local")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include(carbin_module)
include(CMakePackageConfigHelpers)

if (NOT DEV_MODE AND ${PROJECT_NAME} MATCHES "carbin")
    carbin_error("project name must be set to your project name \
                  or define -DDEV_MODE=true set to develop modeeg: \n\
                   cmake .. -DDEV_MODE=true")
endif ()

#########################
# default pavkage


set(ABEL_DYLINK)
if (APPLE)
    find_library(CoreFoundation CoreFoundation)
    list(APPEND ABEL_DYLINK ${CoreFoundation} pthread)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND ABEL_DYLINK rt dl pthread)
endif ()


#TODO do add you own subdirs
add_subdirectory(abel)
# TODO end

if (ENABLE_BENCHMARK OR ENABLE_TESTING)
    add_subdirectory(testing)
endif()
if (ENABLE_EXAMPLE)
    add_subdirectory(example)
endif (ENABLE_EXAMPLE)


if (ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif (ENABLE_BENCHMARK)


if (ENABLE_TESTING)
    find_package(gtest REQUIRED)
    find_package(gmock REQUIRED)

    carbin_check_target(gtest::gtest)
    carbin_check_target(gtest::gtest_main)
    carbin_check_target(gmock::gmock)
    carbin_check_target(gmock::gmock_main)

    set(CARBIN_RUN_TESTS true)
    enable_testing()
    add_subdirectory(test)
endif (ENABLE_TESTING)

################################################################################
# Installation of the library and all it's sub components. No need to edit this.
################################################################################
# Get the Default installation folders:
# * CMAKE_INSTALL_LIBDIR
# * CMAKE_INSTALL_BINDIR
# * CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)

# Layout. This works for all platforms:
#   * <prefix>/lib*/cmake/<PROJECT-NAME>
#   * <prefix>/lib*/
#   * <prefix>/include/
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
        "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
        "cmake/package/carbin_config.cmake.in"
        "${project_config}"
        INSTALL_DESTINATION "${config_install_dir}"
)

#Targets:
#  * <prefix>/lib/libbar.a
#  * <prefix>/lib/libbaz.a
#  * header location after install: <prefix>/include/foo/Bar.hpp
#  * headers can be included by C++ code `#include <foo/Bar.hpp>`


# Config
#   * <prefix>/lib/cmake/Foo/FooConfig.cmake
#   * <prefix>/lib/cmake/Foo/FooConfigVersion.cmake
install(
        FILES
        "${project_config}" "${version_config}"
        DESTINATION
        "${config_install_dir}"
)

# Config
#   * <prefix>/lib/cmake/Foo/FooTargets.cmake
install(
        EXPORT
        "${TARGETS_EXPORT_NAME}"
        NAMESPACE
        "${namespace}"
        DESTINATION
        "${config_install_dir}"
)


if (CARBIN_PACKAGE_GEN)
    include(carbin_package)
endif ()
if (ENABLE_SUMMARY)
    include(carbin_summary)
endif ()
