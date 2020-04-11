//

#include <abel/config/internal/type_erased.h>

#include <cmath>

#include <gtest/gtest.h>
#include <abel/config/flag.h>
#include <abel/memory/memory.h>
#include <abel/strings/str_cat.h>

ABEL_FLAG(int, int_flag, 1, "int_flag help");
ABEL_FLAG(std::string, string_flag, "dflt", "string_flag help");
ABEL_RETIRED_FLAG(bool, bool_retired_flag, false, "bool_retired_flag help");

namespace {

    namespace flags = abel::flags_internal;

    class TypeErasedTest : public testing::Test {
    protected:
        void SetUp() override { flag_saver_ = abel::make_unique<flags::flag_saver>(); }

        void TearDown() override { flag_saver_.reset(); }

    private:
        std::unique_ptr<flags::flag_saver> flag_saver_;
    };

// --------------------------------------------------------------------

    TEST_F(TypeErasedTest, TestGetCommandLineOption) {
        std::string value;
        EXPECT_TRUE(flags::get_command_line_option("int_flag", &value));
        EXPECT_EQ(value, "1");

        EXPECT_TRUE(flags::get_command_line_option("string_flag", &value));
        EXPECT_EQ(value, "dflt");

        EXPECT_FALSE(flags::get_command_line_option("bool_retired_flag", &value));

        EXPECT_FALSE(flags::get_command_line_option("unknown_flag", &value));
    }

// --------------------------------------------------------------------

    TEST_F(TypeErasedTest, TestSetCommandLineOption) {
        EXPECT_TRUE(flags::set_command_line_option("int_flag", "101"));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 101);

        EXPECT_TRUE(flags::set_command_line_option("string_flag", "asdfgh"));
        EXPECT_EQ(abel::get_flag(FLAGS_string_flag), "asdfgh");

        EXPECT_FALSE(flags::set_command_line_option("bool_retired_flag", "true"));

        EXPECT_FALSE(flags::set_command_line_option("unknown_flag", "true"));
    }

// --------------------------------------------------------------------

    TEST_F(TypeErasedTest, TestSetCommandLineOptionWithMode_SET_FLAGS_VALUE) {
        EXPECT_TRUE(flags::set_command_line_option_with_mode("int_flag", "101",
                                                        flags::SET_FLAGS_VALUE));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 101);

        EXPECT_TRUE(flags::set_command_line_option_with_mode("string_flag", "asdfgh",
                                                        flags::SET_FLAGS_VALUE));
        EXPECT_EQ(abel::get_flag(FLAGS_string_flag), "asdfgh");

        EXPECT_FALSE(flags::set_command_line_option_with_mode("bool_retired_flag", "true",
                                                         flags::SET_FLAGS_VALUE));

        EXPECT_FALSE(flags::set_command_line_option_with_mode("unknown_flag", "true",
                                                         flags::SET_FLAGS_VALUE));
    }

// --------------------------------------------------------------------

    TEST_F(TypeErasedTest, TestSetCommandLineOptionWithMode_SET_FLAG_IF_DEFAULT) {
        EXPECT_TRUE(flags::set_command_line_option_with_mode("int_flag", "101",
                                                        flags::SET_FLAG_IF_DEFAULT));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 101);

        // This semantic is broken. We return true instead of false. Value is not
        // updated.
        EXPECT_TRUE(flags::set_command_line_option_with_mode("int_flag", "202",
                                                        flags::SET_FLAG_IF_DEFAULT));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 101);

        EXPECT_TRUE(flags::set_command_line_option_with_mode("string_flag", "asdfgh",
                                                        flags::SET_FLAG_IF_DEFAULT));
        EXPECT_EQ(abel::get_flag(FLAGS_string_flag), "asdfgh");

        EXPECT_FALSE(flags::set_command_line_option_with_mode("bool_retired_flag", "true",
                                                         flags::SET_FLAG_IF_DEFAULT));

        EXPECT_FALSE(flags::set_command_line_option_with_mode("unknown_flag", "true",
                                                         flags::SET_FLAG_IF_DEFAULT));
    }

// --------------------------------------------------------------------

    TEST_F(TypeErasedTest, TestSetCommandLineOptionWithMode_SET_FLAGS_DEFAULT) {
        EXPECT_TRUE(flags::set_command_line_option_with_mode("int_flag", "101",
                                                        flags::SET_FLAGS_DEFAULT));

        EXPECT_TRUE(flags::set_command_line_option_with_mode("string_flag", "asdfgh",
                                                        flags::SET_FLAGS_DEFAULT));
        EXPECT_EQ(abel::get_flag(FLAGS_string_flag), "asdfgh");

        EXPECT_FALSE(flags::set_command_line_option_with_mode("bool_retired_flag", "true",
                                                         flags::SET_FLAGS_DEFAULT));

        EXPECT_FALSE(flags::set_command_line_option_with_mode("unknown_flag", "true",
                                                         flags::SET_FLAGS_DEFAULT));

        // This should be successfull, since flag is still is not set
        EXPECT_TRUE(flags::set_command_line_option_with_mode("int_flag", "202",
                                                        flags::SET_FLAG_IF_DEFAULT));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 202);
    }

// --------------------------------------------------------------------

    TEST_F(TypeErasedTest, TestIsValidFlagValue) {
        EXPECT_TRUE(flags::is_valid_flag_value("int_flag", "57"));
        EXPECT_TRUE(flags::is_valid_flag_value("int_flag", "-101"));
        EXPECT_FALSE(flags::is_valid_flag_value("int_flag", "1.1"));

        EXPECT_TRUE(flags::is_valid_flag_value("string_flag", "#%^#%^$%DGHDG$W%adsf"));

        EXPECT_TRUE(flags::is_valid_flag_value("bool_retired_flag", "true"));
    }

}  // namespace
