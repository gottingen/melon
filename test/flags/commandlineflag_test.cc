//

#include <abel/flags/internal/commandlineflag.h>

#include <algorithm>
#include <string>

#include <gtest/gtest.h>
#include <abel/flags/flag.h>
#include <abel/flags/internal/registry.h>
#include <abel/flags/usage_config.h>
#include <abel/memory/memory.h>
#include <abel/strings/match.h>
#include <abel/strings/str_cat.h>

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
    abel::FlagsUsageConfig default_config;
    default_config.normalize_filename = &CommandLineFlagTest::NormalizeFileName;
    abel::SetFlagsUsageConfig(default_config);
  }

  void SetUp() override { flag_saver_ = abel::make_unique<flags::FlagSaver>(); }
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

  std::unique_ptr<flags::FlagSaver> flag_saver_;
};

TEST_F(CommandLineFlagTest, TestAttributesAccessMethods) {
  auto* flag_01 = flags::FindCommandLineFlag("int_flag");

  ASSERT_TRUE(flag_01);
  EXPECT_EQ(flag_01->Name(), "int_flag");
  EXPECT_EQ(flag_01->Help(), "int_flag help");
  EXPECT_EQ(flag_01->Typename(), "");
  EXPECT_TRUE(!flag_01->IsRetired());
  EXPECT_TRUE(flag_01->IsOfType<int>());
  EXPECT_TRUE(
      abel::ends_with(flag_01->Filename(),
                     "commandlineflag_test.cc"))
      << flag_01->Filename();

  auto* flag_02 = flags::FindCommandLineFlag("string_flag");

  ASSERT_TRUE(flag_02);
  EXPECT_EQ(flag_02->Name(), "string_flag");
  EXPECT_EQ(flag_02->Help(), "string_flag help");
  EXPECT_EQ(flag_02->Typename(), "");
  EXPECT_TRUE(!flag_02->IsRetired());
  EXPECT_TRUE(flag_02->IsOfType<std::string>());
  EXPECT_TRUE(
      abel::ends_with(flag_02->Filename(),
                     "commandlineflag_test.cc"))
      << flag_02->Filename();

  auto* flag_03 = flags::FindRetiredFlag("bool_retired_flag");

  ASSERT_TRUE(flag_03);
  EXPECT_EQ(flag_03->Name(), "bool_retired_flag");
  EXPECT_EQ(flag_03->Help(), "");
  EXPECT_EQ(flag_03->Typename(), "");
  EXPECT_TRUE(flag_03->IsRetired());
  EXPECT_TRUE(flag_03->IsOfType<bool>());
  EXPECT_EQ(flag_03->Filename(), "RETIRED");
}

// --------------------------------------------------------------------

TEST_F(CommandLineFlagTest, TestValueAccessMethods) {
  abel::SetFlag(&FLAGS_int_flag, 301);
  auto* flag_01 = flags::FindCommandLineFlag("int_flag");

  ASSERT_TRUE(flag_01);
  EXPECT_EQ(flag_01->CurrentValue(), "301");
  EXPECT_EQ(flag_01->DefaultValue(), "201");

  abel::SetFlag(&FLAGS_string_flag, "new_str_value");
  auto* flag_02 = flags::FindCommandLineFlag("string_flag");

  ASSERT_TRUE(flag_02);
  EXPECT_EQ(flag_02->CurrentValue(), "new_str_value");
  EXPECT_EQ(flag_02->DefaultValue(), "dflt");
}

// --------------------------------------------------------------------

TEST_F(CommandLineFlagTest, TestSetFromStringCurrentValue) {
  std::string err;

  auto* flag_01 = flags::FindCommandLineFlag("int_flag");
  EXPECT_FALSE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(flag_01->SetFromString("11", flags::SET_FLAGS_VALUE,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), 11);
  EXPECT_FALSE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(flag_01->SetFromString("-123", flags::SET_FLAGS_VALUE,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), -123);
  EXPECT_FALSE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(!flag_01->SetFromString("xyz", flags::SET_FLAGS_VALUE,
                                      flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), -123);
  EXPECT_EQ(err, "Illegal value 'xyz' specified for flag 'int_flag'");
  EXPECT_FALSE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(!flag_01->SetFromString("A1", flags::SET_FLAGS_VALUE,
                                      flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), -123);
  EXPECT_EQ(err, "Illegal value 'A1' specified for flag 'int_flag'");
  EXPECT_FALSE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(flag_01->SetFromString("0x10", flags::SET_FLAGS_VALUE,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), 16);
  EXPECT_FALSE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(flag_01->SetFromString("011", flags::SET_FLAGS_VALUE,
                                     flags::kCommandLine, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), 11);
  EXPECT_TRUE(flag_01->IsSpecifiedOnCommandLine());

  EXPECT_TRUE(!flag_01->SetFromString("", flags::SET_FLAGS_VALUE,
                                      flags::kProgrammaticChange, &err));
  EXPECT_EQ(err, "Illegal value '' specified for flag 'int_flag'");

  auto* flag_02 = flags::FindCommandLineFlag("string_flag");
  EXPECT_TRUE(flag_02->SetFromString("xyz", flags::SET_FLAGS_VALUE,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_string_flag), "xyz");

  EXPECT_TRUE(flag_02->SetFromString("", flags::SET_FLAGS_VALUE,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_string_flag), "");
}

// --------------------------------------------------------------------

TEST_F(CommandLineFlagTest, TestSetFromStringDefaultValue) {
  std::string err;

  auto* flag_01 = flags::FindCommandLineFlag("int_flag");

  EXPECT_TRUE(flag_01->SetFromString("111", flags::SET_FLAGS_DEFAULT,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(flag_01->DefaultValue(), "111");

  auto* flag_02 = flags::FindCommandLineFlag("string_flag");

  EXPECT_TRUE(flag_02->SetFromString("abc", flags::SET_FLAGS_DEFAULT,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(flag_02->DefaultValue(), "abc");
}

// --------------------------------------------------------------------

TEST_F(CommandLineFlagTest, TestSetFromStringIfDefault) {
  std::string err;

  auto* flag_01 = flags::FindCommandLineFlag("int_flag");

  EXPECT_TRUE(flag_01->SetFromString("22", flags::SET_FLAG_IF_DEFAULT,
                                     flags::kProgrammaticChange, &err))
      << err;
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), 22);

  EXPECT_TRUE(flag_01->SetFromString("33", flags::SET_FLAG_IF_DEFAULT,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), 22);
  // EXPECT_EQ(err, "ERROR: int_flag is already set to 22");

  // Reset back to default value
  EXPECT_TRUE(flag_01->SetFromString("201", flags::SET_FLAGS_VALUE,
                                     flags::kProgrammaticChange, &err));

  EXPECT_TRUE(flag_01->SetFromString("33", flags::SET_FLAG_IF_DEFAULT,
                                     flags::kProgrammaticChange, &err));
  EXPECT_EQ(abel::GetFlag(FLAGS_int_flag), 201);
  // EXPECT_EQ(err, "ERROR: int_flag is already set to 201");
}

}  // namespace
