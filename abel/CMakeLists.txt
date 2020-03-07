
FILE(GLOB ALGO_SRC "algorithm/*.cc")
FILE(GLOB BASE_SRC "base/*.cc")

FILE(GLOB CONMTAINER_INTERNAL_SRC "container/internal/*.cc")
FILE(GLOB CONMTAINER_SRC "container/*.cc")

FILE(GLOB DEBUG_SRC "debugging/*.cc")
FILE(GLOB DEBUG_INTERNAL_SRC "debugging/internal/*.cc")

FILE(GLOB CONFIG_FLAGS_SRC "config/flags/*.cc")
FILE(GLOB CONFIG_FLAGS_INTERNAL_SRC "config/flags/internal/*.cc")

FILE(GLOB HARDWARE_SRC "hardware/*.cc")

FILE(GLOB HASH_INTERNAL_SRC "hash/internal/*.cc")

FILE(GLOB MEMORY_INTERNAL_SRC "memory/internal/*.cc")

FILE(GLOB NUMERIC_SRC "numeric/*.cc")


FILE(GLOB STATS_RND_SRC "stats/random/*.cc")
FILE(GLOB STATS_RND_ENGINE_SRC "stats/random/engine/*.cc")
FILE(GLOB STATS_RND_INTERNAL_SRC "stats/random/internal/*.cc")
FILE(GLOB STATS_RND_SEED_SRC "stats/random/seed/*.cc")

FILE(GLOB STRINGS_SRC "strings/*.cc")
FILE(GLOB STRINGS_INTERNAL_SRC "strings/internal/*.cc")

FILE(GLOB FORMAT_SRC "format/*.cc")

FILE(GLOB TYPES_SRC "types/*.cc")

FILE(GLOB SYNC_SRC "synchronization/*.cc")
FILE(GLOB SYNC_INTERNAL_SRC "synchronization/internal/*.cc")

FILE(GLOB THREADING_SRC "threading/*.cc")
FILE(GLOB THREADING_INTERNAL_SRC "threading/internal/*.cc")

FILE(GLOB THREAD_SRC "thread/*.cc")

FILE(GLOB METRICS_SRC "metrics/*.cc")

FILE(GLOB DIGEST_SRC "digest/*.cc")
FILE(GLOB SYSTEM_SRC "system/*.cc")

FILE(GLOB LOG_SRC "log/*.cc")

FILE(GLOB CHRONO_SRC "chrono/*.cc")
FILE(GLOB CHRONO_INTERNAL_SRC "chrono/internal/*.cc")

SET(ABEL_SRC
        ${CHRONO_SRC}
        ${CHRONO_INTERNAL_SRC}
        ${ALGO_SRC}
        ${BASE_SRC}
        ${CONMTAINER_INTERNAL_SRC}
        ${CONMTAINER_SRC}
        ${DEBUG_INTERNAL_SRC}
        ${DEBUG_SRC}
        ${CONFIG_FLAGS_INTERNAL_SRC}
        ${CONFIG_FLAGS_SRC}
        ${HARDWARE_SRC}
        ${HASH_INTERNAL_SRC}
        ${METRICS_SRC}
        ${NUMERIC_SRC}
        ${STATS_RND_ENGINE_SRC}
        ${STATS_RND_SEED_SRC}
        ${STATS_RND_INTERNAL_SRC}
        ${STATS_RND_SRC}
        ${STRINGS_SRC}
        ${STRINGS_INTERNAL_SRC}
        ${FORMAT_SRC}
        ${TYPES_SRC}
        ${SYNC_SRC}
        ${SYNC_INTERNAL_SRC}
        ${DIGEST_SRC}
        ${SYSTEM_SRC}
        ${THREADING_SRC}
        ${THREADING_INTERNAL_SRC}
        ${MEMORY_INTERNAL_SRC}
        ${LOG_SRC}
        ${THREAD_SRC}
        )

add_library(abel_static STATIC ${ABEL_SRC})
add_library(abel_shared SHARED ${ABEL_SRC})

target_link_libraries(abel_static pthread)
if (APPLE)
    find_library(CoreFoundation CoreFoundation)
    target_link_libraries(abel_static ${CoreFoundation} pthread)
    target_link_libraries(abel_shared ${CoreFoundation} pthread)
else ()
    target_link_libraries(abel_static pthread)
endif ()

set_target_properties(abel_shared PROPERTIES
        OUTPUT_NAME abel
        VERSION ${CMAKE_PROJECT_VERSION}
        SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
        )

set_target_properties(abel_static PROPERTIES
        OUTPUT_NAME abel
        VERSION ${CMAKE_PROJECT_VERSION}
        SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
        )

install(TARGETS abel_static abel_shared
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

file(COPY ${PROJECT_SOURCE_DIR}/abel
        DESTINATION ${PROJECT_BINARY_DIR}/include
        FILES_MATCHING
        PATTERN "*.h"
        )

install(DIRECTORY ${PROJECT_BINARY_DIR}/include/abel
        DESTINATION include
        FILES_MATCHING
        PATTERN "*.h"
        )