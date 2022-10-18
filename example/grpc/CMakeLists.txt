
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER helloworld.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(grpc_server server.cc ${PROTO_SRC} ${PROTO_HEADER} )
add_executable(grpc_client client.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(grpc_server ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(grpc_client ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
