
find_package(Gperftools)
include_directories(${GPERFTOOLS_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/test)

include(require_gtest)
include(require_gmock)

add_definitions(
        -Dprivate=public
        -Dprotected=public
)
add_subdirectory(testing)
set(TEST_LINKS)
list(APPEND TEST_LINKS
        ${DYNAMIC_LIB}
        ${GTEST_MAIN_LIB}
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GPERFTOOLS_LIBRARIES}
        "melon::testing"
        "melon::melon"
        )
list(APPEND TEST_LINKS ${CARBIN_SYS_DYLINK})

add_subdirectory(base)
add_subdirectory(bootstrap)
add_subdirectory(container)
add_subdirectory(debugging)
add_subdirectory(files)
add_subdirectory(hash)
add_subdirectory(log)
add_subdirectory(memory)
add_subdirectory(strings)
add_subdirectory(thread)
add_subdirectory(times)




