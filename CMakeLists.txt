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

cmake_minimum_required(VERSION 3.15)
project(melon C CXX)

option(WITH_MESALINK "With MesaLink" OFF)
option(WITH_BORINGSSL "With BoringSSL" OFF)
option(DEBUG "Print debug logs" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)
option(WITH_SNAPPY "With snappy" OFF)
option(WITH_RDMA "With RDMA" OFF)
option(BUILD_UNIT_TESTS "Whether to build unit tests" ON)
option(BUILD_FUZZ_TESTS "Whether to build fuzz tests" OFF)
option(BUILD_MELON_TOOLS "Whether to build melon tools" ON)
option(DOWNLOAD_GTEST "Download and build a fresh copy of googletest. Requires Internet access." ON)

list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})

# Enable MACOSX_RPATH. Run "cmake --help-policy CMP0042" for policy details.
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

set(BRPC_VERSION 1.6.0)

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

set(WITH_RDMA_VAL "0")
if (WITH_RDMA)
    set(WITH_RDMA_VAL "1")
endif ()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(gflags REQUIRED)
find_package(Snappy REQUIRED)
find_package(leveldb REQUIRED)
message(STATUS "Found gflags: ${gflags_INCLUDE_DIR}")

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

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/output/include
)

execute_process(
        COMMAND bash -c "${PROJECT_SOURCE_DIR}/tools/get_brpc_revision.sh ${PROJECT_SOURCE_DIR} | tr -d '\n'"
        OUTPUT_VARIABLE BRPC_REVISION
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if (NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif ()
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wno-deprecated-declarations -Wno-inconsistent-missing-override")
endif ()

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEFINE_CLOCK_GETTIME} -DBRPC_WITH_RDMA=${WITH_RDMA_VAL} -DGFLAGS_NS=${GFLAGS_NS}")
if (WITH_MESALINK)
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DUSE_MESALINK")
endif ()
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__=__unused__ -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DBRPC_REVISION=\\\"${BRPC_REVISION}\\\" -D__STRICT_ANSI__")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL} ${THRIFT_CPP_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-parameter -fno-omit-frame-pointer")

macro(use_cxx17)
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
endmacro(use_cxx17)

use_cxx17()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    #required by butil/crc32.cc to boost performance for 10x
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

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif ()

if (WITH_SNAPPY)
    find_path(SNAPPY_INCLUDE_PATH NAMES snappy.h)
    find_library(SNAPPY_LIB NAMES snappy)
    if ((NOT SNAPPY_INCLUDE_PATH) OR (NOT SNAPPY_LIB))
        message(FATAL_ERROR "Fail to find snappy")
    endif ()
    include_directories(${SNAPPY_INCLUDE_PATH})
endif ()

find_path(GLOG_INCLUDE_PATH NAMES glog/logging.h)
find_library(GLOG_LIB NAMES glog)
if ((NOT GLOG_INCLUDE_PATH) OR (NOT GLOG_LIB))
    message(FATAL_ERROR "Fail to find glog")
endif ()
include_directories(${GLOG_INCLUDE_PATH})

if (WITH_MESALINK)
    find_path(MESALINK_INCLUDE_PATH NAMES mesalink/openssl/ssl.h)
    find_library(MESALINK_LIB NAMES mesalink)
    if ((NOT MESALINK_INCLUDE_PATH) OR (NOT MESALINK_LIB))
        message(FATAL_ERROR "Fail to find MesaLink")
    else ()
        message(STATUS "Found MesaLink: ${MESALINK_LIB}")
    endif ()
    include_directories(${MESALINK_INCLUDE_PATH})
endif ()

if (WITH_RDMA)
    message("melon compile with rdma")
    find_path(RDMA_INCLUDE_PATH NAMES infiniband/verbs.h)
    find_library(RDMA_LIB NAMES ibverbs)
    if ((NOT RDMA_INCLUDE_PATH) OR (NOT RDMA_LIB))
        message(FATAL_ERROR "Fail to find ibverbs")
    endif ()
endif ()

find_library(PROTOC_LIB NAMES protoc)
if (NOT PROTOC_LIB)
    message(FATAL_ERROR "Fail to find protoc lib")
endif ()

if (WITH_BORINGSSL)
    find_package(BoringSSL)
    include_directories(${BORINGSSL_INCLUDE_DIR})
else ()
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(OPENSSL_ROOT_DIR
                "/usr/local/opt/openssl" # Homebrew installed OpenSSL
        )
    endif ()

    find_package(OpenSSL)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif ()

include_directories(
        ${gflags_INCLUDE_DIR}
        ${PROTOBUF_INCLUDE_DIRS}
        ${LEVELDB_INCLUDE_PATH}
)
message(STATUS "Found gflags: ${gflags_LIBRARIES}")
set(DYNAMIC_LIB
        ${gflags_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${LEVELDB_LIB}
        ${PROTOC_LIB}
        ${CMAKE_THREAD_LIBS_INIT}
        dl
        z)

if (WITH_BORINGSSL)
    list(APPEND DYNAMIC_LIB ${BORINGSSL_SSL_LIBRARY})
    list(APPEND DYNAMIC_LIB ${BORINGSSL_CRYPTO_LIBRARY})
else ()
    list(APPEND DYNAMIC_LIB ${OPENSSL_CRYPTO_LIBRARY})
    if (WITH_MESALINK)
        list(APPEND DYNAMIC_LIB ${MESALINK_LIB})
    else ()
        list(APPEND DYNAMIC_LIB ${OPENSSL_SSL_LIBRARY})
    endif ()
endif ()

if (WITH_RDMA)
    list(APPEND DYNAMIC_LIB ${RDMA_LIB})
endif ()

set(BRPC_PRIVATE_LIBS "-lgflags -lprotobuf -lleveldb -lprotoc -lssl -lcrypto -ldl -lz")

set(DYNAMIC_LIB ${DYNAMIC_LIB} ${GLOG_LIB})
set(BRPC_PRIVATE_LIBS "${BRPC_PRIVATE_LIBS} -lglog")

if (WITH_SNAPPY)
    set(DYNAMIC_LIB ${DYNAMIC_LIB} ${SNAPPY_LIB})
    set(BRPC_PRIVATE_LIBS "${BRPC_PRIVATE_LIBS} -lsnappy")
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DYNAMIC_LIB ${DYNAMIC_LIB} rt)
    set(BRPC_PRIVATE_LIBS "${BRPC_PRIVATE_LIBS} -lrt")
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
            "-Wl,-U,_ProfilerStop"
            "-Wl,-U,__Z13GetStackTracePPvii")
