#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.15)
project(restful C CXX)

# for development
list(APPEND CMAKE_PREFIX_PATH "../../output")
# EA may always install in /opt/EA/inf
list(APPEND CMAKE_PREFIX_PATH "/opt/EA/inf")

find_package(melon REQUIRED)
# MELON_DEPS_LIBS is a list of all the libraries that melon depends on
# MELON_DEPS_INCLUDE_DIRS is a list of all the include directories that melon depends on
# MELON_DEPS_LIBS and MELON_DEPS_INCLUDE_DIRS are set by the find_package(melon) command

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(vue_server vue_server.cc)

target_link_libraries(vue_server ${MELON_STATIC_LIBRARIES})
target_include_directories(vue_server PRIVATE ${MELON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${PROJECT_SOURCE_DIR}/key.pem
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/cert.pem
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
