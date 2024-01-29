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

find_package(Gperftools)
include_directories(${GPERFTOOLS_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#[[
include(CompileProto)
set(TEST_PROTO_FILES addressbook1.proto
                     addressbook_encode_decode.proto
                     addressbook_map.proto
                     addressbook.proto
                     echo.proto
                     iobuf.proto
                     message.proto
                     repeated.proto
                     snappy_message.proto
                     v1.proto
                     v2.proto
                     grpc.proto
                     health_check.proto)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/hdrs)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${PROJECT_SOURCE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${PROJECT_BINARY_DIR}/test
                                    ${PROJECT_BINARY_DIR}/test/hdrs
                                    ${PROJECT_SOURCE_DIR}/test
                                    "${TEST_PROTO_FILES}")
add_library(TEST_PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})
]]
set(MELON_SYSTEM_GTEST_SOURCE_DIR "" CACHE PATH "System googletest source directory.")

if(DOWNLOAD_GTEST)
    include(SetupGtest)
elseif(MELON_SYSTEM_GTEST_SOURCE_DIR)
    add_subdirectory("${MELON_SYSTEM_GTEST_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/system-googletest-build")
else()
    message(FATAL_ERROR "Googletest is not available")
endif()

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEFINE_CLOCK_GETTIME} -DBRPC_WITH_GLOG=${WITH_GLOG_VAL} -DBRPC_WITH_RDMA=${WITH_RDMA_VAL} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__=__unused__ -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DUNIT_TEST -Dprivate=public -Dprotected=public -DMELON_VAR_NOT_LINK_DEFAULT_VARIABLES -D__STRICT_ANSI__ -include ${PROJECT_SOURCE_DIR}/test/sstream_workaround.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -g -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
use_cxx11()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    #required by butil/crc32.cc to boost performance for 10x
    if((CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)") AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
    endif()
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new")
    endif()
endif()

file(COPY ${PROJECT_SOURCE_DIR}/test/cert1.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert2.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert1.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert2.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/jsonout DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/run_tests.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)

add_subdirectory(util)
add_subdirectory(var)
add_subdirectory(bthread)
add_subdirectory(rpc)

if(BUILD_FUZZ_TESTS)
    add_library(melon-static-debug STATIC $<TARGET_OBJECTS:BUTIL_DEBUG_LIB>
                                        $<TARGET_OBJECTS:SOURCES_DEBUG_LIB>
                                        $<TARGET_OBJECTS:PROTO_LIB>)
    # change the debug lib output dir to be different from the release output
    set_target_properties(melon-static-debug PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/test)

    target_link_libraries(melon-static-debug ${DYNAMIC_LIB})
    if(BRPC_WITH_GLOG)
        target_link_libraries(melon-static-debug ${GLOG_LIB})
    endif()

    set(FUZZ_TARGETS fuzz_butil fuzz_esp fuzz_hpack fuzz_http
        fuzz_hulu fuzz_json fuzz_redis fuzz_shead fuzz_sofa fuzz_uri)

    foreach(target ${FUZZ_TARGETS})
        add_executable(${target} fuzzing/${target}.cc $<TARGET_OBJECTS:TEST_PROTO_LIB>)
        target_link_libraries(${target} melon-static-debug ${LIB_FUZZING_ENGINE})
        set_target_properties(${target} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN/lib")
    endforeach()
endif()
