
#include <testing/inline_variable_testing.h>

namespace abel {

    namespace inline_variable_testing_internal {

        const Foo &get_foo_b() { return inline_variable_foo; }

        const int &get_int_b() { return inline_variable_int; }

    }  // namespace inline_variable_testing_internal

}  // namespace abel
