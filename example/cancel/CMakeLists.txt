
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(cancel_client client.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(cancel_server server.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(cancel_client ${MELON_LIB} ${DYNAMIC_LIB})
target_link_libraries(cancel_server ${MELON_LIB} ${DYNAMIC_LIB})
