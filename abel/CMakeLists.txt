
FILE(GLOB CONTAINER_INTERNAL_SRC "container/internal/*.cc")
FILE(GLOB CONTAINER_SRC "container/*.cc")

FILE(GLOB HASH_SRC "hash/internal/*.cc")
FILE(GLOB UTILITY_SRC "utility/*.cc")
FILE(GLOB BASE_SRC "base/*.cc")
FILE(GLOB LOG_SRC "log/*.cc")
FILE(GLOB IO_SRC "io/*.cc")

FILE(GLOB NUMERIC_SRC "numeric/*.cc")
FILE(GLOB MEM_SRC "memory/*.cc")


FILE(GLOB DEBUG_SRC "debugging/*.cc")
FILE(GLOB DEBUG_INTERNAL_SRC "debugging/internal/*.cc")


FILE(GLOB HARDWARE_SRC "hardware/*.cc")

FILE(GLOB FIBER_SRC "fiber/*.cc")
FILE(GLOB FIBER_INTERNAL_SRC "fiber/internal/*.cc")


FILE(GLOB MEMORY_INTERNAL_SRC "memory/internal/*.cc")



FILE(GLOB RND_SRC "random/*.cc")
FILE(GLOB RND_ENGINE_SRC "random/engine/*.cc")
FILE(GLOB RND_INTERNAL_SRC "random/internal/*.cc")
FILE(GLOB RND_SEED_SRC "random/seed/*.cc")

FILE(GLOB STRINGS_SRC "strings/*.cc")
FILE(GLOB STRINGS_INTERNAL_SRC "strings/internal/*.cc")


FILE(GLOB THREAD_INTERNAL_SRC "thread/internal/*.cc")
FILE(GLOB THREAD_SRC "thread/*.cc")

FILE(GLOB METRICS_SRC "metrics/*.cc")

FILE(GLOB DIGEST_SRC "digest/*.cc")
FILE(GLOB SYSTEM_SRC "system/*.cc")

FILE(GLOB LOG_SRC "log/*.cc")

FILE(GLOB CHRONO_SRC "chrono/*.cc")
FILE(GLOB CHRONO_INTERNAL_SRC "chrono/internal/*.cc")

SET(ABEL_SRC
        ${CONTAINER_SRC}
        ${CONTAINER_INTERNAL_SRC}
        ${NUMERIC_SRC}
        ${HASH_SRC}
        ${BASE_SRC}
        ${LOG_SRC}
        ${IO_SRC}
        ${FIBER_INTERNAL_SRC}
        ${FIBER_SRC}
        ${UTILITY_SRC}
        ${MEM_SRC}
        ${CHRONO_SRC}
        ${CHRONO_INTERNAL_SRC}
        ${DEBUG_INTERNAL_SRC}
        ${DEBUG_SRC}
        ${HARDWARE_SRC}
        ${METRICS_SRC}
        ${RND_ENGINE_SRC}
        ${RND_SEED_SRC}
        ${RND_INTERNAL_SRC}
        ${RND_SRC}
        ${STRINGS_SRC}
        ${STRINGS_INTERNAL_SRC}
        ${TYPES_SRC}
        ${DIGEST_SRC}
        ${SYSTEM_SRC}
        ${MEMORY_INTERNAL_SRC}
        ${LOG_SRC}
        ${THREAD_INTERNAL_SRC}
        ${THREAD_SRC}
        )

#
#if (APPLE)
#    find_library(CoreFoundation CoreFoundation)
#    carbin_cc_library(
#            NAME
#            abel
#            SRCS
#            ${ABEL_SRC}
#            COPTS
#            ${CARBIN_DEFAULT_COPTS}
#            DEPS
#                ${CoreFoundation}
#                pthread
#            PUBLIC
#    )
#elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
#    carbin_cc_library(
#            NAME
#            abel
#            SRCS
#            ${ABEL_SRC}
#            COPTS
#            ${CARBIN_DEFAULT_COPTS}
#            DEPS
#            rt
#            pthread
#            PUBLIC
#    )
#endif ()
#

# carbin_cc_library(  NAME myLibrary
#                  NAMESPACE myNamespace
#                  SOURCES
#                       myLib.cpp
#                       myLib_functions.cpp
#                  HEADERS
#                        mylib.h
#                  PUBLIC_DEFINITIONS
#                     USE_DOUBLE_PRECISION=1
#                  PRIVATE_DEFINITIONS
#                     DEBUG_VERBOSE
#                  PUBLIC_INCLUDE_PATHS
#                     ${CMAKE_SOURCE_DIR}/mylib/include
#                  PRIVATE_INCLUDE_PATHS
#                     ${CMAKE_SOURCE_DIR}/include
#                  PRIVATE_LINKED_TARGETS
#                     Threads::Threads
#                  PUBLIC_LINKED_TARGETS
#                     Threads::Threads
#                  LINKED_TARGETS
#                     Threads::Threads
#                  PUBLIC
#                  SHARED


carbin_cc_library(
        NAMESPACE abel
        NAME abel
        SOURCES ${ABEL_SRC}
        PUBLIC_LINKED_TARGETS ${ABEL_DYLINK}
        PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
        PUBLIC
)


install(DIRECTORY ${PROJECT_SOURCE_DIR}/abel
        DESTINATION include
        FILES_MATCHING
        PATTERN "*.h"
        )