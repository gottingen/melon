

file(GLOB FIBER_UNITTESTS "fiber*test.cc")
foreach(FIBER_UT ${FIBER_UNITTESTS})
    get_filename_component(FIBER_UT_WE ${FIBER_UT} NAME_WE)
    add_executable(${FIBER_UT_WE} ${FIBER_UT})
    target_link_libraries(${FIBER_UT_WE}
            ${GTEST_MAIN_LIB}
            ${GTEST_LIB}
            ${GMOCK_LIB}
            melon-shared-debug
            ${GPERFTOOLS_LIBRARIES})
    add_test(NAME ${FIBER_UT_WE} COMMAND ${FIBER_UT_WE})
endforeach()