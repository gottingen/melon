# Install script for directory: /Users/liyinbin/github/gottingen/melon/example

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
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/asynchronous_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/auto_concurrency_limiter/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/backup_request/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/cancel/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/cascade_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/dynamic_partition_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/grpc/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/http/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/memcache/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/multi_threaded_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/multi_threaded_echo_fns/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/nshead_extension/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/nshead_pb_extension/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/parallel_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/partition_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/redis/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/selective_echo/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/session_data_and_thread_local/cmake_install.cmake")
  include("/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/example/streaming_echo/cmake_install.cmake")

endif()

