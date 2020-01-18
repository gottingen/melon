message(STATUS "compiler is ${CMAKE_COMPILER_IS_GNUCXX}")

set(CXX_FLAGS
        -DCHECK_PTHREAD_RETURN_VALUE
        -D_FILE_OFFSET_BITS=64
        -Wall
        -Wextra
        #-Werror
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Woverloaded-virtual
        -Wpointer-arith
        -Wwrite-strings
        -march=native
        -MMD
        -fPIC
        )
include(${CMAKE_ROOT}/Modules/CheckCXXCompilerFlag.cmake)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

if (COMPILER_SUPPORTS_CXX14)
    set(CXX_FLAGS "${CXX_FLAGS} -std=c++14")
elseif (COMPILER_SUPPORTS_CXX11)
    set(CXX_FLAGS "${CXX_FLAGS} -std=c++11")
else ()
    set(CXX_FLAGS "${CXX_FLAGS} -std=c++0x")
endif ()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")