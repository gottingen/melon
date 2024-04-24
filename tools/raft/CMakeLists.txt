
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/output/bin)

carbin_cc_binary(
        NAMESPACE melon
        NAME raft_cli
        SOURCES raft_cli.cc
        DEPS melon::melon
        CXXOPTS ${MELON_CXX_OPTIONS}
        DEFINES ${MELON_CXX_DEFINES}
        LINKS melon::melon ${MELON_DEPS_LINK}
        PUBLIC
)