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
project(http_c++ C CXX)

# for development
list(APPEND CMAKE_PREFIX_PATH "../../output")
# EA may always install in /opt/EA/inf
list(APPEND CMAKE_PREFIX_PATH "/opt/EA/inf")

find_package(melon REQUIRED)
# MELON_DEPS_LIBS is a list of all the libraries that melon depends on
# MELON_DEPS_INCLUDE_DIRS is a list of all the include directories that melon depends on
# MELON_DEPS_LIBS and MELON_DEPS_INCLUDE_DIRS are set by the find_package(melon) command

include(FindThreads)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER http.proto)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(http_client http_client.cc)
add_executable(http_server http_server.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(benchmark_http benchmark_http.cc)

target_link_libraries(http_server ${MELON_STATIC_LIBRARIES})
target_include_directories(http_server PRIVATE ${MELON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(http_client  ${MELON_SHARED_LIBRARIES})
target_include_directories(http_client PRIVATE ${MELON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(benchmark_http melon::melon_shared ${MELON_DEPS_LIBS} Threads::Threads)
target_include_directories(benchmark_http PRIVATE ${MELON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${PROJECT_SOURCE_DIR}/key.pem
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/cert.pem
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
