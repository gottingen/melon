//

#ifndef ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
#define ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_

#include <cassert>
#include <functional>
#include <type_traits>

#include <abel/functional/internal/invoke.h>
#include <abel/meta/type_traits.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace functional_internal {

// Like a void* that can handle function pointers as well. The standard does not
// allow function pointers to round-trip through void*, but void(*)() is fine.
//
// Note: It's important that this class remains trivial and is the same size as
// a pointer, since this allows the compiler to perform tail-call optimizations
// when the underlying function is a callable object with a matching signature.
union VoidPtr {
  const void* obj;
  void (*fun)();
};

// Chooses the best type for passing T as an argument.
// Attempt to be close to SystemV AMD64 ABI. Objects with trivial copy ctor are
// passed by value.
template <typename T>
constexpr bool PassByValue() {
  return !std::is_lvalue_reference<T>::value &&
         abel::is_trivially_copy_constructible<T>::value &&
         abel::is_trivially_copy_assignable<
             typename std::remove_cv<T>::type>::value &&
         std::is_trivially_destructible<T>::value &&
         sizeof(T) <= 2 * sizeof(void*);
}

template <typename T>
struct ForwardT : std::conditional<PassByValue<T>(), T, T&&> {};

// An Invoker takes a pointer to the type-erased invokable object, followed by
// the arguments that the invokable object expects.
//
// Note: The order of arguments here is an optimization, since member functions
// have an implicit "this" pointer as their first argument, putting VoidPtr
// first allows the compiler to perform tail-call optimization in many cases.
template <typename R, typename... Args>
using Invoker = R (*)(VoidPtr, typename ForwardT<Args>::type...);

//
// InvokeObject and InvokeFunction provide static "Invoke" functions that can be
// used as Invokers for objects or functions respectively.
//
// static_cast<R> handles the case the return type is void.
template <typename Obj, typename R, typename... Args>
R InvokeObject(VoidPtr ptr, typename ForwardT<Args>::type... args) {
  auto o = static_cast<const Obj*>(ptr.obj);
  return static_cast<R>(
      abel::base_internal::Invoke(*o, std::forward<Args>(args)...));
}

template <typename Fun, typename R, typename... Args>
R InvokeFunction(VoidPtr ptr, typename ForwardT<Args>::type... args) {
  auto f = reinterpret_cast<Fun>(ptr.fun);
  return static_cast<R>(
      abel::base_internal::Invoke(f, std::forward<Args>(args)...));
}

template <typename Sig>
void AssertNonNull(const std::function<Sig>& f) {
  assert(f != nullptr);
  (void)f;
}

template <typename F>
void AssertNonNull(const F&) {}

template <typename F, typename C>
void AssertNonNull(F C::*f) {
  assert(f != nullptr);
  (void)f;
}

template <bool C>
using EnableIf = typename ::std::enable_if<C, int>::type;

}  // namespace functional_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
