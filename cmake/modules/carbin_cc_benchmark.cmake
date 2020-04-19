
# carbin_cc_benchmark()
#
# CMake function to imitate Bazel's cc_test rule.
#
# Parameters:
# NAME: name of target (see Usage below)
# SRCS: List of source files for the binary
# DEPS: List of other libraries to be linked in to the binary targets
# COPTS: List of private compile options
# DEFINES: List of public defines
# LINKOPTS: List of link options
#
# Note:
# By default, carbin_cc_benchmark will always create a binary named ${PROJECT_NAME}_${NAME}.
# This will also add it to ctest list as ${PROJECT_NAME}_${NAME}.
#
# Usage:
# carbin_cc_library(
#   NAME
#     awesome
#   HDRS
#     "a.h"
#   SRCS
#     "a.cc"
#   PUBLIC
# )
#
# carbin_cc_benchmark(
#   NAME
#     awesome_test
#   SRCS
#     "awesome_test.cc"
#   DEPS
#     ${PROJECT_NAME}::awesome
#     gmock
#     gtest_main
# )

include(CMakeParseArguments)
include(carbin_config_cxx_opts)
include(carbin_install_dirs)

function(carbin_cc_benchmark)
    if (NOT CARBIN_RUN_TESTS)
        return()
    endif ()

    cmake_parse_arguments(CARBIN_CC_TEST
            ""
            "NAME"
            "SRCS;COPTS;DEFINES;LINKOPTS;DEPS"
            ${ARGN}
            )

    set(_NAME "${PROJECT_NAME}_${CARBIN_CC_TEST_NAME}")

    add_executable(${_NAME} "")
    target_sources(${_NAME} PRIVATE ${CARBIN_CC_TEST_SRCS})
    target_include_directories(${_NAME}
            PUBLIC ${CARBIN_COMMON_INCLUDE_DIRS}
            PRIVATE ${GMOCK_INCLUDE_DIRS} ${GTEST_INCLUDE_DIRS}
            )

    target_compile_definitions(${_NAME}
            PUBLIC
            ${CARBIN_CC_TEST_DEFINES}
            )
    target_compile_options(${_NAME}
            PRIVATE ${CARBIN_CC_TEST_COPTS}
            )

    target_link_libraries(${_NAME}
            PUBLIC ${CARBIN_CC_TEST_DEPS}
            PRIVATE ${CARBIN_CC_TEST_LINKOPTS}
            )
    # Add all Abseil targets to a folder in the IDE for organization.
    set_property(TARGET ${_NAME} PROPERTY FOLDER ${CARBIN_IDE_FOLDER}/test)

    set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${CARBIN_CXX_STANDARD})
    set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

endfunction()
