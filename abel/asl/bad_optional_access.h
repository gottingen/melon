//
// -----------------------------------------------------------------------------
// bad_optional_access.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::bad_optional_access` type.

#ifndef ABEL_ASL_BAD_OPTIONAL_ACCESS_H_
#define ABEL_ASL_BAD_OPTIONAL_ACCESS_H_

#include <stdexcept>

#include <abel/base/profile.h>

#ifdef ABEL_USES_STD_OPTIONAL

#include <optional>

namespace abel {

using std::bad_optional_access;

}  // namespace abel

#else  // ABEL_USES_STD_OPTIONAL

namespace abel {


// -----------------------------------------------------------------------------
// bad_optional_access
// -----------------------------------------------------------------------------
//
// An `abel::bad_optional_access` type is an exception type that is thrown when
// attempting to access an `abel::optional` object that does not contain a
// value.
//
// Example:
//
//   abel::optional<int> o;
//
//   try {
//     int n = o.value();
//   } catch(const abel::bad_optional_access& e) {
//     std::cout << "Bad optional access: " << e.what() << '\n';
//   }
    class bad_optional_access : public std::exception {
    public:
        bad_optional_access() = default;

        ~bad_optional_access() override;

        const char *what() const noexcept override;
    };

    namespace optional_internal {

// throw delegator
        [[noreturn]] void throw_bad_optional_access();

    }  // namespace optional_internal

}  // namespace abel

#endif  // ABEL_USES_STD_OPTIONAL

#endif  // ABEL_ASL_BAD_OPTIONAL_ACCESS_H_
