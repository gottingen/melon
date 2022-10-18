# CMake generated Testfile for 
# Source directory: /Users/liyinbin/github/gottingen/melon/test
# Build directory: /Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_io "/Users/liyinbin/github/gottingen/melon/-DCMAKE_PREFIX_PATH=~/miniconda3/envs/melon-dev/test/test_io")
set_tests_properties(test_io PROPERTIES  _BACKTRACE_TRIPLES "/Users/liyinbin/github/gottingen/melon/test/CMakeLists.txt;115;add_test;/Users/liyinbin/github/gottingen/melon/test/CMakeLists.txt;0;")
subdirs("metrics")
subdirs("fiber")
subdirs("rpc")
subdirs("base")
subdirs("bootstrap")
subdirs("container")
subdirs("files")
subdirs("future")
subdirs("hash")
subdirs("log")
subdirs("memory")
subdirs("strings")
subdirs("io")
subdirs("debugging")
subdirs("times")
subdirs("thread")
subdirs("dag")
subdirs("rapidjson")
