//
// Created by liyinbin on 2021/4/19.
//

#ifndef ABEL_IO_HANDLER_H_
#define ABEL_IO_HANDLER_H_


#include <unistd.h>

#include "abel/log/logging.h"

namespace abel {


    struct handler_deleter {
        void operator()(int fd) const noexcept {
            if (fd == 0 || fd == -1) {
                return;
            }
            DCHECK(::close(fd) == 0);
        }
    };

    template<class T, class Deleter, T... kInvalidHandles>
    class handler_base : private Deleter {
        // Anyone will do.
        static constexpr T kDefaultInvalid = (kInvalidHandles, ...);

    public:
        handler_base() = default;

        constexpr explicit handler_base(T handle) noexcept : handle_(handle) {}

        ~handler_base() { reset(); }

        // Movable.
        constexpr handler_base(handler_base &&h) noexcept {
            handle_ = h.handle_;
            h.handle_ = kDefaultInvalid;
        }

        constexpr handler_base &operator=(handler_base &&h) noexcept {
            handle_ = h.handle_;
            h.handle_ = kDefaultInvalid;
            return *this;
        }

        // Non-copyable.
        handler_base(const handler_base &) = delete;

        handler_base &operator=(const handler_base &) = delete;

        // Useful if handle is returned by argument:
        //
        // GetHandle(..., h.Retrieve());
        T *retrieve() noexcept { return &handle_; }

        // Return handle's value.
        constexpr T get() const noexcept { return handle_; }

        // Return if we're holding a valid handle value.
        constexpr explicit operator bool() const noexcept {
            // The "extra" parentheses is grammatically required.
            return ((handle_ != kInvalidHandles) && ...);
        }

        // Return handle's value, and give up ownership.
        constexpr T leak() noexcept {
            int rc = handle_;
            handle_ = kDefaultInvalid;
            return rc;
        }

        void reset(T new_value = kDefaultInvalid) noexcept {
            if (operator bool()) {
                Deleter::operator()(handle_);
            }
            handle_ = new_value;
        }

    private:
        T handle_ = kDefaultInvalid;
    };


    using handler = handler_base<int, handler_deleter, -1, 0>;

}  // namespace abel


#endif  // ABEL_IO_HANDLER_H_
