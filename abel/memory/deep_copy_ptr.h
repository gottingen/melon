// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_MEMORY_DEEP_COPY_PTR_H_
#define ABEL_MEMORY_DEEP_COPY_PTR_H_

namespace abel {

    template<typename T>
    class deep_copy_ptr {
    public:
        deep_copy_ptr() : _ptr(NULL) {}

        explicit deep_copy_ptr(T *obj) : _ptr(obj) {}

        ~deep_copy_ptr() {
            delete _ptr;
        }

        deep_copy_ptr(const deep_copy_ptr &rhs)
                : _ptr(rhs._ptr ? new T(*rhs._ptr) : NULL) {}

        void operator=(const deep_copy_ptr &rhs) {
            if (rhs._ptr) {
                if (_ptr) {
                    *_ptr = *rhs._ptr;
                } else {
                    _ptr = new T(*rhs._ptr);
                }
            } else {
                delete _ptr;
                _ptr = NULL;
            }
        }

        T *get() const { return _ptr; }

        void reset(T *ptr) {
            delete _ptr;
            _ptr = ptr;
        }

        operator void *() const { return _ptr; }

    private:
        T *_ptr;
    };


    namespace memory_internal {

        template<class T, class Tag = void>
        struct early_init_instance {
            inline static non_destroy <T> object;
        };

    }  // namespace detail

    template<class T, class Tag = void>
    const T &early_init_constant() {
        return *memory_internal::early_init_instance<T, Tag>::object.get();
    }
}  // namespace abel
#endif  // ABEL_MEMORY_DEEP_COPY_PTR_H_
