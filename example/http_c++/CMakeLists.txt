# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

cmake_minimum_required(VERSION 2.8.10)
project(http_c++ C CXX)

list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})
option(LINK_SO "Whether examples are linked dynamically" OFF)

execute_process(
        COMMAND bash -c "find ${PROJECT_SOURCE_DIR}/../.. -type d -regex \".*output/include$\" | head -n1 | xargs dirname | tr -d '\n'"
        OUTPUT_VARIABLE OUTPUT_PATH
)

list(APPEND CMAKE_PREFIX_PATH ${OUTPUT_PATH})

find_package(protobuf REQUIRED)
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER http.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_path(GPERFTOOLS_INCLUDE_DIR NAMES gperftools/heap-profiler.h)
find_library(GPERFTOOLS_LIBRARIES NAMES tcmalloc_and_profiler)
include_directories(${GPERFTOOLS_INCLUDE_DIR})

find_path(MELON_INCLUDE_PATH NAMES melon/rpc/server.h)
if (LINK_SO)
    find_library(MELON_LIB NAMES melon)
else ()
    find_library(MELON_LIB NAMES libmelon.a melon)
endif ()
if ((NOT MELON_INCLUDE_PATH) OR (NOT MELON_LIB))
    message(FATAL_ERROR "Fail to find melon")
endif ()
include_directories(${MELON_INCLUDE_PATH})

find_package(gflags REQUIRED)
include_directories(${gflags_INCLUDE_DIR})

execute_process(
        COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${gflags_INCLUDE_DIR}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
)
if (${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
    execute_process(
            COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${gflags_INCLUDE_DIR}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
            OUTPUT_VARIABLE GFLAGS_NS
    )
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if (NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif ()
endif ()

set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__=__unused__ -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

if (CMAKE_VERSION VERSION_LESS "3.1.3")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif ()
else ()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif ()
include_directories(${LEVELDB_INCLUDE_PATH})

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
            "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
    )
endif ()

find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})
find_path(GLOG_INCLUDE_PATH NAMES glog/logging.h)
find_library(GLOG_LIB NAMES glog)
if ((NOT GLOG_INCLUDE_PATH) OR (NOT GLOG_LIB))
    message(FATAL_ERROR "Fail to find glog")
endif ()
include_directories(${GLOG_INCLUDE_PATH})

set(DYNAMIC_LIB
        ${CMAKE_THREAD_LIBS_INIT}
        ${gflags_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${LEVELDB_LIB}
        ${GLOG_LIB}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
        dl
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DYNAMIC_LIB ${DYNAMIC_LIB}
            pthread
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework CoreData"
            "-framework CoreText"
            "-framework Security"
            "-framework Foundation"
            "-Wl,-U,_MallocExtension_ReleaseFreeMemory"
            "-Wl,-U,_ProfilerStart"
            "-Wl,-U,_ProfilerStop"
            "-Wl,-U,__Z13GetStackTracePPvii")
endif ()

add_executable(http_client http_client.cpp)
add_executable(http_server http_server.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(benchmark_http ${PROJECT_SOURCE_DIR}/benchmark_http.cpp)

add_executable(benchmark_mc ${PROJECT_SOURCE_DIR}/benchmark_mc.cc)

target_link_libraries(http_server ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(http_client ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(benchmark_http ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(benchmark_mc ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})

file(COPY ${PROJECT_SOURCE_DIR}/key.pem
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/cert.pem
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
