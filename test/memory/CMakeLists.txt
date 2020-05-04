
file(GLOB SRC "*.cc")

foreach (fl ${SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" TEST_NAME ${fl})
    get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" DIR_NAME ${DIR_NAME})

    set(EXE_NAME ${DIR_NAME}_${TEST_NAME})
    carbin_cc_test(
            NAME ${EXE_NAME}
            SOURCES ${fl}
            PUBLIC_LINKED_TARGETS
            ${TEST_LINKS}
            PRIVATE_COMPILE_OPTIONS ${CARBIN_TEST_COPTS}
            VERBOSE
    )
endforeach (fl ${SRC})