endif ()

# for *.so
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)
# for *.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)

# the reason why not using file(GLOB_RECURSE...) is that we want to
# include different files on different platforms.
set(BUTIL_SOURCES
        ${PROJECT_SOURCE_DIR}/melon/common/logging.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/dmg_fp/g_fmt.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/dmg_fp/dtoa_wrapper.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/dynamic_annotations/dynamic_annotations.c
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/icu/icu_utf.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/superfasthash/superfasthash.c
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/modp_b64/modp_b64.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/symbolize/demangle.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/symbolize/symbolize.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/snappy/snappy-sinksource.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/snappy/snappy-stubs-internal.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/snappy/snappy.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/third_party/murmurhash3/murmurhash3.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/arena.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/at_exit.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/atomicops_internals_x86_gcc.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/base64.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/base64url.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/big_endian.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/cpu.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/alias.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/asan_invalid_access.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/crash_logging.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/debugger.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/debugger_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/dump_without_crashing.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/proc_maps_linux.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/stack_trace.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/debug/stack_trace_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/environment.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file_enumerator.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file_enumerator_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file_path.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file_path_constants.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/memory_mapped_file.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/memory_mapped_file_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/scoped_file.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/files/scoped_temp_dir.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/file_util.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/file_util_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/guid.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/guid_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/hash.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/lazy_instance.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/location.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/memory/aligned_memory.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/memory/ref_counted.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/memory/ref_counted_memory.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/memory/singleton.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/memory/weak_ptr.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/posix/file_descriptor_shuffle.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/posix/global_descriptors.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/process_util.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/rand_util.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/rand_util_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/fast_rand.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/safe_strerror_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/sha1_portable.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/latin1_string_conversions.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/nullable_string16.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/safe_sprintf.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/string16.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/string_number_conversions.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/string_split.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/string_piece.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/string_util.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/string_util_constants.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/stringprintf.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/utf_offset_string_conversions.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/utf_string_conversion_utils.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/strings/utf_string_conversions.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/synchronization/cancellation_flag.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/synchronization/condition_variable_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/synchronization/waitable_event_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/non_thread_safe_impl.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/platform_thread_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/simple_thread.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_checker_impl.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_collision_warner.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_id_name_manager.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_local_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_local_storage.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_local_storage_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/thread_restrictions.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/threading/watchdog.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/time/clock.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/time/default_clock.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/time/default_tick_clock.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/time/tick_clock.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/time/time.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/time/time_posix.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/version.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/class_name.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/errno.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/find_cstr.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/status.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/string_printf.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/thread_local.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/thread_key.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/unix_socket.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/endpoint.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/fd_utility.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/files/temp_file.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/files/file_watcher.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/time.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/zero_copy_stream_as_streambuf.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/crc32c.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/containers/case_ignored_flat_map.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/iobuf.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/binary_printer.cpp
        ${PROJECT_SOURCE_DIR}/melon/butil/recordio.cc
        ${PROJECT_SOURCE_DIR}/melon/butil/popen.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BUTIL_SOURCES ${BUTIL_SOURCES}
            ${PROJECT_SOURCE_DIR}/melon/butil/file_util_linux.cc
            ${PROJECT_SOURCE_DIR}/melon/butil/threading/platform_thread_linux.cc
            ${PROJECT_SOURCE_DIR}/melon/butil/strings/sys_string_conversions_posix.cc)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUTIL_SOURCES ${BUTIL_SOURCES}
            ${PROJECT_SOURCE_DIR}/melon/butil/mac/bundle_locations.mm
            ${PROJECT_SOURCE_DIR}/melon/butil/mac/foundation_util.mm
            ${PROJECT_SOURCE_DIR}/melon/butil/file_util_mac.mm
            ${PROJECT_SOURCE_DIR}/melon/butil/threading/platform_thread_mac.mm
            ${PROJECT_SOURCE_DIR}/melon/butil/strings/sys_string_conversions_mac.mm
            ${PROJECT_SOURCE_DIR}/melon/butil/time/time_mac.cc
            ${PROJECT_SOURCE_DIR}/melon/butil/mac/scoped_mach_port.cc)
