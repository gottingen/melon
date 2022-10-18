

include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER cl_test.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(auto_concurrency_limiter_client client.cc ${PROTO_SRC})
add_executable(auto_concurrency_limiter_server server.cc ${PROTO_SRC})

target_link_libraries(auto_concurrency_limiter_client melon-shared ${DYNAMIC_LIB})
target_link_libraries(auto_concurrency_limiter_server melon-shared ${DYNAMIC_LIB})
