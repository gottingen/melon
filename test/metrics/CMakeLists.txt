

file(GLOB TEST_VARIABLE_SRCS "*_test.cc")
add_executable(metrics_test ${TEST_VARIABLE_SRCS}
        $<TARGET_OBJECTS:VARIABLE_DEBUG_LIB>
        $<TARGET_OBJECTS:MELON_BASE_DEBUG_LIB>)
target_link_libraries(metrics_test ${GTEST_LIB}
        ${GPERFTOOLS_LIBRARIES}
        ${DYNAMIC_LIB})
add_test(NAME metrics_test COMMAND metrics_test)