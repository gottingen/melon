
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(nshead_pb_extension_client client.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(nshead_pb_extension_server server.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(nshead_pb_extension_client ${MELON_LIB} ${DYNAMIC_LIB})
target_link_libraries(nshead_pb_extension_server ${MELON_LIB} ${DYNAMIC_LIB})
