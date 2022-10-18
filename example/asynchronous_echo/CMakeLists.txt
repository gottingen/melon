
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(asynchronous_echo_client client.cc ${PROTO_SRC})
add_executable(asynchronous_echo_server server.cc ${PROTO_SRC})

target_link_libraries(asynchronous_echo_client melon-shared ${DYNAMIC_LIB})
target_link_libraries(asynchronous_echo_server melon-shared ${DYNAMIC_LIB})
