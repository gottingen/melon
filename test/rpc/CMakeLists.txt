

# rpc tests
file(GLOB RPC_UNITTESTS "rpc_*_unittest.cc")
foreach(RPC_UT ${RPC_UNITTESTS})
    get_filename_component(RPC_UT_WE ${RPC_UT} NAME_WE)
    add_executable(${RPC_UT_WE} ${RPC_UT} $<TARGET_OBJECTS:TEST_PROTO_LIB>)
    target_link_libraries(${RPC_UT_WE} melon-shared-debug
            gtest_main
            ${GPERFTOOLS_LIBRARIES})
    add_test(NAME ${RPC_UT_WE} COMMAND ${RPC_UT_WE})
endforeach()
