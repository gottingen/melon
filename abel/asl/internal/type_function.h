//
// Created by liyinbin on 2020/3/23.
//

#ifndef ABEL_ASL_INTERNAL_TYPE_FUNCTION_H_
#define ABEL_ASL_INTERNAL_TYPE_FUNCTION_H_

#include <abel/asl/internal/config.h>
#include <abel/asl//internal/type_common.h>
#include <abel/asl/internal/type_pod.h>
#include <abel/asl//internal/type_transformation.h>

namespace abel {

    // function_traits

    template<typename T>
    struct function_traits;

    template<typename Ret, typename... Args>
    struct function_traits<Ret(Args...)> {
        using return_type = Ret;
        using args_as_tuple = std::tuple<Args...>;
        using signature = Ret(Args...);

        static constexpr std::size_t arity = sizeof...(Args);

        template<std::size_t N>
        struct arg {
            static_assert(N < arity, "no such parameter index.");
            using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
        };
    };

    template<typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)> {
    };

    template<typename T, typename Ret, typename... Args>
    struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)> {
    };

    template<typename T, typename Ret, typename... Args>
    struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)> {
    };

    template<typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {
    };

    template<typename T>
    struct function_traits<T &> : public function_traits<typename std::remove_reference<T>::type> {
    };


    #if defined(_MSC_VER) || (defined(_LIBCPP_VERSION) && \
                              _LIBCPP_VERSION < 4000 && _LIBCPP_STD_VER > 11)
    #define ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_ 0
    #else
    #define ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_ 1
    #endif

#if !ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
    template <typename Key, typename = size_t>
        struct is_hashable : std::true_type {};
#else   // ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
    template<typename Key, typename = void>
    struct is_hashable : std::false_type {
    };

    template<typename Key>
    struct is_hashable<
            Key,
            abel::enable_if_t<std::is_convertible<
                    decltype(std::declval<std::hash<Key> &>()(std::declval<Key const &>())),
                    std::size_t>::value>> : std::true_type {
    };
#endif  // !ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_

    struct assert_hash_enabled_helper {
    private:
        static void sink(...) {}

        struct nat {
        };

        template<class Key>
        static auto get_return_type(int)
        -> decltype(std::declval<std::hash<Key>>()(std::declval<Key const &>()));

        template<class Key>
        static nat get_return_type(...);

        template<class Key>
        static std::nullptr_t do_it() {
            static_assert(is_hashable<Key>::value,
                          "std::hash<Key> does not provide a call operator");
            static_assert(
                    std::is_default_constructible<std::hash<Key>>::value,
                    "std::hash<Key> must be default constructible when it is enabled");
            static_assert(
                    std::is_copy_constructible<std::hash<Key>>::value,
                    "std::hash<Key> must be copy constructible when it is enabled");
            static_assert(abel::is_copy_assignable<std::hash<Key>>::value,
                          "std::hash<Key> must be copy assignable when it is enabled");
            // is_destructible is unchecked as it's implied by each of the
            // is_constructible checks.
            using ReturnType = decltype(get_return_type<Key>(0));
            static_assert(std::is_same<ReturnType, nat>::value ||
                          std::is_same<ReturnType, size_t>::value,
                          "std::hash<Key> must return size_t");
            return nullptr;
        }

        template<class... Ts>
        friend void assert_hash_enabled();
    };

    template<class... Ts>
    ABEL_FORCE_INLINE void assert_hash_enabled() {
        using Helper = assert_hash_enabled_helper;
        Helper::sink(Helper::do_it<Ts>()...);
    }


} //namespace abel

#endif //ABEL_ASL_INTERNAL_TYPE_FUNCTION_H_
