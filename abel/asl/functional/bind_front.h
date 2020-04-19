//
// -----------------------------------------------------------------------------
// File: bind_front.h
// -----------------------------------------------------------------------------
//
// `abel::bind_front()` returns a functor by binding a number of arguments to
// the front of a provided (usually more generic) functor. Unlike `std::bind`,
// it does not require the use of argument placeholders. The simpler syntax of
// `abel::bind_front()` allows you to avoid known misuses with `std::bind()`.
//
// `abel::bind_front()` is meant as a drop-in replacement for C++20's upcoming
// `std::bind_front()`, which similarly resolves these issues with
// `std::bind()`. Both `bind_front()` alternatives, unlike `std::bind()`, allow
// partial function application. (See
// https://en.wikipedia.org/wiki/Partial_application).

#ifndef ABEL_FUNCTIONAL_BIND_FRONT_H_
#define ABEL_FUNCTIONAL_BIND_FRONT_H_

#include <abel/asl/functional/front_binder.h>
#include <abel/asl/utility.h>

namespace abel {


// bind_front()
//
// Binds the first N arguments of an invocable object and stores them by value,
// except types of `std::reference_wrapper` which are 'unwound' and stored by
// reference.
//
// Like `std::bind()`, `abel::bind_front()` is implicitly convertible to
// `std::function`.  In particular, it may be used as a simpler replacement for
// `std::bind()` in most cases, as it does not require  placeholders to be
// specified. More importantly, it provides more reliable correctness guarantees
// than `std::bind()`; while `std::bind()` will silently ignore passing more
// parameters than expected, for example, `abel::bind_front()` will report such
// mis-uses as errors.
//
// abel::bind_front(a...) can be seen as storing the results of
// std::make_tuple(a...).
//
// Example: Binding a free function.
//
//   int Minus(int a, int b) { return a - b; }
//
//   assert(abel::bind_front(Minus)(3, 2) == 3 - 2);
//   assert(abel::bind_front(Minus, 3)(2) == 3 - 2);
//   assert(abel::bind_front(Minus, 3, 2)() == 3 - 2);
//
// Example: Binding a member function.
//
//   struct Math {
//     int Double(int a) const { return 2 * a; }
//   };
//
//   Math math;
//
//   assert(abel::bind_front(&Math::Double)(&math, 3) == 2 * 3);
//   // Stores a pointer to math inside the functor.
//   assert(abel::bind_front(&Math::Double, &math)(3) == 2 * 3);
//   // Stores a copy of math inside the functor.
//   assert(abel::bind_front(&Math::Double, math)(3) == 2 * 3);
//   // Stores std::unique_ptr<Math> inside the functor.
//   assert(abel::bind_front(&Math::Double,
//                           std::unique_ptr<Math>(new Math))(3) == 2 * 3);
//
// Example: Using `abel::bind_front()`, instead of `std::bind()`, with
//          `std::function`.
//
//   class FileReader {
//    public:
//     void ReadFileAsync(const std::string& filename, std::string* content,
//                        const std::function<void()>& done) {
//       // Calls Executor::schedule(std::function<void()>).
//       Executor::DefaultExecutor()->schedule(
//           abel::bind_front(&FileReader::BlockingRead, this,
//                            filename, content, done));
//     }
//
//    private:
//     void BlockingRead(const std::string& filename, std::string* content,
//                       const std::function<void()>& done) {
//       CHECK_OK(file::GetContents(filename, content, {}));
//       done();
//     }
//   };
//
// `abel::bind_front()` stores bound arguments explicitly using the type passed
// rather than implicitly based on the type accepted by its functor.
//
// Example: Binding arguments explicitly.
//
//   void LogStringView(abel::string_view sv) {
//     LOG(INFO) << sv;
//   }
//
//   Executor* e = Executor::DefaultExecutor();
//   std::string s = "hello";
//   abel::string_view sv = s;
//
//   // abel::bind_front(LogStringView, arg) makes a copy of arg and stores it.
//   e->schedule(abel::bind_front(LogStringView, sv)); // ERROR: dangling
//                                                     // string_view.
//
//   e->schedule(abel::bind_front(LogStringView, s));  // OK: stores a copy of
//                                                     // s.
//
// To store some of the arguments passed to `abel::bind_front()` by reference,
//  use std::ref()` and `std::cref()`.
//
// Example: Storing some of the bound arguments by reference.
//
//   class Service {
//    public:
//     void Serve(const Request& req, std::function<void()>* done) {
//       // The request protocol buffer won't be deleted until done is called.
//       // It's safe to store a reference to it inside the functor.
//       Executor::DefaultExecutor()->schedule(
//           abel::bind_front(&Service::BlockingServe, this, std::cref(req),
//           done));
//     }
//
//    private:
//     void BlockingServe(const Request& req, std::function<void()>* done);
//   };
//
// Example: Storing bound arguments by reference.
//
//   void Print(const string& a, const string& b) { LOG(INFO) << a << b; }
//
//   std::string hi = "Hello, ";
//   std::vector<std::string> names = {"Chuk", "Gek"};
//   // Doesn't copy hi.
//   for_each(names.begin(), names.end(),
//            abel::bind_front(Print, std::ref(hi)));
//
//   // DO NOT DO THIS: the functor may outlive "hi", resulting in
//   // dangling references.
//   foo->DoInFuture(abel::bind_front(Print, std::ref(hi), "Guest"));  // BAD!
//   auto f = abel::bind_front(Print, std::ref(hi), "Guest"); // BAD!
    template<class F, class... BoundArgs>
    constexpr functional_internal::bind_front_t<F, BoundArgs...> bind_front(
            F &&func, BoundArgs &&... args) {
        return functional_internal::bind_front_t<F, BoundArgs...>(
                abel::in_place, abel::forward<F>(func),
                abel::forward<BoundArgs>(args)...);
    }


}  // namespace abel

#endif  // ABEL_FUNCTIONAL_BIND_FRONT_H_
