//

#include "inline_variable_testing.h"

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace inline_variable_testing_internal {

const Foo& get_foo_b() { return inline_variable_foo; }

const int& get_int_b() { return inline_variable_int; }

}  // namespace inline_variable_testing_internal
ABEL_NAMESPACE_END
}  // namespace abel
