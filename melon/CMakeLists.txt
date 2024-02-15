# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

if(NOT DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/melon)

add_library(MUTIL_LIB OBJECT ${UTILITY_SOURCES})
add_library(SOURCES_LIB OBJECT ${SOURCES})
add_dependencies(SOURCES_LIB PROTO_LIB)

# shared library needs POSITION_INDEPENDENT_CODE
set_property(TARGET ${SOURCES_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${MUTIL_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(melon-static STATIC $<TARGET_OBJECTS:MUTIL_LIB>
                               $<TARGET_OBJECTS:SOURCES_LIB>
                               $<TARGET_OBJECTS:PROTO_LIB>)


SET_TARGET_PROPERTIES(melon-static PROPERTIES OUTPUT_NAME melon CLEAN_DIRECT_OUTPUT 1)

# for protoc-gen-mcpack
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/output/bin)
    
set(protoc_gen_mcpack_SOURCES
    ${PROJECT_SOURCE_DIR}/melon/mcpack2pb/generator.cpp
 )
    
add_executable(protoc-gen-mcpack ${protoc_gen_mcpack_SOURCES})

if(BUILD_SHARED_LIBS)
    add_library(melon-shared SHARED $<TARGET_OBJECTS:MUTIL_LIB>
                                   $<TARGET_OBJECTS:SOURCES_LIB>
                                   $<TARGET_OBJECTS:PROTO_LIB>)
    target_link_libraries(melon-shared ${DYNAMIC_LIB})
    if(WITH_GLOG)
        target_link_libraries(melon-shared ${GLOG_LIB})
    endif()
    SET_TARGET_PROPERTIES(melon-shared PROPERTIES OUTPUT_NAME melon CLEAN_DIRECT_OUTPUT 1)

    target_link_libraries(protoc-gen-mcpack melon-shared ${DYNAMIC_LIB} pthread)

    install(TARGETS melon-shared
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
else()
    target_link_libraries(protoc-gen-mcpack melon-static ${DYNAMIC_LIB} pthread)
endif()



install(TARGETS melon-static
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
