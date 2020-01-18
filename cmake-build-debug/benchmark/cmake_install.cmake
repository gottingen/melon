# Install script for directory: /Users/liyinbin/github/gottingen/abel/benchmark

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/algorithm/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/base/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/container/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/functional/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/numeric/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/synchronization/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/types/cmake_install.cmake")

endif()

