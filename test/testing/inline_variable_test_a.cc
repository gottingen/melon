//

#include "inline_variable_testing.h"

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace inline_variable_testing_internal {

const Foo& get_foo_a() { return inline_variable_foo; }

const int& get_int_a() { return inline_variable_int; }

}  // namespace inline_variable_testing_internal
ABEL_NAMESPACE_END
}  // namespace abel
