
#ifndef ABEL_MEMORY_TLS_SLAB_H_
#define ABEL_MEMORY_TLS_SLAB_H_

#include <cstddef>                       // size_t

namespace abel {

    template<typename T>
    struct tls_slab_block_max_size {
        static const size_t value = 64 * 1024; // bytes
    };
    template<typename T>
    struct tls_slab_block_max_item {
        static const size_t value = 256;
    };

    template<typename T>
    struct tls_slab_block_max_free_chunk {
        static size_t value() { return 256; }
    };

    template<typename T>
    struct tls_slab_validator {
        static bool validate(const T *) { return true; }
    };

}  // namespace abel

#include <abel/memory/tls_slab_inl.h>

namespace abel {

    template<typename T>
    inline T *get_resource(item_id<T> *id) {
        return tls_slab<T>::singleton()->get_resource(id);
    }

    template<typename T, typename A1>
    inline T *get_resource(item_id<T> *id, const A1 &arg1) {
        return tls_slab<T>::singleton()->get_resource(id, arg1);
    }

    template<typename T, typename A1, typename A2>
    inline T *get_resource(item_id<T> *id, const A1 &arg1, const A2 &arg2) {
        return tls_slab<T>::singleton()->get_resource(id, arg1, arg2);
    }


    template<typename T>
    inline int return_resource(item_id<T> id) {
        return tls_slab<T>::singleton()->return_resource(id);
    }

    template<typename T>
    inline T *address_resource(item_id<T> id) {
        return tls_slab<T>::address_resource(id);
    }

    template<typename T>
    inline void clear_resources() {
        tls_slab<T>::singleton()->clear_resources();
    }

    template<typename T>
    tls_slab_info describe_resources() {
        return tls_slab<T>::singleton()->describe_resources();
    }

}  // namespace abel

#endif  //ABEL_MEMORY_TLS_SLAB_H_

