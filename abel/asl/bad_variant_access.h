//
// -----------------------------------------------------------------------------
// bad_variant_access.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::bad_variant_access` type.

#ifndef ABEL_ASL_BAD_VARIANT_ACCESS_H_
#define ABEL_ASL_BAD_VARIANT_ACCESS_H_

#include <stdexcept>
#include <abel/base/profile.h>

#ifdef ABEL_USES_STD_VARIANT

#include <variant>

namespace abel {

using std::bad_variant_access;

}  // namespace abel

#else  // ABEL_USES_STD_VARIANT

namespace abel {


// -----------------------------------------------------------------------------
// bad_variant_access
// -----------------------------------------------------------------------------
//
// An `abel::bad_variant_access` type is an exception type that is thrown in
// the following cases:
//
//   * Calling `abel::get(abel::variant) with an index or type that does not
//     match the currently selected alternative type
//   * Calling `abel::visit on an `abel::variant` that is in the
//     `variant::valueless_by_exception` state.
//
// Example:
//
//   abel::variant<int, std::string> v;
//   v = 1;
//   try {
//     abel::get<std::string>(v);
//   } catch(const abel::bad_variant_access& e) {
//     std::cout << "Bad variant access: " << e.what() << '\n';
//   }
    class bad_variant_access : public std::exception {
    public:
        bad_variant_access() noexcept = default;

        ~bad_variant_access() override;

        const char *what() const noexcept override;
    };

    namespace variant_internal {

        [[noreturn]] void throw_bad_variant_access();

        [[noreturn]] void rethrow();

    }  // namespace variant_internal

}  // namespace abel

#endif  // ABEL_USES_STD_VARIANT

#endif  // ABEL_ASL_BAD_VARIANT_ACCESS_H_