endif ()

file(GLOB_RECURSE VAR_SOURCES "${PROJECT_SOURCE_DIR}/melon/var/*.cc")
file(GLOB_RECURSE BTHREAD_SOURCES "${PROJECT_SOURCE_DIR}/melon/bthread/*.cc")
file(GLOB_RECURSE JSON2PB_SOURCES "${PROJECT_SOURCE_DIR}/melon/json2pb/*.cc")
file(GLOB_RECURSE RPC_SOURCES "${PROJECT_SOURCE_DIR}/melon/rpc/*.cc")
file(GLOB_RECURSE NAMING_SOURCES "${PROJECT_SOURCE_DIR}/melon/naming/*.cc")
file(GLOB_RECURSE LB_SOURCES "${PROJECT_SOURCE_DIR}/melon/lb/*.cc")
file(GLOB_RECURSE BUILTIN_SOURCES "${PROJECT_SOURCE_DIR}/melon/builtin/*.cc")
file(GLOB_RECURSE COMPRESS_SOURCES "${PROJECT_SOURCE_DIR}/melon/compress/*.cc")
file(GLOB_RECURSE RAFT_SOURCES "${PROJECT_SOURCE_DIR}/melon/raft/*.cc")
file(GLOB_RECURSE CORE_SOURCES "${PROJECT_SOURCE_DIR}/melon/core/*.cc")
file(GLOB_RECURSE EXCLUDE_SOURCES "${PROJECT_SOURCE_DIR}/melon/rpc/event_dispatcher_*.cc")


