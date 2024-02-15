

# test_var
# -DMELON_VAR_NOT_LINK_DEFAULT_VARIABLES not work for gcc >= 5.0, just remove the file to prevent linking into unit tests
list(REMOVE_ITEM VAR_SOURCES ${PROJECT_SOURCE_DIR}/melon/var/default_variables.cc)

add_library(VAR_DEBUG_LIB OBJECT ${VAR_SOURCES})
file(GLOB TEST_VAR_SRCS "var_*_unittest.cc")
add_executable(test_var ${TEST_VAR_SRCS}
        $<TARGET_OBJECTS:VAR_DEBUG_LIB>
        $<TARGET_OBJECTS:MUTIL_DEBUG_LIB>)
target_link_libraries(test_var gtest
        ${GPERFTOOLS_LIBRARIES}
        ${DYNAMIC_LIB})
add_test(NAME test_var COMMAND test_var)
