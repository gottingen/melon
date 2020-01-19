
set(ABEL_GNUC_CXX_FLAGS
        -DCHECK_PTHREAD_RETURN_VALUE
        -D_FILE_OFFSET_BITS=64
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Woverloaded-virtual
        -Wpointer-arith
        -Wwrite-strings
        -march=native
        -MMD
        -fPIC
        -std=c++11
        )