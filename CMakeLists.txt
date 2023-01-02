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

cmake_minimum_required(VERSION 3.1.3)

project(melon C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/package)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/recipes)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/copts)
include(carbin_module)

option(DEBUG "Print debug logs" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)
option(WITH_THRIFT "With thrift framed protocol supported" OFF)
option(BUILD_UNIT_TESTS "Whether to build unit tests" ON)
option(BUILD_BENCHMARK "Whether to build benchmarks" ON)
option(INSTALL_STATIC_LIBS "Whether to install static libraries" OFF)
option(BUILD_IN_CONDA "build in conda enviroment" ON)
option(BUILD_EXAMPLES "build in conda environemnt" ON)

if (BUILD_IN_CONDA)
    list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})
endif (BUILD_IN_CONDA)

# Enable MACOSX_RPATH. Run "cmake --help-policy CMP0042" for policy details.
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

set(MELON_VERSION 0.9.0)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "melon authors")
INCLUDE(CPack)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # require at least gcc 4.8
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC is too old, please install a newer version supporting C++11")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # require at least clang 3.3
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang is too old, please install a newer version supporting C++11")
    endif ()
else ()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif ()


if (WITH_DEBUG_SYMBOLS)
    set(DEBUG_SYMBOL "-g")
endif ()

if (WITH_THRIFT)
    set(THRIFT_CPP_FLAG "-DENABLE_THRIFT_FRAMED_PROTOCOL")
    set(THRIFT_LIB "thrift")
endif ()

include(GNUInstallDirs)

include(require_gflags)
include(require_leveldb)

execute_process(
        COMMAND bash -c "${PROJECT_SOURCE_DIR}/tools/get_melon_revision.sh ${PROJECT_SOURCE_DIR} | tr -d '\n'"
        OUTPUT_VARIABLE MELON_RPC_REVISION
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if (NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif ()
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wno-deprecated-declarations -Wno-inconsistent-missing-override")
endif ()

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEFINE_CLOCK_GETTIME}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DFIBER_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DMELON_RPC_REVISION=\\\"${MELON_RPC_REVISION}\\\" -D__STRICT_ANSI__")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL} ${THRIFT_CPP_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-parameter -fno-omit-frame-pointer")

macro(use_cxx17)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endmacro(use_cxx17)

use_cxx17()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if ((CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)") AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
    elseif ((CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
        # segmentation fault in libcontext
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-gcse")
    endif ()
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new")
    endif ()
endif ()

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

find_library(PROTOC_LIB NAMES protoc)
if (NOT PROTOC_LIB)
    message(FATAL_ERROR "Fail to find protoc lib")
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
            "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
            )
endif ()

find_package(OpenSSL)

include_directories(
        ${GFLAGS_INCLUDE_PATH}
        ${PROTOBUF_INCLUDE_DIRS}
        ${LEVELDB_INCLUDE_PATH}
        ${OPENSSL_INCLUDE_DIR}
)

set(DYNAMIC_LIB
        ${GFLAGS_LIBRARY}
        ${PROTOBUF_LIBRARIES}
        ${Protobuf_LITE_LIBRARIES}
        ${LEVELDB_LIB}
        ${PROTOC_LIB}
        ${CMAKE_THREAD_LIBS_INIT}
        ${THRIFT_LIB}
        ${THRIFTNB_LIB}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        dl
        z)


set(MELON_PRIVATE_LIBS "-lgflags -lprotobuf -lleveldb -lprotoc -lssl -lcrypto -ldl -lz")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DYNAMIC_LIB ${DYNAMIC_LIB} rt)
    set(MELON_PRIVATE_LIBS "${MELON_PRIVATE_LIBS} -lrt")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
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
            "-Wl,-U,_ProfilerStop")
endif ()

# for *.so
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)
# for *.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)

# the reason why not using file(GLOB_RECURSE...) is that we want to
# include different files on different platforms.

file(GLOB_RECURSE MELON_BASE_SRC "${PROJECT_SOURCE_DIR}/melon/base/*.cc")
file(GLOB_RECURSE MELON_CONTAINER_SRC "${PROJECT_SOURCE_DIR}/melon/container/*.cc")
file(GLOB_RECURSE MELON_HASH_SRC "${PROJECT_SOURCE_DIR}/melon/hash/*.cc")
file(GLOB_RECURSE MELON_MEMORY_SRC "${PROJECT_SOURCE_DIR}/melon/memory/*.cc")
file(GLOB_RECURSE MELON_IO_SRC "${PROJECT_SOURCE_DIR}/melon/io/*.cc")
file(GLOB_RECURSE MELON_STRINGS_SRC "${PROJECT_SOURCE_DIR}/melon/strings/*.cc")
file(GLOB_RECURSE MELON_DEBUG_SRC "${PROJECT_SOURCE_DIR}/melon/debugging/*.cc")
file(GLOB_RECURSE MELON_LOG_SRC "${PROJECT_SOURCE_DIR}/melon/log/*.cc")
file(GLOB_RECURSE MELON_BOOTSTRAP_SRC "${PROJECT_SOURCE_DIR}/melon/bootstrap/*.cc")
file(GLOB_RECURSE MELON_TIMES_SRC "${PROJECT_SOURCE_DIR}/melon/times/*.cc")
file(GLOB_RECURSE MELON_FILES_SRC "${PROJECT_SOURCE_DIR}/melon/files/*.cc")
file(GLOB_RECURSE MELON_THREAD_SRC "${PROJECT_SOURCE_DIR}/melon/thread/*.cc")
file(GLOB_RECURSE MELON_FUTURE_SRC "${PROJECT_SOURCE_DIR}/melon/future/*.cc")
file(GLOB_RECURSE MELON_DAG_SRC "${PROJECT_SOURCE_DIR}/melon/dag/*.cc")
file(GLOB_RECURSE MELON_SYSTEM_SRC "${PROJECT_SOURCE_DIR}/melon/system/*.cc")
file(GLOB_RECURSE MELON_JSON_SRC "${PROJECT_SOURCE_DIR}/melon/rapidjson/*.cc")

