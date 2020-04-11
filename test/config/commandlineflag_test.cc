//

#include <abel/config/internal/command_line_flag.h>
#include <algorithm>
#include <string>
#include <gtest/gtest.h>
#include <abel/config/flag.h>
#include <abel/config/internal/registry.h>
#include <abel/config/usage_config.h>
#include <abel/memory/memory.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/ends_with.h>

ABEL_FLAG(int, int_flag, 201, "int_flag help");
ABEL_FLAG(std::string, string_flag, "dflt",
          abel::string_cat("string_flag", " help"));
ABEL_RETIRED_FLAG(bool, bool_retired_flag, false, "bool_retired_flag help");

namespace {

    namespace flags = abel::flags_internal;

    class CommandLineFlagTest : public testing::Test {
    protected:
        static void SetUpTestSuite() {
            // Install a function to normalize filenames before this test is run.
            abel::flags_usage_config default_config;
            default_config.normalize_filename = &CommandLineFlagTest::NormalizeFileName;
            abel::set_flags_usage_config(default_config);
        }

        void SetUp() override { flag_saver_ = abel::make_unique<flags::flag_saver>(); }

        void TearDown() override { flag_saver_.reset(); }

    private:
        static std::string NormalizeFileName(abel::string_view fname) {
#ifdef _WIN32
            std::string normalized(fname);
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            fname = normalized;
#endif
            return std::string(fname);
        }

        std::unique_ptr<flags::flag_saver> flag_saver_;
    };

    TEST_F(CommandLineFlagTest, TestAttributesAccessMethods) {
        auto *flag_01 = flags::find_command_line_flag("int_flag");

        ASSERT_TRUE(flag_01);
        EXPECT_EQ(flag_01->name(), "int_flag");
        EXPECT_EQ(flag_01->help(), "int_flag help");
        EXPECT_EQ(flag_01->type_name(), "");
        EXPECT_TRUE(!flag_01->is_retired());
        EXPECT_TRUE(flag_01->is_of_type<int>());
        EXPECT_TRUE(
                abel::ends_with(flag_01->file_name(),
                                "commandlineflag_test.cc"))
                            << flag_01->file_name();

        auto *flag_02 = flags::find_command_line_flag("string_flag");

        ASSERT_TRUE(flag_02);
        EXPECT_EQ(flag_02->name(), "string_flag");
        EXPECT_EQ(flag_02->help(), "string_flag help");
        EXPECT_EQ(flag_02->type_name(), "");
        EXPECT_TRUE(!flag_02->is_retired());
        EXPECT_TRUE(flag_02->is_of_type<std::string>());
        EXPECT_TRUE(
                abel::ends_with(flag_02->file_name(),
                                "commandlineflag_test.cc"))
                            << flag_02->file_name();

        auto *flag_03 = flags::find_retired_flag("bool_retired_flag");

        ASSERT_TRUE(flag_03);
        EXPECT_EQ(flag_03->name(), "bool_retired_flag");
        EXPECT_EQ(flag_03->help(), "");
        EXPECT_EQ(flag_03->type_name(), "");
        EXPECT_TRUE(flag_03->is_retired());
        EXPECT_TRUE(flag_03->is_of_type<bool>());
        EXPECT_EQ(flag_03->file_name(), "RETIRED");
    }

// --------------------------------------------------------------------

    TEST_F(CommandLineFlagTest, TestValueAccessMethods) {
        abel::set_flag(&FLAGS_int_flag, 301);
        auto *flag_01 = flags::find_command_line_flag("int_flag");

        ASSERT_TRUE(flag_01);
        EXPECT_EQ(flag_01->current_value(), "301");
        EXPECT_EQ(flag_01->default_value(), "201");

        abel::set_flag(&FLAGS_string_flag, "new_str_value");
        auto *flag_02 = flags::find_command_line_flag("string_flag");

        ASSERT_TRUE(flag_02);
        EXPECT_EQ(flag_02->current_value(), "new_str_value");
        EXPECT_EQ(flag_02->default_value(), "dflt");
    }

// --------------------------------------------------------------------

    TEST_F(CommandLineFlagTest, TestSetFromStringCurrentValue) {
        std::string err;

        auto *flag_01 = flags::find_command_line_flag("int_flag");
        EXPECT_FALSE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(flag_01->set_from_string("11", flags::SET_FLAGS_VALUE,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 11);
        EXPECT_FALSE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(flag_01->set_from_string("-123", flags::SET_FLAGS_VALUE,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), -123);
        EXPECT_FALSE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(!flag_01->set_from_string("xyz", flags::SET_FLAGS_VALUE,
                                            flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), -123);
        EXPECT_EQ(err, "Illegal value 'xyz' specified for flag 'int_flag'");
        EXPECT_FALSE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(!flag_01->set_from_string("A1", flags::SET_FLAGS_VALUE,
                                            flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), -123);
        EXPECT_EQ(err, "Illegal value 'A1' specified for flag 'int_flag'");
        EXPECT_FALSE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(flag_01->set_from_string("0x10", flags::SET_FLAGS_VALUE,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 16);
        EXPECT_FALSE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(flag_01->set_from_string("011", flags::SET_FLAGS_VALUE,
                                           flags::kCommandLine, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 11);
        EXPECT_TRUE(flag_01->is_specified_on_command_line());

        EXPECT_TRUE(!flag_01->set_from_string("", flags::SET_FLAGS_VALUE,
                                            flags::kProgrammaticChange, &err));
        EXPECT_EQ(err, "Illegal value '' specified for flag 'int_flag'");

        auto *flag_02 = flags::find_command_line_flag("string_flag");
        EXPECT_TRUE(flag_02->set_from_string("xyz", flags::SET_FLAGS_VALUE,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_string_flag), "xyz");

        EXPECT_TRUE(flag_02->set_from_string("", flags::SET_FLAGS_VALUE,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_string_flag), "");
    }

// --------------------------------------------------------------------

    TEST_F(CommandLineFlagTest, TestSetFromStringDefaultValue) {
        std::string err;

        auto *flag_01 = flags::find_command_line_flag("int_flag");

        EXPECT_TRUE(flag_01->set_from_string("111", flags::SET_FLAGS_DEFAULT,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(flag_01->default_value(), "111");

        auto *flag_02 = flags::find_command_line_flag("string_flag");

        EXPECT_TRUE(flag_02->set_from_string("abc", flags::SET_FLAGS_DEFAULT,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(flag_02->default_value(), "abc");
    }

// --------------------------------------------------------------------

    TEST_F(CommandLineFlagTest, TestSetFromStringIfDefault) {
        std::string err;

        auto *flag_01 = flags::find_command_line_flag("int_flag");

        EXPECT_TRUE(flag_01->set_from_string("22", flags::SET_FLAG_IF_DEFAULT,
                                           flags::kProgrammaticChange, &err))
                            << err;
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 22);

        EXPECT_TRUE(flag_01->set_from_string("33", flags::SET_FLAG_IF_DEFAULT,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 22);
        // EXPECT_EQ(err, "ERROR: int_flag is already set to 22");

        // Reset back to default value
        EXPECT_TRUE(flag_01->set_from_string("201", flags::SET_FLAGS_VALUE,
                                           flags::kProgrammaticChange, &err));

        EXPECT_TRUE(flag_01->set_from_string("33", flags::SET_FLAG_IF_DEFAULT,
                                           flags::kProgrammaticChange, &err));
        EXPECT_EQ(abel::get_flag(FLAGS_int_flag), 201);
        // EXPECT_EQ(err, "ERROR: int_flag is already set to 201");
    }

}  // namespace
