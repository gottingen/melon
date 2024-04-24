#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.15)
project(redis_c++ C CXX)

# Install dependencies:
# With apt:
# sudo apt-get install libreadline-dev
# sudo apt-get install ncurses-dev
# With yum:
# sudo yum install readline-devel
# sudo yum install ncurses-devel


# for development
list(APPEND CMAKE_PREFIX_PATH "../../output")
# EA may always install in /opt/EA/inf
list(APPEND CMAKE_PREFIX_PATH "/opt/EA/inf")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(melon REQUIRED)
# MELON_DEPS_LIBS is a list of all the libraries that melon depends on
# MELON_DEPS_INCLUDE_DIRS is a list of all the include directories that melon depends on
# MELON_DEPS_LIBS and MELON_DEPS_INCLUDE_DIRS are set by the find_package(melon) command

find_package(Threads REQUIRED)

find_path(GPERFTOOLS_INCLUDE_DIR NAMES gperftools/heap-profiler.h)
find_library(GPERFTOOLS_LIBRARIES NAMES tcmalloc_and_profiler)
include_directories(${GPERFTOOLS_INCLUDE_DIR})

add_executable(redis_cli redis_cli.cc)
add_executable(redis_press redis_press.cc)
add_executable(redis_server redis_server.cc)

set(AUX_LIB readline ncurses)

target_link_libraries(redis_cli melon::melon_shared ${MELON_DEPS_LIBS}  Threads::Threads ${AUX_LIB})
target_link_libraries(redis_press melon::melon_shared ${MELON_DEPS_LIBS}  Threads::Threads)
target_link_libraries(redis_server melon::melon_shared ${MELON_DEPS_LIBS}  Threads::Threads)
