#
# Copyright 2023 The Carbin Authors.
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
############################################################
# system pthread and rt, dl
############################################################
#[[
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
            "-Wl,-U,_ProfilerStop"
            "-Wl,-U,__Z13GetStackTracePPvii")
endif ()
]]
find_package(Threads REQUIRED)
set(CARBIN_SYSTEM_DYLINK)
if (APPLE)
    find_library(CoreFoundation CoreFoundation)
    list(APPEND CARBIN_SYSTEM_DYLINK ${CoreFoundation} pthread)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND CARBIN_SYSTEM_DYLINK rt dl pthread)
endif ()

if (CARBIN_BUILD_TEST)
    enable_testing()
    #include(require_gtest)
    #include(require_gmock)
    #include(require_doctest)
endif (CARBIN_BUILD_TEST)

if (CARBIN_BUILD_BENCHMARK)
    #include(require_benchmark)
endif ()

set(CARBIN_DEPS_INCLUDE "")
############################################################
# gflags
############################################################
carbin_find_gflags()
message(STATUS "GFLAGS_INCLUDE_PATH: ${GFLAGS_INCLUDE_PATH}")
message(STATUS "GFLAGS_DEV_FOUND : ${GFLAGS_DEV_FOUND}")
if (NOT GFLAGS_DEV_FOUND)
    message(FATAL_ERROR "Fail to find gflags")
endif ()
list(APPEND CARBIN_DEPS_INCLUDE ${GFLAGS_INCLUDE_PATH})
############################################################
# leveldb
############################################################
carbin_find_leveldb()
if (NOT LEVELDB_DEV_FOUND)
    message(FATAL_ERROR "Fail to find leveldb")
endif ()
list(APPEND CARBIN_DEPS_INCLUDE ${LEVELDB_INCLUDE_PATH})
############################################################
# protobuf
############################################################
find_package(Protobuf REQUIRED)
list(APPEND CARBIN_DEPS_INCLUDE ${PROTOBUF_INCLUDE_DIRS})
find_library(PROTOC_LIB NAMES protoc)
if (NOT PROTOC_LIB)
    message(FATAL_ERROR "Fail to find protoc lib")
endif ()
############################################################
# openssl
############################################################
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
            "/usr/local/opt/openssl" # Homebrew installed OpenSSL
    )
endif ()

find_package(OpenSSL REQUIRED)

list(APPEND CARBIN_DEPS_INCLUDE ${OPENSSL_INCLUDE_DIR})

############################################################
# zlib
############################################################
carbin_find_zlib()
if (NOT ZLIB_DEV_FOUND)
    message(FATAL_ERROR "Fail to find zlib")
endif ()
list(APPEND CARBIN_DEPS_INCLUDE ${ZLIB_INCLUDE_PATH})
############################################################
#
# add you libs to the CARBIN_DEPS_LINK variable eg as turbo
# so you can and system pthread and rt, dl already add to
# CARBIN_SYSTEM_DYLINK, using it for fun.
##########################################################
set(MELON_DEPS_LINK
        ${GFLAGS_SHARED_LIB}
        ${PROTOBUF_LIBRARIES}
        ${LEVELDB_SHARED_LIB}
        ${PROTOC_LIB}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
        ${ZLIB_SHARED_LIB}
        ${CARBIN_SYSTEM_DYLINK}
        )
list(REMOVE_DUPLICATES MELON_DEPS_LINK)
list(REMOVE_DUPLICATES CARBIN_DEPS_INCLUDE)
carbin_print_list_label("Denpendcies:" MELON_DEPS_LINK)
carbin_print_list_label("Denpendcies:" CARBIN_DEPS_INCLUDE)





