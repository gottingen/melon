//
// -----------------------------------------------------------------------------
// File: function_ref.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::FunctionRef` type for holding a
// non-owning reference to an object of any invocable type. This function
// reference is typically most useful as a type-erased argument type for
// accepting function types that neither take ownership nor copy the type; using
// the reference type in this case avoids a copy and an allocation. Best
// practices of other non-owning reference-like objects (such as
// `abel::string_view`) apply here.
//
//  An `abel::FunctionRef` is similar in usage to a `std::function` but has the
//  following differences:
//
//  * It doesn't own the underlying object.
//  * It doesn't have a null or empty state.
//  * It never performs deep copies or allocations.
//  * It's much faster and cheaper to construct.
//  * It's trivially copyable and destructable.
//
// Generally, `abel::FunctionRef` should not be used as a return value, data
// member, or to initialize a `std::function`. Such usages will often lead to
// problematic lifetime issues. Once you convert something to an
// `abel::FunctionRef` you cannot make a deep copy later.
//
// This class is suitable for use wherever a "const std::function<>&"
// would be used without making a copy. ForEach functions and other versions of
// the visitor pattern are a good example of when this class should be used.
//
// This class is trivial to copy and should be passed by value.
#ifndef ABEL_FUNCTIONAL_FUNCTION_REF_H_
#define ABEL_FUNCTIONAL_FUNCTION_REF_H_

#include <cassert>
#include <functional>
#include <type_traits>

#include <abel/functional/internal/function_ref.h>
#include <abel/meta/type_traits.h>

namespace abel {
ABEL_NAMESPACE_BEGIN

// FunctionRef
//
// Dummy class declaration to allow the partial specialization based on function
// types below.
template <typename T>
class FunctionRef;

// FunctionRef
//
// An `abel::FunctionRef` is a lightweight wrapper to any invokable object with
// a compatible signature. Generally, an `abel::FunctionRef` should only be used
// as an argument type and should be preferred as an argument over a const
// reference to a `std::function`.
//
// Example:
//
//   // The following function takes a function callback by const reference
//   bool Visitor(const std::function<void(my_proto&,
//                                         abel::string_view)>& callback);
//
//   // Assuming that the function is not stored or otherwise copied, it can be
//   // replaced by an `abel::FunctionRef`:
//   bool Visitor(abel::FunctionRef<void(my_proto&, abel::string_view)>
//                  callback);
//
// Note: the assignment operator within an `abel::FunctionRef` is intentionally
// deleted to prevent misuse; because the `abel::FunctionRef` does not own the
// underlying type, assignment likely indicates misuse.
template <typename R, typename... Args>
class FunctionRef<R(Args...)> {
 private:
  // Used to disable constructors for objects that are not compatible with the
  // signature of this FunctionRef.
  template <typename F,
            typename FR = abel::base_internal::InvokeT<F, Args&&...>>
  using EnableIfCompatible =
      typename std::enable_if<std::is_void<R>::value ||
                              std::is_convertible<FR, R>::value>::type;

 public:
  // Constructs a FunctionRef from any invokable type.
  template <typename F, typename = EnableIfCompatible<const F&>>
  FunctionRef(const F& f)  // NOLINT(runtime/explicit)
      : invoker_(&abel::functional_internal::InvokeObject<F, R, Args...>) {
    abel::functional_internal::AssertNonNull(f);
    ptr_.obj = &f;
  }

  // Overload for function pointers. This eliminates a level of indirection that
  // would happen if the above overload was used (it lets us store the pointer
  // instead of a pointer to a pointer).
  //
  // This overload is also used for references to functions, since references to
  // functions can decay to function pointers implicitly.
  template <
      typename F, typename = EnableIfCompatible<F*>,
      abel::functional_internal::EnableIf<abel::is_function<F>::value> = 0>
  FunctionRef(F* f)  // NOLINT(runtime/explicit)
      : invoker_(&abel::functional_internal::InvokeFunction<F*, R, Args...>) {
    assert(f != nullptr);
    ptr_.fun = reinterpret_cast<decltype(ptr_.fun)>(f);
  }

  // To help prevent subtle lifetime bugs, FunctionRef is not assignable.
  // Typically, it should only be used as an argument type.
  FunctionRef& operator=(const FunctionRef& rhs) = delete;

  // Call the underlying object.
  R operator()(Args... args) const {
    return invoker_(ptr_, std::forward<Args>(args)...);
  }

 private:
  abel::functional_internal::VoidPtr ptr_;
  abel::functional_internal::Invoker<R, Args...> invoker_;
};

ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_FUNCTIONAL_FUNCTION_REF_H_
