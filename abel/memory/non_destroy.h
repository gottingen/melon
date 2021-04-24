//
// Created by liyinbin on 2021/4/2.
//

#ifndef ABEL_MEMORY_NON_DESTROY_H_
#define ABEL_MEMORY_NON_DESTROY_H_

#include <new>
#include <type_traits>
#include <utility>

namespace abel {

    namespace memory_detail {

        template<class T>
        class non_destroy_impl {
        public:
            // Noncopyable / nonmovable.
            non_destroy_impl(const non_destroy_impl &) = delete;

            non_destroy_impl &operator=(const non_destroy_impl &) = delete;

            // Accessors.
            T *get() noexcept { return reinterpret_cast<T *>(&_storage); }

            const T *get() const noexcept {
                return reinterpret_cast<const T *>(&_storage);
            }

            T *operator->() noexcept { return get(); }

            const T *operator->() const noexcept { return get(); }

            T &operator*() noexcept { return *get(); }

            const T &operator*() const noexcept { return *get(); }

        protected:
            non_destroy_impl() = default;

        protected:
            std::aligned_storage_t<sizeof(T), alignof(T)> _storage;
        };

    }  // namespace memory_detail

    template<class T>
    class non_destroy final : private memory_detail::non_destroy_impl<T> {
        using Impl = memory_detail::non_destroy_impl<T>;

    public:
        template<class... Ts>
        explicit non_destroy(Ts &&... args) {
            new(&this->_storage) T(std::forward<Ts>(args)...);
        }

        using Impl::get;
        using Impl::operator->;
        using Impl::operator*;
    };

    template<class T>
    class non_destroyed_singleton final
            : private abel::memory_detail::non_destroy_impl<T> {
        using Impl = memory_detail::non_destroy_impl<T>;

    public:
        using Impl::get;
        using Impl::operator->;
        using Impl::operator*;

    private:
        friend T;

        template<class... Ts>
        explicit non_destroyed_singleton(Ts &&... args) {
            new(&this->_storage) T(std::forward<Ts>(args)...);
        }
    };


}  // namespace abel

#endif  // ABEL_MEMORY_NON_DESTROY_H_
