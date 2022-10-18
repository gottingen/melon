
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER http.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(http_client http_client.cc)
add_executable(http_server http_server.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(benchmark_http benchmark_http.cc)

target_link_libraries(http_server ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(http_client ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(benchmark_http ${MELON_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})

file(COPY key.pem
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY cert.pem
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
