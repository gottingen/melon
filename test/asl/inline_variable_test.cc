//

#include <type_traits>

#include <abel/asl/internal/inline_variable.h>
#include <testing/inline_variable_testing.h>

#include <gtest/gtest.h>

namespace abel {

    namespace inline_variable_testing_internal {
        namespace {

            TEST(InlineVariableTest, Constexpr) {
                static_assert(inline_variable_foo.value == 5, "");
                static_assert(other_inline_variable_foo.value == 5, "");
                static_assert(inline_variable_int == 5, "");
                static_assert(other_inline_variable_int == 5, "");
            }

            TEST(InlineVariableTest, DefaultConstructedIdentityEquality) {
                EXPECT_EQ(get_foo_a().value, 5);
                EXPECT_EQ(get_foo_b().value, 5);
                EXPECT_EQ(&get_foo_a(), &get_foo_b());
            }

            TEST(InlineVariableTest, DefaultConstructedIdentityInequality) {
                EXPECT_NE(&inline_variable_foo, &other_inline_variable_foo);
            }

            TEST(InlineVariableTest, InitializedIdentityEquality) {
                EXPECT_EQ(get_int_a(), 5);
                EXPECT_EQ(get_int_b(), 5);
                EXPECT_EQ(&get_int_a(), &get_int_b());
            }

            TEST(InlineVariableTest, InitializedIdentityInequality) {
                EXPECT_NE(&inline_variable_int, &other_inline_variable_int);
            }

            TEST(InlineVariableTest, FunPtrType) {
                static_assert(
                        std::is_same<void (*)(),
                                std::decay<decltype(inline_variable_fun_ptr)>::type>::value,
                        "");
            }

        }  // namespace
    }  // namespace inline_variable_testing_internal

}  // namespace abel