foreach (v ${EXCLUDE_SOURCES})
    list(REMOVE_ITEM RPC_SOURCES ${v})
endforeach ()

set(MCPACK2PB_SOURCES
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/field_type.cpp
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/mcpack2pb.cpp
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/parser.cpp
        ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/serializer.cpp)

include(CompileProto)
set(PROTO_FILES melon/idl_options.proto
        melon/proto/rpc/rtmp.proto
        melon/proto/rpc/rpc_dump.proto
        melon/proto/rpc/get_favicon.proto
        melon/proto/rpc/span.proto
        melon/proto/rpc/builtin_service.proto
        melon/proto/rpc/grpc_health_check.proto
        melon/proto/rpc/get_js.proto
        melon/proto/rpc/errno.proto
        melon/proto/rpc/nshead_meta.proto
        melon/proto/rpc/options.proto
        melon/proto/rpc/baidu_rpc_meta.proto
        melon/proto/rpc/hulu_pbrpc_meta.proto
        melon/proto/rpc/public_pbrpc_meta.proto
        melon/proto/rpc/sofa_pbrpc_meta.proto
        melon/proto/rpc/mongo.proto
        melon/proto/rpc/trackme.proto
        melon/proto/rpc/streaming_rpc_meta.proto
        melon/proto/rpc/proto_base.proto
        melon/proto/raft/builtin_service.proto
        melon/proto/raft/cli.proto
        melon/proto/raft/enum.proto
        melon/proto/raft/errno.proto
        melon/proto/raft/file_service.proto
        melon/proto/raft/local_file_meta.proto
        melon/proto/raft/local_storage.proto
        melon/proto/raft/raft.proto
)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/output/include/melon/rpc)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${PROTOBUF_INCLUDE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/output/include
        ${PROJECT_SOURCE_DIR}
        "${PROTO_FILES}")
add_library(PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

set(SOURCES
        ${VAR_SOURCES}
        ${BTHREAD_SOURCES}
        ${JSON2PB_SOURCES}
        ${MCPACK2PB_SOURCES}
        ${RPC_SOURCES}
        ${NAMING_SOURCES}
        ${LB_SOURCES}
        ${COMPRESS_SOURCES}
        ${BUILTIN_SOURCES}
        ${CORE_SOURCES}
        ${RAFT_SOURCES}
)

add_subdirectory(melon)
if (BUILD_UNIT_TESTS)

    set(TEST_PROTO_FILES
            test/addressbook1.proto
            test/addressbook_encode_decode.proto
            test/addressbook_map.proto
            test/addressbook.proto
            test/echo.proto
            test/iobuf.proto
            test/message.proto
            test/repeated.proto
            test/snappy_message.proto
            test/v1.proto
            test/v2.proto
            test/grpc.proto
            test/health_check.proto)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test/hdrs)
    set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${PROJECT_SOURCE_DIR})
    compile_proto(PROTO_HDRS PROTO_SRCS ${PROJECT_BINARY_DIR}
            ${PROJECT_BINARY_DIR}/test/hdrs
            ${PROJECT_SOURCE_DIR}
            "${TEST_PROTO_FILES}")
    add_library(TEST_PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

    enable_testing()
    add_subdirectory(test)
endif ()

if (BUILD_FUZZ_TESTS)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Fuzzing is only supported with clang")
    endif ()
    if (NOT BUILD_UNIT_TESTS)
        message(FATAL_ERROR "BUILD_UNIT_TESTS must be enabled to build fuzz tests")
    endif ()
endif ()

if (BUILD_MELON_TOOLS)
    add_subdirectory(tools)
endif ()
#[[
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/brpc/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/include/brpc/
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )
        ]]
file(COPY ${PROJECT_SOURCE_DIR}/melon/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/include/melon
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
install(FILES ${PROJECT_BINARY_DIR}/melon.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
