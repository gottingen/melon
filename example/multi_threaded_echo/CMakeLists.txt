
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(mt_echo_client client.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(mt_echo_server server.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(mt_echo_client ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(mt_echo_server ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})

file(COPY key.pem
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY cert.pem
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
