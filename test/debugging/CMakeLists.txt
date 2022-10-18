file(GLOB BASE_UNITTESTS "*test.cc")
list(REMOVE_ITEM BASE_UNITTESTS ${PROJECT_SOURCE_DIR}/test/debugging/symbolize_test.cc)
foreach(BASE_UT ${BASE_UNITTESTS})
    get_filename_component(BASE_UT_WE ${BASE_UT} NAME_WE)
    add_executable(${BASE_UT_WE} ${BASE_UT})
    target_link_libraries(${BASE_UT_WE} melon-shared-debug
            ${GTEST_MAIN_LIB}
            ${GTEST_LIB}
            ${GMOCK_LIB}
            ${GPERFTOOLS_LIBRARIES})
    add_test(NAME ${BASE_UT_WE} COMMAND ${BASE_UT_WE})
endforeach()

add_executable(symbolize_test symbolize_test.cc)
target_link_libraries(symbolize_test melon-shared-debug
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GPERFTOOLS_LIBRARIES})
add_test(NAME symbolize_test COMMAND symbolize_test)