

# carbin_cc_library()
#
# CMake function to imitate Bazel's cc_library rule.
#
# Parameters:
# NAME: name of target (see Note)
# HDRS: List of public header files for the library
# SRCS: List of source files for the library
# DEPS: List of other libraries to be linked in to the binary targets
# COPTS: List of private compile options
# DEFINES: List of public defines
# LINKOPTS: List of link options
# PUBLIC: Add this so that this library will be exported under ${PROJECT_NAME}::
# Also in IDE, target will appear in Abseil folder while non PUBLIC will be in Abseil/internal.
# TESTONLY: When added, this target will only be built if user passes -DCARBIN_RUN_TESTS=ON to CMake.
#
# Note:
# By default, carbin_cc_library will always create a library named ${PROJECT_NAME}_${NAME},
# and alias target ${PROJECT_NAME}::${NAME}.  The ${PROJECT_NAME}:: form should always be used.
# This is to reduce namespace pollution.
#
# carbin_cc_library(
#   NAME
#     awesome
#   HDRS
#     "a.h"
#   SRCS
#     "a.cc"
# )
# carbin_cc_library(
#   NAME
#     fantastic_lib
#   SRCS
#     "b.cc"
#   DEPS
#     ${PROJECT_NAME}::awesome # not "awesome" !
#   PUBLIC
# )
#
# carbin_cc_library(
#   NAME
#     main_lib
#   ...
#   DEPS
#     ${PROJECT_NAME}::fantastic_lib
# )
#
# TODO: Implement "ALWAYSLINK"

include(CMakeParseArguments)
include(carbin_config_cxx_opts)
include(carbin_install_dirs)

function(carbin_cc_library)
    cmake_parse_arguments(CARBIN_CC_LIB
            "DISABLE_INSTALL;PUBLIC;TESTONLY"
            "NAME"
            "HDRS;SRCS;COPTS;DEFINES;LINKOPTS;DEPS"
            ${ARGN}
            )

    if (CARBIN_CC_LIB_TESTONLY AND NOT CARBIN_RUN_TESTS)
        return()
    endif ()

    set(_NAME "${CARBIN_CC_LIB_NAME}")

    # Check if this is a header-only library
    # Note that as of February 2019, many popular OS's (for example, Ubuntu
    # 16.04 LTS) only come with cmake 3.5 by default.  For this reason, we can't
    # use list(FILTER...)
    set(CARBIN_CC_SRCS "${CARBIN_CC_LIB_SRCS}")
    foreach (src_file IN LISTS CARBIN_CC_SRCS)
        if (${src_file} MATCHES ".*\\.(h|inc)")
            list(REMOVE_ITEM CARBIN_CC_SRCS "${src_file}")
        endif ()
    endforeach ()

    if ("${CARBIN_CC_SRCS}" STREQUAL "")
        set(CARBIN_CC_LIB_IS_INTERFACE 1)
    else ()
        set(CARBIN_CC_LIB_IS_INTERFACE 0)
    endif ()

    # Determine this build target's relationship to the DLL. It's one of four things:
    # 1. "shared"  -- This is a shared library, perhaps on a non-windows platform
    #                 where DLL doesn't make sense.
    # 2. "static"  -- This target does not depend on the DLL and should be built
    #                 statically.
    if (BUILD_SHARED_LIBS)
        set(_build_type "shared")
    else ()
        set(_build_type "static")
    endif ()

    if (NOT CARBIN_CC_LIB_IS_INTERFACE)
        if (${_build_type} STREQUAL "static" OR ${_build_type} STREQUAL "shared")
            add_library(${_NAME} "")
            target_sources(${_NAME} PRIVATE ${CARBIN_CC_LIB_SRCS} ${CARBIN_CC_LIB_HDRS})
            target_link_libraries(${_NAME}
                    PUBLIC ${CARBIN_CC_LIB_DEPS}
                    PRIVATE
                    ${CARBIN_CC_LIB_LINKOPTS}
                    ${CARBIN_DEFAULT_LINKOPTS}
                    )
        else ()
            message(FATAL_ERROR "Invalid build type: ${_build_type}")
        endif ()

        # Linker language can be inferred from sources, but in the case of DLLs we
        # don't have any .cc files so it would be ambiguous. We could set it
        # explicitly only in the case of DLLs but, because "CXX" is always the
        # correct linker language for static or for shared libraries, we set it
        # unconditionally.
        set_property(TARGET ${_NAME} PROPERTY LINKER_LANGUAGE "CXX")

        target_include_directories(${_NAME}
                PUBLIC
                "$<BUILD_INTERFACE:${CARBIN_COMMON_INCLUDE_DIRS}>"
                $<INSTALL_INTERFACE:${CARBIN_INSTALL_INCLUDEDIR}>
                )
        target_compile_options(${_NAME}
                PRIVATE ${CARBIN_CC_LIB_COPTS})
        target_compile_definitions(${_NAME} PUBLIC ${CARBIN_CC_LIB_DEFINES})

        # Add all Abseil targets to a a folder in the IDE for organization.
        if (CARBIN_CC_LIB_PUBLIC)
            set_property(TARGET ${_NAME} PROPERTY FOLDER ${CARBIN_IDE_FOLDER})
        elseif (CARBIN_CC_LIB_TESTONLY)
            set_property(TARGET ${_NAME} PROPERTY FOLDER ${CARBIN_IDE_FOLDER}/test)
        else ()
            set_property(TARGET ${_NAME} PROPERTY FOLDER ${CARBIN_IDE_FOLDER}/internal)
        endif ()

        # INTERFACE libraries can't have the CXX_STANDARD property set
        set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${CARBIN_CXX_STANDARD})
        set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

        # When being installed, we lose the ${PROJECT_NAME}_ prefix.  We want to put it back
        # to have properly named lib files.  This is a no-op when we are not being
        # installed.

        set_target_properties(${_NAME} PROPERTIES
                #OUTPUT_NAME "${PROJECT_NAME}_${_NAME}"
                OUTPUT_NAME "${_NAME}"
                )

    else ()
        # Generating header-only library
        add_library(${_NAME} INTERFACE)
        target_include_directories(${_NAME}
                INTERFACE
                "$<BUILD_INTERFACE:${CARBIN_COMMON_INCLUDE_DIRS}>"
                $<INSTALL_INTERFACE:${CARBIN_INSTALL_INCLUDEDIR}>
                )

        target_link_libraries(${_NAME}
                INTERFACE
                ${CARBIN_CC_LIB_DEPS}
                ${CARBIN_CC_LIB_LINKOPTS}
                ${CARBIN_DEFAULT_LINKOPTS}
                )
        target_compile_definitions(${_NAME} INTERFACE ${CARBIN_CC_LIB_DEFINES})
    endif ()

    if (NOT CARBIN_CC_LIB_TESTONLY)
        install(TARGETS ${_NAME} EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CARBIN_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CARBIN_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CARBIN_INSTALL_LIBDIR}
                )
    endif ()

    add_library(${PROJECT_NAME}::${CARBIN_CC_LIB_NAME} ALIAS ${_NAME})
endfunction()
