//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_ATOMIC_COPYABLE_ATOMIC_H_
#define ABEL_ATOMIC_COPYABLE_ATOMIC_H_


#include <atomic>

namespace abel {

    // Make `std::atomic<T>` copyable.
    template<class T>
    class copyable_atomic : public std::atomic<T> {
    public:
        copyable_atomic() = default;

        /* implicit */ copyable_atomic(T value) : std::atomic<T>(std::move(value)) {}

        constexpr copyable_atomic(const copyable_atomic &from)
                : std::atomic<T>(from.load()) {}

        constexpr copyable_atomic &operator=(const copyable_atomic &from) {
            store(from.load());
            return *this;
        }
    };

}  // namespace abel

#endif  // ABEL_ATOMIC_COPYABLE_ATOMIC_H_
