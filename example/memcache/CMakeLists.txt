
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(memcache_client client.cc)

target_link_libraries(memcache_client ${MELON_LIB} ${DYNAMIC_LIB})
