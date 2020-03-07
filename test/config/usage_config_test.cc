//

#include <abel/config/flags/usage_config.h>

#include <gtest/gtest.h>
#include <abel/config/flags/internal/path_util.h>
#include <abel/config/flags/internal/program_name.h>
#include <abel/strings/starts_with.h>
#include <abel/strings/ends_with.h>

namespace {

    class FlagsUsageConfigTest : public testing::Test {
    protected:
        void SetUp() override {
            // Install Default config for the use on this unit test.
            // Binary may install a custom config before tests are run.
            abel::FlagsUsageConfig default_config;
            abel::SetFlagsUsageConfig(default_config);
        }
    };

    namespace flags = abel::flags_internal;

    bool TstContainsHelpshortFlags(abel::string_view f) {
        return abel::starts_with(flags::Basename(f), "progname.");
    }

    bool TstContainsHelppackageFlags(abel::string_view f) {
        return abel::ends_with(flags::Package(f), "aaa/");
    }

    bool TstContainsHelpFlags(abel::string_view f) {
        return abel::ends_with(flags::Package(f), "zzz/");
    }

    std::string TstVersionString() { return "program 1.0.0"; }

    std::string TstNormalizeFilename(abel::string_view filename) {
        return std::string(filename.substr(2));
    }

//void TstReportUsageMessage(abel::string_view msg) {}

// --------------------------------------------------------------------

    TEST_F(FlagsUsageConfigTest, TestGetSetFlagsUsageConfig) {
        EXPECT_TRUE(flags::GetUsageConfig().contains_helpshort_flags);
        EXPECT_TRUE(flags::GetUsageConfig().contains_help_flags);
        EXPECT_TRUE(flags::GetUsageConfig().contains_helppackage_flags);
        EXPECT_TRUE(flags::GetUsageConfig().version_string);
        EXPECT_TRUE(flags::GetUsageConfig().normalize_filename);

        abel::FlagsUsageConfig empty_config;
        empty_config.contains_helpshort_flags = &TstContainsHelpshortFlags;
        empty_config.contains_help_flags = &TstContainsHelpFlags;
        empty_config.contains_helppackage_flags = &TstContainsHelppackageFlags;
        empty_config.version_string = &TstVersionString;
        empty_config.normalize_filename = &TstNormalizeFilename;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_TRUE(flags::GetUsageConfig().contains_helpshort_flags);
        EXPECT_TRUE(flags::GetUsageConfig().contains_help_flags);
        EXPECT_TRUE(flags::GetUsageConfig().contains_helppackage_flags);
        EXPECT_TRUE(flags::GetUsageConfig().version_string);
        EXPECT_TRUE(flags::GetUsageConfig().normalize_filename);
    }

// --------------------------------------------------------------------

    TEST_F(FlagsUsageConfigTest, TestContainsHelpshortFlags) {
        flags::SetProgramInvocationName("usage_config_test");

        auto config = flags::GetUsageConfig();
        EXPECT_TRUE(config.contains_helpshort_flags("adir/cd/usage_config_test.cc"));
        EXPECT_TRUE(
                config.contains_helpshort_flags("aaaa/usage_config_test-main.cc"));
        EXPECT_TRUE(config.contains_helpshort_flags("abc/usage_config_test_main.cc"));
        EXPECT_FALSE(config.contains_helpshort_flags("usage_config_main.cc"));

        abel::FlagsUsageConfig empty_config;
        empty_config.contains_helpshort_flags = &TstContainsHelpshortFlags;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_TRUE(
                flags::GetUsageConfig().contains_helpshort_flags("aaa/progname.cpp"));
        EXPECT_FALSE(
                flags::GetUsageConfig().contains_helpshort_flags("aaa/progmane.cpp"));
    }

// --------------------------------------------------------------------

    TEST_F(FlagsUsageConfigTest, TestContainsHelpFlags) {
        flags::SetProgramInvocationName("usage_config_test");

        auto config = flags::GetUsageConfig();
        EXPECT_TRUE(config.contains_help_flags("zzz/usage_config_test.cc"));
        EXPECT_TRUE(
                config.contains_help_flags("bdir/a/zzz/usage_config_test-main.cc"));
        EXPECT_TRUE(
                config.contains_help_flags("//aqse/zzz/usage_config_test_main.cc"));
        EXPECT_FALSE(config.contains_help_flags("zzz/aa/usage_config_main.cc"));

        abel::FlagsUsageConfig empty_config;
        empty_config.contains_help_flags = &TstContainsHelpFlags;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_TRUE(flags::GetUsageConfig().contains_help_flags("zzz/main-body.c"));
        EXPECT_FALSE(
                flags::GetUsageConfig().contains_help_flags("zzz/dir/main-body.c"));
    }

// --------------------------------------------------------------------

    TEST_F(FlagsUsageConfigTest, TestContainsHelppackageFlags) {
        flags::SetProgramInvocationName("usage_config_test");

        auto config = flags::GetUsageConfig();
        EXPECT_TRUE(config.contains_helppackage_flags("aaa/usage_config_test.cc"));
        EXPECT_TRUE(
                config.contains_helppackage_flags("bbdir/aaa/usage_config_test-main.cc"));
        EXPECT_TRUE(config.contains_helppackage_flags(
                "//aqswde/aaa/usage_config_test_main.cc"));
        EXPECT_FALSE(config.contains_helppackage_flags("aadir/usage_config_main.cc"));

        abel::FlagsUsageConfig empty_config;
        empty_config.contains_helppackage_flags = &TstContainsHelppackageFlags;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_TRUE(
                flags::GetUsageConfig().contains_helppackage_flags("aaa/main-body.c"));
        EXPECT_FALSE(
                flags::GetUsageConfig().contains_helppackage_flags("aadir/main-body.c"));
    }

// --------------------------------------------------------------------

    TEST_F(FlagsUsageConfigTest, TestVersionString) {
        flags::SetProgramInvocationName("usage_config_test");

#ifdef NDEBUG
        std::string expected_output = "usage_config_test\n";
#else
        std::string expected_output =
                "usage_config_test\nDebug build (NDEBUG not #defined)\n";
#endif

        EXPECT_EQ(flags::GetUsageConfig().version_string(), expected_output);

        abel::FlagsUsageConfig empty_config;
        empty_config.version_string = &TstVersionString;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_EQ(flags::GetUsageConfig().version_string(), "program 1.0.0");
    }

// --------------------------------------------------------------------

    TEST_F(FlagsUsageConfigTest, TestNormalizeFilename) {
        // This tests the default implementation.
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("a/a.cc"), "a/a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("/a/a.cc"), "a/a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("///a/a.cc"), "a/a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("/"), "");

        // This tests that the custom implementation is called.
        abel::FlagsUsageConfig empty_config;
        empty_config.normalize_filename = &TstNormalizeFilename;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("a/a.cc"), "a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("aaa/a.cc"), "a/a.cc");

        // This tests that the default implementation is called.
        empty_config.normalize_filename = nullptr;
        abel::SetFlagsUsageConfig(empty_config);

        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("a/a.cc"), "a/a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("/a/a.cc"), "a/a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("///a/a.cc"), "a/a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("\\a\\a.cc"), "a\\a.cc");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("//"), "");
        EXPECT_EQ(flags::GetUsageConfig().normalize_filename("\\\\"), "");
    }

}  // namespace
