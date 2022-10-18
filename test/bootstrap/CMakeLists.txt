
add_executable(reset_flags_test reset_flags_test.cc)
target_link_libraries(reset_flags_test melon-shared-debug
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GPERFTOOLS_LIBRARIES})
add_test(NAME symbolize_test COMMAND reset_flags_test)