set(MELON_SRC
        "${PROJECT_SOURCE_DIR}/melon/base/dynamic_annotations/dynamic_annotations.c"
        ${MELON_BASE_SRC}
        ${MELON_CONTAINER_SRC}
        ${MELON_HASH_SRC}
        ${MELON_MEMORY_SRC}
        ${MELON_IO_SRC}
        ${MELON_STRINGS_SRC}
        ${MELON_DEBUG_SRC}
        ${MELON_LOG_SRC}
        ${MELON_BOOTSTRAP_SRC}
        ${MELON_HASH_SRC}
        ${MELON_TIMES_SRC}
        ${MELON_FILES_SRC}
        ${MELON_THREAD_SRC}
        ${MELON_FUTURE_SRC}
        ${MELON_DAG_SRC}
        ${MELON_SYSTEM_SRC}
        ${MELON_JSON_SRC}
        )

file(GLOB_RECURSE METRICS_SOURCES "${PROJECT_SOURCE_DIR}/melon/metrics/*.cc")
file(GLOB_RECURSE FIBER_SOURCES "${PROJECT_SOURCE_DIR}/melon/fiber/*.cc")
file(GLOB_RECURSE JSON2PB_SOURCES "${PROJECT_SOURCE_DIR}/melon/json2pb/*.cc")
file(GLOB_RECURSE RPC_SOURCES "${PROJECT_SOURCE_DIR}/melon/rpc/*.cc")
file(GLOB_RECURSE THRIFT_SOURCES "${PROJECT_SOURCE_DIR}/melon/rpc/thrift*.cc")

if (WITH_THRIFT)
    message("melon compile with thrift protocol")
else ()
    # Remove thrift sources
    foreach (v ${THRIFT_SOURCES})
        list(REMOVE_ITEM RPC_SOURCES ${v})
    endforeach ()
    set(THRIFT_SOURCES "")
endif ()

set(MCPACK2PB_SOURCES
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/field_type.cc
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/mcpack2pb.cc
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/parser.cc
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/serializer.cc)

include(CompileProto)
set(PROTO_FILES melon/idl_options.proto
        melon/rpc/rtmp.proto
        melon/rpc/rpc_dump.proto
        melon/rpc/get_favicon.proto
        melon/rpc/span.proto
        melon/rpc/builtin_service.proto
        melon/rpc/get_js.proto
        melon/rpc/errno.proto
        melon/rpc/nshead_meta.proto
        melon/rpc/options.proto
        melon/rpc/policy/baidu_rpc_meta.proto
        melon/rpc/policy/hulu_pbrpc_meta.proto
        melon/rpc/policy/public_pbrpc_meta.proto
        melon/rpc/policy/sofa_pbrpc_meta.proto
        melon/rpc/policy/mongo.proto
        melon/rpc/trackme.proto
        melon/rpc/streaming_rpc_meta.proto
        melon/rpc/proto_base.proto)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/output/include/melon)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${PROTOBUF_INCLUDE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/output/include
        ${PROJECT_SOURCE_DIR}
        "${PROTO_FILES}")
add_library(PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

set(SOURCES
        ${METRICS_SOURCES}
        ${FIBER_SOURCES}
        ${JSON2PB_SOURCES}
        ${MCPACK2PB_SOURCES}
        ${RPC_SOURCES}
        ${THRIFT_SOURCES}
        )

add_subdirectory(melon)

set(MELON_LIB melon-shared)

if (BUILD_EXAMPLES)
    add_subdirectory(example)
endif (BUILD_EXAMPLES)

if (BUILD_BENCHMARK)
    enable_testing()
    add_subdirectory(benchmark)
endif (BUILD_BENCHMARK)

if (BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()
add_subdirectory(tools)

#[[
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/melon/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/include/melon/
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )
]]

file(COPY ${PROJECT_SOURCE_DIR}/melon/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/include/melon/
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )

# Install pkgconfig
configure_file(cmake/melon.pc.in ${PROJECT_BINARY_DIR}/melon.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/melon.pc DESTINATION lib/pkgconfig)
