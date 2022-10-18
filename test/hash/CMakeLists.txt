file(GLOB BASE_UNITTESTS "*test.cc")
foreach(BASE_UT ${BASE_UNITTESTS})
    get_filename_component(BASE_UT_WE ${BASE_UT} NAME_WE)
    add_executable(${BASE_UT_WE} ${BASE_UT} $<TARGET_OBJECTS:TEST_PROTO_LIB>)
    target_link_libraries(${BASE_UT_WE} melon-shared-debug
            ${GTEST_MAIN_LIB}
            ${GTEST_LIB}
            ${GMOCK_LIB}
            ${GPERFTOOLS_LIBRARIES})
    add_test(NAME ${BASE_UT_WE} COMMAND ${BASE_UT_WE})
endforeach()