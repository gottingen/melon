
FILE(GLOB ALGO_SRC "algorithm/*.cc")
FILE(GLOB BASE_SRC "base/*.cc")
FILE(GLOB BASE_INTERNAL_SRC "base/internal/*.cc")

FILE(GLOB CONMTAINER_INTERNAL_SRC "container/internal/*.cc")

FILE(GLOB DEBUG_SRC "debugging/*.cc")
FILE(GLOB DEBUG_INTERNAL_SRC "debugging/internal/*.cc")

FILE(GLOB FLAGS_SRC "flags/*.cc")
FILE(GLOB FLAGS_INTERNAL_SRC "flags/internal/*.cc")

FILE(GLOB HASH_INTERNAL_SRC "hash/internal/*.cc")

FILE(GLOB MEMORY_INTERNAL_SRC "memory/internal/*.cc")

FILE(GLOB NUMERIC_SRC "numeric/*.cc")

FILE(GLOB RANDOM_SRC "random/*.cc")
FILE(GLOB RANDOM_INTERNAL_SRC "random/internal/*.cc")

FILE(GLOB STRINGS_SRC "strings/*.cc")
FILE(GLOB STRINGS_INTERNAL_SRC "strings/internal/*.cc")

FILE(GLOB FORMAT_SRC "format/*.cc")

FILE(GLOB TYPES_SRC "types/*.cc")

FILE(GLOB SYNC_SRC "synchronization/*.cc")
FILE(GLOB SYNC_INTERNAL_SRC "synchronization/internal/*.cc")

FILE(GLOB THREADING_SRC "threading/*.cc")
FILE(GLOB THREADING_INTERNAL_SRC "threading/internal/*.cc")

FILE(GLOB THREAD_SRC "thread/*.cc")

#FILE(GLOB TIME_SRC "time/*.cc")
#FILE(GLOB TIME_INTERNAL_SRC "time/internal/*.cc")


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
        ${BASE_INTERNAL_SRC}
        ${CONMTAINER_INTERNAL_SRC}
        ${DEBUG_INTERNAL_SRC}
        ${DEBUG_SRC}
        ${FLAGS_INTERNAL_SRC}
        ${FLAGS_SRC}
        ${HASH_INTERNAL_SRC}
        ${NUMERIC_SRC}
        ${RANDOM_SRC}
        ${RANDOM_INTERNAL_SRC}
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
target_link_libraries(abel_static pthread)
if(APPLE)
        find_library(CoreFoundation CoreFoundation)
        target_link_libraries(abel_static ${CoreFoundation} pthread)
else()
        target_link_libraries(abel_static pthread)
endif()