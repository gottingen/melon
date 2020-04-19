include_directories(${PROJECT_SOURCE_DIR}/test)

find_package(GTest REQUIRED)
find_package(GMock REQUIRED)


carbin_check_target(GTest::Main)
carbin_check_target(GTest::GTest)

if (APPLE)
    set(TEST_DEPS
            "${GMOCK_LIBRARY}"
            "GTest::Main"
            "GTest::GTest"
            "abel::abel"
            "pthread"
            "testing_static"
            )
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(TEST_DEPS
            "${GMOCK_LIBRARY}"
            "GTest::Main"
            "GTest::GTest"
            "abel::abel"
            "pthread"
            "testing_static"
            "rt"
            )
endif ()
add_subdirectory(testing)
add_subdirectory(asl)
add_subdirectory(atomic)
add_subdirectory(base)
add_subdirectory(chrono)
add_subdirectory(config)
add_subdirectory(debugging)
add_subdirectory(digest)
add_subdirectory(log)
add_subdirectory(math)
add_subdirectory(memory)
add_subdirectory(metrics)
add_subdirectory(rapidjson)
add_subdirectory(stats)
add_subdirectory(strings)
add_subdirectory(system)
add_subdirectory(thread)




