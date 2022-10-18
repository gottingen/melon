# Install script for directory: /Users/liyinbin/github/gottingen/melon/test

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

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/metrics/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/fiber/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/rpc/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/base/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/bootstrap/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/container/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/files/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/future/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/hash/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/log/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/memory/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/strings/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/io/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/debugging/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/times/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/thread/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/dag/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/rapidjson/cmake_install.cmake")

endif()

