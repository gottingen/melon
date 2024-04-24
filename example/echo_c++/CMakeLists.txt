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
project(echo_c++ C CXX)


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
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)

add_executable(echo_client client.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(echo_server server.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(echo_client melon::melon_shared ${MELON_DEPS_LIBS}  Threads::Threads)
target_include_directories(echo_client PRIVATE ${MELON_DEPS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(echo_server melon::melon_shared ${MELON_DEPS_LIBS}  Threads::Threads)
target_include_directories(echo_server PRIVATE ${MELON_DEPS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
