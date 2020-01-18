//

#ifndef ABEL_BASE_INLINE_VARIABLE_TESTING_H_
#define ABEL_BASE_INLINE_VARIABLE_TESTING_H_

#include <abel/base/internal/inline_variable.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace inline_variable_testing_internal {

struct Foo {
  int value = 5;
};

ABEL_INTERNAL_INLINE_CONSTEXPR(Foo, inline_variable_foo, {});
ABEL_INTERNAL_INLINE_CONSTEXPR(Foo, other_inline_variable_foo, {});

ABEL_INTERNAL_INLINE_CONSTEXPR(int, inline_variable_int, 5);
ABEL_INTERNAL_INLINE_CONSTEXPR(int, other_inline_variable_int, 5);

ABEL_INTERNAL_INLINE_CONSTEXPR(void(*)(), inline_variable_fun_ptr, nullptr);

const Foo& get_foo_a();
const Foo& get_foo_b();

const int& get_int_a();
const int& get_int_b();

}  // namespace inline_variable_testing_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_INLINE_VARIABLE_TESTING_H_
