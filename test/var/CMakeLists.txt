#
# Copyright 2024 The Carbin Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

file(GLOB TEST_VAR_SRCS "var_*_unittest.cc")

carbin_cc_test(
        NAME var_test
        MODULE var
        SOURCES ${TEST_VAR_SRCS}
        DEFINES ${MELON_TEST_DEFINES}
        LINKS melon::var_test_lib ${MELON_DEPS_LINK} ${GPERFTOOLS_LIBRARIES} ${GTEST_LIB} ${GTEST_MAIN_LIB}
        CXXOPTS ${MELON_TEST_CXX_OPTIONS}
)
