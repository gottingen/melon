//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_ERASED_PTR_H_
#define ABEL_MEMORY_ERASED_PTR_H_

#include <utility>

#include "abel/base/profile.h"

namespace abel {


    
    class erased_ptr final {
    public:
        using Deleter = void (*)(void *);

        // A default constructed one is an empty one.
        /* implicit */ constexpr erased_ptr(std::nullptr_t = nullptr)
                : ptr_(nullptr), deleter_(nullptr) {}

        // Ownership taken.
        template<class T>
        constexpr explicit erased_ptr(T *ptr) noexcept
                : ptr_(ptr), deleter_([](void *ptr) { delete static_cast<T *>(ptr); }) {}

        template<class T, class D>
        constexpr erased_ptr(T *ptr, D deleter) noexcept
                : ptr_(ptr), deleter_(deleter) {}

        // Movable
        constexpr erased_ptr(erased_ptr &&ptr) noexcept
                : ptr_(ptr.ptr_), deleter_(ptr.deleter_) {
            ptr.ptr_ = nullptr;
        }

        erased_ptr &operator=(erased_ptr &&ptr) noexcept {
            if (ABEL_LIKELY(&ptr != this)) {
                reset();
            }
            std::swap(ptr_, ptr.ptr_);
            std::swap(deleter_, ptr.deleter_);
            return *this;
        }

        // But not copyable.
        erased_ptr(const erased_ptr &) = delete;

        erased_ptr &operator=(const erased_ptr &) = delete;

        // Any resource we're holding is freed in dtor.
        ~erased_ptr() {
            if (ptr_) {
                deleter_(ptr_);
            }
        }

        // Accessor.
        constexpr void *get() const noexcept { return ptr_; }

        // It's your responsibility to check if the type match.
        template<class T>
        T *unchecked_get() const noexcept {
            return reinterpret_cast<T *>(get());
        }

        // Test if this object holds a valid pointer.
        constexpr explicit operator bool() const noexcept { return !!ptr_; }

        // Free any resource this class holds and reset its internal pointer to
        // `nullptr`.
        constexpr void reset(std::nullptr_t = nullptr) noexcept {
            if (ptr_) {
                deleter_(ptr_);
                ptr_ = nullptr;
            }
        }

        // Release ownership of its internal object.
        [[nodiscard]] void *leak() noexcept { return std::exchange(ptr_, nullptr); }

        // This is the only way you can destroy the pointer you obtain from `Leak()`.
        constexpr Deleter get_deleter() const noexcept { return deleter_; }

    private:
        void *ptr_;
        Deleter deleter_;
    };

    template<class T, class... Args>
    erased_ptr make_erased(Args &&... args) {
        return erased_ptr(new T(std::forward<Args>(args)...));
    }
    
}  // namespace abel

#endif  // ABEL_MEMORY_ERASED_PTR_H_
