#
# Copyright 2024 The Carbin Authors.
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

find_package(Gperftools)
include_directories(${GPERFTOOLS_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(GTest REQUIRED PATHS "${PROJECT_SOURCE_DIR}/carbin" "/opt/EA/inf" "/opt/EA/")
include_directories(${GTEST_INCLUDE_DIRS})
set(GTEST_LIB GTest::gtest)
set(GTEST_MAIN_LIB GTest::gtest_main)
set(GMOCK_LIB GTest::gmock)
set(GMOCK_MAIN_LIB GTest::gmock_main)

file(COPY ${PROJECT_SOURCE_DIR}/test/cert1.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert2.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert1.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert2.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/jsonout DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)

add_subdirectory(util)
add_subdirectory(var)
add_subdirectory(fiber)
add_subdirectory(rpc)
add_subdirectory(raft)
