//

#ifndef ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
#define ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_

#include <cassert>
#include <functional>
#include <type_traits>
#include <abel/asl/functional/invoke.h>
#include <abel/asl/type_traits.h>

namespace abel {

    namespace functional_internal {

// Like a void* that can handle function pointers as well. The standard does not
// allow function pointers to round-trip through void*, but void(*)() is fine.
//
// Note: It's important that this class remains trivial and is the same size as
// a pointer, since this allows the compiler to perform tail-call optimizations
// when the underlying function is a callable object with a matching signature.
        union VoidPtr {
            const void *obj;

            void (*fun)();
        };

// Chooses the best type for passing T as an argument.
// Attempt to be close to SystemV AMD64 ABI. Objects with trivial copy ctor are
// passed by value.
        template<typename T>
        constexpr bool PassByValue() {
            return !std::is_lvalue_reference<T>::value &&
                   abel::is_trivially_copy_constructible<T>::value &&
                   abel::is_trivially_copy_assignable<
                           typename std::remove_cv<T>::type>::value &&
                   std::is_trivially_destructible<T>::value &&
                   sizeof(T) <= 2 * sizeof(void *);
        }

        template<typename T>
        struct ForwardT : std::conditional<PassByValue<T>(), T, T &&> {
        };

// An Invoker takes a pointer to the type-erased invokable object, followed by
// the arguments that the invokable object expects.
//
// Note: The order of arguments here is an optimization, since member functions
// have an implicit "this" pointer as their first argument, putting VoidPtr
// first allows the compiler to perform tail-call optimization in many cases.
        template<typename R, typename... Args>
        using Invoker = R (*)(VoidPtr, typename ForwardT<Args>::type...);

//
// InvokeObject and InvokeFunction provide static "Invoke" functions that can be
// used as Invokers for objects or functions respectively.
//
// static_cast<R> handles the case the return type is void.
        template<typename Obj, typename R, typename... Args>
        R InvokeObject(VoidPtr ptr, typename ForwardT<Args>::type... args) {
            auto o = static_cast<const Obj *>(ptr.obj);
            return static_cast<R>(
                    abel::base_internal::Invoke(*o, std::forward<Args>(args)...));
        }

        template<typename Fun, typename R, typename... Args>
        R InvokeFunction(VoidPtr ptr, typename ForwardT<Args>::type... args) {
            auto f = reinterpret_cast<Fun>(ptr.fun);
            return static_cast<R>(
                    abel::base_internal::Invoke(f, std::forward<Args>(args)...));
        }

        template<typename Sig>
        void AssertNonNull(const std::function<Sig> &f) {
            assert(f != nullptr);
            (void) f;
        }

        template<typename F>
        void AssertNonNull(const F &) {}

        template<typename F, typename C>
        void AssertNonNull(F C::*f) {
            assert(f != nullptr);
            (void) f;
        }

        template<bool C>
        using EnableIf = typename ::std::enable_if<C, int>::type;

    }  // namespace functional_internal


// FunctionRef
//
// Dummy class declaration to allow the partial specialization based on function
// types below.
    template<typename T>
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
    template<typename R, typename... Args>
    class FunctionRef<R(Args...)> {
    private:
        // Used to disable constructors for objects that are not compatible with the
        // signature of this FunctionRef.
        template<typename F,
                typename FR = abel::base_internal::InvokeT<F, Args &&...>>
        using EnableIfCompatible =
        typename std::enable_if<std::is_void<R>::value ||
                                std::is_convertible<FR, R>::value>::type;

    public:
        // Constructs a FunctionRef from any invokable type.
        template<typename F, typename = EnableIfCompatible<const F &>>
        FunctionRef(const F &f)  // NOLINT(runtime/explicit)
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
        template<
                typename F, typename = EnableIfCompatible<F *>,
                abel::functional_internal::EnableIf<abel::is_function<F>::value> = 0>
        FunctionRef(F *f)  // NOLINT(runtime/explicit)
                : invoker_(&abel::functional_internal::InvokeFunction<F *, R, Args...>) {
            assert(f != nullptr);
            ptr_.fun = reinterpret_cast<decltype(ptr_.fun)>(f);
        }

        // To help prevent subtle lifetime bugs, FunctionRef is not assignable.
        // Typically, it should only be used as an argument type.
        FunctionRef &operator=(const FunctionRef &rhs) = delete;

        // Call the underlying object.
        R operator()(Args... args) const {
            return invoker_(ptr_, std::forward<Args>(args)...);
        }

    private:
        abel::functional_internal::VoidPtr ptr_;
        abel::functional_internal::Invoker<R, Args...> invoker_;
    };


}  // namespace abel

#endif  // ABEL_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
