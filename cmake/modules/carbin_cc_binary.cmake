


# carbin_cc_binary()
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
# By default, carbin_cc_binary will always create a binary named ${PROJECT_NAME}_${NAME}.
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
# carbin_cc_binary(
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

function(carbin_cc_binary)

    cmake_parse_arguments(CARBIN_CC_BINARY
            ""
            "NAME"
            "SRCS;COPTS;DEFINES;LINKOPTS;DEPS"
            ${ARGN}
            )

    set(_NAME "${CARBIN_CC_BINARY_NAME}")

    add_executable(${_NAME} "")
    target_sources(${_NAME} PRIVATE ${CARBIN_CC_BINARY_SRCS})
    target_include_directories(${_NAME}
            PUBLIC ${CARBIN_COMMON_INCLUDE_DIRS}
            PRIVATE ${GMOCK_INCLUDE_DIRS} ${GTEST_INCLUDE_DIRS}
            )

    target_compile_definitions(${_NAME}
            PUBLIC
            ${CARBIN_CC_BINARY_DEFINES}
            )
    target_compile_options(${_NAME}
            PRIVATE ${CARBIN_CC_BINARY_COPTS}
            )

    target_link_libraries(${_NAME}
            PUBLIC ${CARBIN_CC_BINARY_DEPS}
            PRIVATE ${CARBIN_CC_BINARY_LINKOPTS}
            )

    set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${CARBIN_CXX_STANDARD})
    set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

    install(TARGETS ${_NAME} EXPORT ${PROJECT_NAME}Targets
            RUNTIME DESTINATION ${CARBIN_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CARBIN_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CARBIN_INSTALL_LIBDIR}
            )

endfunction()