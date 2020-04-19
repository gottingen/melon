cmake_minimum_required(VERSION 3.5)


# Compiler id for Apple Clang is now AppleClang.
cmake_policy(SET CMP0025 NEW)

# if command can use IN_LIST
cmake_policy(SET CMP0057 NEW)

# Project version variables are the empty string if version is unspecified
cmake_policy(SET CMP0048 NEW)

# TODO modify this for you project
project(carbin-cmake CXX)
# TODO modyify this for "-std=c++[x]"
set(CMAKE_CXX_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_SOURCE_DIR}/cmake/modules)

include(carbin_module)
include(CMakePackageConfigHelpers)

if (NOT DEV_MODE AND ${PROJECT_NAME} MATCHES "carbin")
    carbin_error("project name must be set")
endif ()





add_subdirectory(${PROJECT_NAME})

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


list(APPEND SYSTEM_INSTALL_DIRS "/usr/local" "/usr" "/opt/" "/opt/local" "c:/Program Files/${PROJECT_NAME}")
if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX IN_LIST SYSTEM_INSTALL_DIRS)
    message(WARNING "\
  The default and system-level install directories are unsupported except in LTS \
  releases of Abseil.  Please set CMAKE_INSTALL_PREFIX to install Abseil in your \
  source or build tree directly.\
    ")
endif()

# install as a subdirectory only
install(EXPORT ${PROJECT_NAME}Targets
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${CARBIN_INSTALL_CONFIGDIR}"
        )

configure_package_config_file(
        cmake/package/carbin_config.cmake.in
        "${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION "${CARBIN_INSTALL_CONFIGDIR}"
)
install(FILES "${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}Config.cmake"
        DESTINATION "${CARBIN_INSTALL_CONFIGDIR}"
        )

# Abseil only has a version in LTS releases.  This mechanism is accomplished
# Abseil's internal Copybara (https://github.com/google/copybara) workflows and
# isn't visible in the CMake buildsystem itself.
#if(${PROJECT_NAME}_VERSION)
    write_basic_package_version_file(
            "${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}ConfigVersion.cmake"
            COMPATIBILITY ExactVersion
    )

    install(FILES "${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION ${CARBIN_INSTALL_CONFIGDIR}
            )
#endif()  # ${PROJECT_NAME}_VERSION

install(DIRECTORY ${PROJECT_NAME}
        DESTINATION ${CARBIN_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.inc"
        PATTERN "*.h"
        )

if (CARBIN_PACKAGE_GEN)
    include(carbin_package)
endif ()
