# Install script for directory: /Users/liyinbin/github/gottingen/abel/test

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
  include("/Users/liyinbin/github/gottingen/abel/build/test/testing/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/algorithm/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/base/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/container/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/debugging/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/flags/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/functional/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/hash/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/memory/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/meta/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/numeric/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/random/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/strings/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/synchronization/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/time/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/types/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/abel/build/test/utility/cmake_install.cmake")

endif()

