
include(FindThreads)
include(FindProtobuf)

find_path(GPERFTOOLS_INCLUDE_DIR NAMES gperftools/heap-profiler.h)
find_library(GPERFTOOLS_LIBRARIES NAMES tcmalloc_and_profiler)
include_directories(${GPERFTOOLS_INCLUDE_DIR})

add_executable(redis_cli redis_cli.cc)
add_executable(redis_press redis_press.cc)
add_executable(redis_server redis_server.cc)

set(AUX_LIB readline ncurses)

target_link_libraries(redis_cli ${MELON_LIB} ${DYNAMIC_LIB} ${AUX_LIB})
target_link_libraries(redis_press ${MELON_LIB} ${DYNAMIC_LIB})
target_link_libraries(redis_server ${MELON_LIB} ${DYNAMIC_LIB})
