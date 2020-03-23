//
// -----------------------------------------------------------------------------
// bad_any_cast.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::bad_any_cast` type.

#ifndef ABEL_ASL_BAD_ANY_CAST_H_
#define ABEL_ASL_BAD_ANY_CAST_H_

#include <typeinfo>

#include <abel/base/profile.h>

#ifdef ABEL_USES_STD_ANY

#include <any>

namespace abel {

using std::bad_any_cast;

}  // namespace abel

#else  // ABEL_USES_STD_ANY

namespace abel {


// -----------------------------------------------------------------------------
// bad_any_cast
// -----------------------------------------------------------------------------
//
// An `abel::bad_any_cast` type is an exception type that is thrown when
// failing to successfully cast the return value of an `abel::any` object.
//
// Example:
//
//   auto a = abel::any(65);
//   abel::any_cast<int>(a);         // 65
//   try {
//     abel::any_cast<char>(a);
//   } catch(const abel::bad_any_cast& e) {
//     std::cout << "Bad any cast: " << e.what() << '\n';
//   }
    class bad_any_cast : public std::bad_cast {
    public:
        ~bad_any_cast() override;

        const char *what() const noexcept override;
    };

    namespace any_internal {

        [[noreturn]] void ThrowBadAnyCast();

    }  // namespace any_internal

}  // namespace abel

#endif  // ABEL_USES_STD_ANY

#endif  // ABEL_ASL_BAD_ANY_CAST_H_
