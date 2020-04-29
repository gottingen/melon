//
// Created by liyinbin on 2020/4/29.
//

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
}
#endif //ABEL_MEMORY_DEEP_COPY_PTR_H_
