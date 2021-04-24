//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_FIBER_LOCAL_H_
#define ABEL_FIBER_FIBER_LOCAL_H_


#include "abel/fiber/internal/index_alloc.h"
#include "abel/fiber/internal/fiber_entity.h"

namespace abel {

    namespace fiber_internal {

        struct FiberLocalIndexTag;
        struct TrivialFiberLocalIndexTag;

    }  // namespace fiber_internal

    // `T` needs to be `DefaultConstructible`.
    //
    // You should normally use this class as static / member variable. In case of
    // variable in stack, just use automatic variable (stack variable) instead.
    template <class T>
    class fiber_local {
        // @sa: Comments in `fiber_entity` for definition of "trivial" here.
        inline static constexpr auto is_using_trivial_fls_v =
                std::is_trivial_v<T> &&
                sizeof(T) <= sizeof(fiber_internal::fiber_entity::trivial_fls_t);

    public:
        // A dedicated FLS slot is allocated for this `fiber_local`.
        fiber_local() : slot_index_(GetIndexAlloc()->next()) {}

        // The FLS slot is released on destruction.
        ~fiber_local() { GetIndexAlloc()->free(slot_index_); }

        // Accessor.
        T* operator->() const noexcept { return get(); }
        T& operator*() const noexcept { return *get(); }
        T* get() const noexcept { return get_inpl(); }

    private:
        T* get_inpl() const noexcept {
            auto current_fiber = fiber_internal::get_current_fiber_entity();
            if constexpr (is_using_trivial_fls_v) {
                return reinterpret_cast<T*>(current_fiber->get_trivial_fls(slot_index_));
            } else {
                auto ptr = current_fiber->get_fls(slot_index_);
                if (!*ptr) {
                    *ptr = make_erased<T>();
                }
                return static_cast<T*>(ptr->get());
            }
        }

        static index_alloc* GetIndexAlloc() {
            if constexpr (is_using_trivial_fls_v) {
                return index_alloc::For<
                        fiber_internal::TrivialFiberLocalIndexTag>();
            } else {
                return index_alloc::For<fiber_internal::FiberLocalIndexTag>();
            }
        }

    private:
        std::size_t slot_index_;
    };

}  // namespace abel


#endif  // ABEL_FIBER_FIBER_LOCAL_H_
