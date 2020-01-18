
FILE(GLOB TESTING_SRC "*.cc")


add_library(testing_static STATIC ${TESTING_SRC})
target_link_libraries(testing_static gtest gmock abel_static pthread)