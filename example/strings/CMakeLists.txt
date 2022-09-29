file(GLOB META_SRC "*.cc")

foreach (fl ${META_SRC})

    string(REGEX REPLACE ".+/(.+)\\.cc$" "\\1" SRC_NAME ${fl})

    get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" DIR_NAME ${DIR_NAME})

    set(BINARY_NAME ${DIR_NAME}_${SRC_NAME})

    carbin_cc_binary(
            NAME ${BINARY_NAME}
            SOURCES ${fl}
            PUBLIC_LINKED_TARGETS melon::melon
            PRIVATE_COMPILE_OPTIONS ${CARBIN_DEFAULT_COPTS}
            VERBOSE
    )

endforeach (fl ${SRC})