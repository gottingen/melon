// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#ifndef ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
#define ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_

#include <cassert>
#include <functional>
#include <type_traits>
#include "abel/functional/invoke.h"
#include "abel/meta/type_traits.h"

namespace abel {

namespace functional_internal {

// Like a void* that can handle function pointers as well. The standard does not
// allow function pointers to round-trip through void*, but void(*)() is fine.
//
// Note: It's important that this class remains trivial and is the same size as
// a pointer, since this allows the compiler to perform tail-call optimizations
// when the underlying function is a callable object with a matching signature.
union void_ptr {
    const void *obj;

    void (*fun)();
};

// Chooses the best type for passing T as an argument.
// Attempt to be close to SystemV AMD64 ABI. Objects with trivial copy ctor are
// passed by value.
template<typename T>
constexpr bool pass_by_value() {
    return !std::is_lvalue_reference<T>::value &&
           abel::is_trivially_copy_constructible<T>::value &&
           abel::is_trivially_copy_assignable<
                   typename std::remove_cv<T>::type>::value &&
           std::is_trivially_destructible<T>::value &&
           sizeof(T) <= 2 * sizeof(void *);
}

template<typename T>
struct forward_type : std::conditional<pass_by_value<T>(), T, T &&> {
};

// An invoker takes a pointer to the type-erased invokable object, followed by
// the arguments that the invokable object expects.
//
// Note: The order of arguments here is an optimization, since member functions
// have an implicit "this" pointer as their first argument, putting void_ptr
// first allows the compiler to perform tail-call optimization in many cases.
template<typename R, typename... Args>
using invoker = R (*)(void_ptr, typename forward_type<Args>::type...);

//
// invoke_object and invoke_function provide static "Invoke" functions that can be
// used as Invokers for objects or functions respectively.
//
// static_cast<R> handles the case the return type is void.
template<typename Obj, typename R, typename... Args>
R invoke_object(void_ptr ptr, typename forward_type<Args>::type... args) {
    auto o = static_cast<const Obj *>(ptr.obj);
    return static_cast<R>(
            abel::base_internal::Invoke(*o, std::forward<Args>(args)...));
}

template<typename Fun, typename R, typename... Args>
R invoke_function(void_ptr ptr, typename forward_type<Args>::type... args) {
    auto f = reinterpret_cast<Fun>(ptr.fun);
    return static_cast<R>(
            abel::base_internal::Invoke(f, std::forward<Args>(args)...));
}

template<typename Sig>
void assert_non_null(const std::function<Sig> &f) {
    assert(f != nullptr);
    (void) f;
}

template<typename F>
void assert_non_null(const F &) {}

template<typename F, typename C>
void assert_non_null(F C::*f) {
    assert(f != nullptr);
    (void) f;
}

template<bool C>
using enable_if_typr = typename ::std::enable_if<C, int>::type;

}  // namespace functional_internal


// function_ref
//
// Dummy class declaration to allow the partial specialization based on function
// types below.
template<typename T>
class function_ref;

// function_ref
//
// An `abel::function_ref` is a lightweight wrapper to any invokable object with
// a compatible signature. Generally, an `abel::function_ref` should only be used
// as an argument type and should be preferred as an argument over a const
// reference to a `std::function`.
//
// Example:
//
//   // The following function takes a function callback by const reference
//   bool Visitor(const std::function<void(my_proto&,
//                                         std::string_view)>& callback);
//
//   // Assuming that the function is not stored or otherwise copied, it can be
//   // replaced by an `abel::function_ref`:
//   bool Visitor(abel::function_ref<void(my_proto&, std::string_view)>
//                  callback);
//
// Note: the assignment operator within an `abel::function_ref` is intentionally
// deleted to prevent misuse; because the `abel::function_ref` does not own the
// underlying type, assignment likely indicates misuse.
template<typename R, typename... Args>
class function_ref<R(Args...)> {
  private:
    // Used to disable constructors for objects that are not compatible with the
    // signature of this function_ref.
    template<typename F,
            typename FR = abel::base_internal::InvokeT<F, Args &&...>>
    using enable_if_compatible =
    typename std::enable_if<std::is_void<R>::value ||
                            std::is_convertible<FR, R>::value>::type;

  public:
    // Constructs a function_ref from any invokable type.
    template<typename F, typename = enable_if_compatible<const F &>>
    function_ref(const F &f)  // NOLINT(runtime/explicit)
            : invoker_(&abel::functional_internal::invoke_object<F, R, Args...>) {
        abel::functional_internal::assert_non_null(f);
        ptr_.obj = &f;
    }

    // Overload for function pointers. This eliminates a level of indirection that
    // would happen if the above overload was used (it lets us store the pointer
    // instead of a pointer to a pointer).
    //
    // This overload is also used for references to functions, since references to
    // functions can decay to function pointers implicitly.
    template<
            typename F, typename = enable_if_compatible<F *>,
            abel::functional_internal::enable_if_typr<abel::is_function<F>::value> = 0>
    function_ref(F *f)  // NOLINT(runtime/explicit)
            : invoker_(&abel::functional_internal::invoke_function<F *, R, Args...>) {
        assert(f != nullptr);
        ptr_.fun = reinterpret_cast<decltype(ptr_.fun)>(f);
    }

    // To help prevent subtle lifetime bugs, function_ref is not assignable.
    // Typically, it should only be used as an argument type.
    function_ref &operator=(const function_ref &rhs) = delete;

    // Call the underlying object.
    R operator()(Args... args) const {
        return invoker_(ptr_, std::forward<Args>(args)...);
    }

  private:
    abel::functional_internal::void_ptr ptr_;
    abel::functional_internal::invoker<R, Args...> invoker_;
};


}  // namespace abel

#endif  // ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
