#
# Copyright 2023 The titan-search Authors.
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

carbin_cc_binary(
        NAMESPACE melon
        NAME trackme_server
        SOURCES trackme_server.cc
        DEPS melon::melon
        CXXOPTS ${MELON_CXX_OPTIONS}
        DEFINES ${MELON_CXX_DEFINES}
        INCLUDES  ${CMAKE_CURRENT_BINARY_DIR}
        LINKS melon::melon ${MELON_DEPS_LINK}
        PUBLIC
)