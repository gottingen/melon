//

#include <abel/config/internal/program_name.h>
#include <abel/strings/ends_with.h>
#include <gtest/gtest.h>

namespace {

    namespace flags = abel::flags_internal;

    TEST(FlagsPathUtilTest, TestInitialProgamName) {
        flags::set_program_invocation_name("flags/program_name_test");
        std::string program_name = flags::program_invocation_name();
        for (char &c : program_name)
            if (c == '\\') c = '/';

#if !defined(__wasm__) && !defined(__asmjs__)
        const std::string expect_name = "flags/program_name_test";
        const std::string expect_basename = "program_name_test";
#else
        // For targets that generate javascript or webassembly the invocation name
        // has the special value below.
        const std::string expect_name = "this.program";
        const std::string expect_basename = "this.program";
#endif

        EXPECT_TRUE(abel::ends_with(program_name, expect_name)) << program_name;
        EXPECT_EQ(flags::short_program_invocation_name(), expect_basename);
    }

    TEST(FlagsPathUtilTest, TestProgamNameInterfaces) {
        flags::set_program_invocation_name("a/my_test");

        EXPECT_EQ(flags::program_invocation_name(), "a/my_test");
        EXPECT_EQ(flags::short_program_invocation_name(), "my_test");

        abel::string_view not_null_terminated("abel/aaa/bbb");
        not_null_terminated = not_null_terminated.substr(1, 10);

        flags::set_program_invocation_name(not_null_terminated);

        EXPECT_EQ(flags::program_invocation_name(), "bel/aaa/bb");
        EXPECT_EQ(flags::short_program_invocation_name(), "bb");
    }

}  // namespace
