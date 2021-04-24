//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_TYPE_DESCRIPTOR_H_
#define ABEL_MEMORY_TYPE_DESCRIPTOR_H_

#include <type_traits>
#include <utility>

#include "abel/base/type_index.h"

namespace abel {

    template<class T>
    struct pool_traits;

    namespace memory_internal {

        template<class F>
        constexpr auto is_valid(F &&f) {
            return [f = std::forward<F>(f)](auto &&... args) constexpr {
                // FIXME: Perfect forwarding.
                return std::is_invocable_v<F, decltype(args)...>;
            };
        }

// `x` should be used as placeholder's name in` expr`.
#define ABEL_INTERNAL_IS_VALID(expr) \
  ::abel::memory_internal::is_valid([](auto&& x) -> decltype(expr) {})


        struct type_descriptor {
            abel::type_index type;

            void *(*create)();

            void (*destroy)(void *);

        };


#define ABEL_DEFINE_OBJECT_POOL_HOOK_IMPL(HookName)                       \
  template <class T>                                                       \
  inline void HookName##Hook([[maybe_unused]] void* p) {                   \
    if constexpr (ABEL_INTERNAL_IS_VALID(&x.HookName)(pool_traits<T>{})) { \
      return pool_traits<T>::HookName(reinterpret_cast<T*>(p));             \
    }                                                                      \
  }

        ABEL_DEFINE_OBJECT_POOL_HOOK_IMPL(OnGet)

        ABEL_DEFINE_OBJECT_POOL_HOOK_IMPL(OnPut)

#undef ABEL_DEFINE_OBJECT_POOL_HOOK_IMPL

        template<class T>
        void *create_object() {
            if constexpr (ABEL_INTERNAL_IS_VALID(&x.create)(pool_traits<T>{})) {
                return pool_traits<T>::create();
            } else {
                return new T();
            }
        }


        template<class T>
        void destroy_object(void *ptr) {
            auto p = static_cast<T *>(ptr);
            if constexpr (ABEL_INTERNAL_IS_VALID(&x.destroy)(pool_traits<T>{})) {
                pool_traits<T>::destroy(p);
            } else {
                delete p;
            }
        }

        template<class T>
        const type_descriptor *get_type_desc() {
            // `constexpr` doesn't work here.
            //
            // Possibly related: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81933
            static const type_descriptor desc = {.type = get_type_index<T>(),
                    .create = create_object<T>,
                    .destroy = destroy_object < T >};
            return &desc;
        }

    }  // namespace memory_internal
}  // namespace abel

#endif  // ABEL_MEMORY_TYPE_DESCRIPTOR_H_
