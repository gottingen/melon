
file(GLOB RAPIDJSON_SRC "*.cc")

add_executable(rapidjson_test
        ${RAPIDJSON_SRC}
        )

target_link_libraries(rapidjson_test
        gtest
        gmock
        gtest_main
        testing_static
        abel_static
        pthread
        )

add_test(
        NAME rapidjson_test
        COMMAND rapidjson_test
)