//

#include <abel/config/flag.h>
#include <algorithm>
#include <string>
#include <gtest/gtest.h>
#include <abel/config/usage_config.h>
#include <abel/strings/numbers.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/str_split.h>

ABEL_DECLARE_FLAG(int64_t, mistyped_int_flag);
ABEL_DECLARE_FLAG(std::vector<std::string>, mistyped_string_flag);

namespace {

    namespace flags = abel::flags_internal;

    std::string TestHelpMsg() { return "dynamic help"; }

    template<typename T>
    void *TestMakeDflt() {
        return new T{};
    }

    void TestCallback() {}

    template<typename T>
    bool TestConstructionFor() {
        constexpr flags::HelpInitArg help_arg{flags::FlagHelpSrc("literal help"),
                                              flags::FlagHelpSrcKind::kLiteral};
        constexpr flags::Flag<T> f1("f1", "file", &flags::flag_marshalling_ops<T>,
                                    help_arg, &TestMakeDflt<T>);
        EXPECT_EQ(f1.Name(), "f1");
        EXPECT_EQ(f1.Help(), "literal help");
        EXPECT_EQ(f1.Filename(), "file");

        ABEL_CONST_INIT static flags::Flag<T> f2(
                "f2", "file", &flags::flag_marshalling_ops<T>,
                {flags::FlagHelpSrc(&TestHelpMsg), flags::FlagHelpSrcKind::kGenFunc},
                &TestMakeDflt<T>);
        flags::FlagRegistrar<T, false>(&f2).OnUpdate(TestCallback);

        EXPECT_EQ(f2.Name(), "f2");
        EXPECT_EQ(f2.Help(), "dynamic help");
        EXPECT_EQ(f2.Filename(), "file");

        return true;
    }

    struct UDT {
        UDT() = default;

        UDT(const UDT &) = default;
    };

    bool abel_parse_flag(abel::string_view, UDT *, std::string *) { return true; }

    std::string abel_unparse_flag(const UDT &) { return ""; }

    class FlagTest : public testing::Test {
    protected:
        static void SetUpTestSuite() {
            // Install a function to normalize filenames before this test is run.
            abel::flags_usage_config default_config;
            default_config.normalize_filename = &FlagTest::NormalizeFileName;
            abel::set_flags_usage_config(default_config);
        }

    private:
        static std::string NormalizeFileName(abel::string_view fname) {
#ifdef _WIN32
            std::string normalized(fname);
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            fname = normalized;
#endif
            return std::string(fname);
        }
    };

    TEST_F(FlagTest, TestConstruction) {
        TestConstructionFor<bool>();
        TestConstructionFor<int16_t>();
        TestConstructionFor<uint16_t>();
        TestConstructionFor<int32_t>();
        TestConstructionFor<uint32_t>();
        TestConstructionFor<int64_t>();
        TestConstructionFor<uint64_t>();
        TestConstructionFor<double>();
        TestConstructionFor<float>();
        TestConstructionFor<std::string>();

        TestConstructionFor<UDT>();
    }

// --------------------------------------------------------------------

}  // namespace

ABEL_DECLARE_FLAG(bool, test_flag_01);
ABEL_DECLARE_FLAG(int, test_flag_02);
ABEL_DECLARE_FLAG(int16_t, test_flag_03);
ABEL_DECLARE_FLAG(uint16_t, test_flag_04);
ABEL_DECLARE_FLAG(int32_t, test_flag_05);
ABEL_DECLARE_FLAG(uint32_t, test_flag_06);
ABEL_DECLARE_FLAG(int64_t, test_flag_07);
ABEL_DECLARE_FLAG(uint64_t, test_flag_08);
ABEL_DECLARE_FLAG(double, test_flag_09);
ABEL_DECLARE_FLAG(float, test_flag_10);
ABEL_DECLARE_FLAG(std::string, test_flag_11);

namespace {

#if !ABEL_FLAGS_STRIP_NAMES

    TEST_F(FlagTest, TestFlagDeclaration) {
        // test that we can access flag objects.
        EXPECT_EQ(FLAGS_test_flag_01.Name(), "test_flag_01");
        EXPECT_EQ(FLAGS_test_flag_02.Name(), "test_flag_02");
        EXPECT_EQ(FLAGS_test_flag_03.Name(), "test_flag_03");
        EXPECT_EQ(FLAGS_test_flag_04.Name(), "test_flag_04");
        EXPECT_EQ(FLAGS_test_flag_05.Name(), "test_flag_05");
        EXPECT_EQ(FLAGS_test_flag_06.Name(), "test_flag_06");
        EXPECT_EQ(FLAGS_test_flag_07.Name(), "test_flag_07");
        EXPECT_EQ(FLAGS_test_flag_08.Name(), "test_flag_08");
        EXPECT_EQ(FLAGS_test_flag_09.Name(), "test_flag_09");
        EXPECT_EQ(FLAGS_test_flag_10.Name(), "test_flag_10");
        EXPECT_EQ(FLAGS_test_flag_11.Name(), "test_flag_11");
    }

#endif  // !ABEL_FLAGS_STRIP_NAMES

// --------------------------------------------------------------------

}  // namespace

ABEL_FLAG(bool, test_flag_01, true, "test flag 01");
ABEL_FLAG(int, test_flag_02, 1234, "test flag 02");
ABEL_FLAG(int16_t, test_flag_03, -34, "test flag 03");
ABEL_FLAG(uint16_t, test_flag_04, 189, "test flag 04");
ABEL_FLAG(int32_t, test_flag_05, 10765, "test flag 05");
ABEL_FLAG(uint32_t, test_flag_06, 40000, "test flag 06");
ABEL_FLAG(int64_t, test_flag_07, -1234567, "test flag 07");
ABEL_FLAG(uint64_t, test_flag_08, 9876543, "test flag 08");
ABEL_FLAG(double, test_flag_09, -9.876e-50, "test flag 09");
ABEL_FLAG(float, test_flag_10, 1.234e12f, "test flag 10");
ABEL_FLAG(std::string, test_flag_11, "", "test flag 11");

namespace {

#if !ABEL_FLAGS_STRIP_NAMES
    TEST_F(FlagTest, TestFlagDefinition) {
        abel::string_view expected_file_name = "config/flag_test.cc";

        EXPECT_EQ(FLAGS_test_flag_01.Name(), "test_flag_01");
        EXPECT_EQ(FLAGS_test_flag_01.Help(), "test flag 01");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_01.Filename(), expected_file_name))
                            << FLAGS_test_flag_01.Filename();

        EXPECT_EQ(FLAGS_test_flag_02.Name(), "test_flag_02");
        EXPECT_EQ(FLAGS_test_flag_02.Help(), "test flag 02");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_02.Filename(), expected_file_name))
                            << FLAGS_test_flag_02.Filename();

        EXPECT_EQ(FLAGS_test_flag_03.Name(), "test_flag_03");
        EXPECT_EQ(FLAGS_test_flag_03.Help(), "test flag 03");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_03.Filename(), expected_file_name))
                            << FLAGS_test_flag_03.Filename();

        EXPECT_EQ(FLAGS_test_flag_04.Name(), "test_flag_04");
        EXPECT_EQ(FLAGS_test_flag_04.Help(), "test flag 04");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_04.Filename(), expected_file_name))
                            << FLAGS_test_flag_04.Filename();

        EXPECT_EQ(FLAGS_test_flag_05.Name(), "test_flag_05");
        EXPECT_EQ(FLAGS_test_flag_05.Help(), "test flag 05");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_05.Filename(), expected_file_name))
                            << FLAGS_test_flag_05.Filename();

        EXPECT_EQ(FLAGS_test_flag_06.Name(), "test_flag_06");
        EXPECT_EQ(FLAGS_test_flag_06.Help(), "test flag 06");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_06.Filename(), expected_file_name))
                            << FLAGS_test_flag_06.Filename();

        EXPECT_EQ(FLAGS_test_flag_07.Name(), "test_flag_07");
        EXPECT_EQ(FLAGS_test_flag_07.Help(), "test flag 07");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_07.Filename(), expected_file_name))
                            << FLAGS_test_flag_07.Filename();

        EXPECT_EQ(FLAGS_test_flag_08.Name(), "test_flag_08");
        EXPECT_EQ(FLAGS_test_flag_08.Help(), "test flag 08");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_08.Filename(), expected_file_name))
                            << FLAGS_test_flag_08.Filename();

        EXPECT_EQ(FLAGS_test_flag_09.Name(), "test_flag_09");
        EXPECT_EQ(FLAGS_test_flag_09.Help(), "test flag 09");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_09.Filename(), expected_file_name))
                            << FLAGS_test_flag_09.Filename();

        EXPECT_EQ(FLAGS_test_flag_10.Name(), "test_flag_10");
        EXPECT_EQ(FLAGS_test_flag_10.Help(), "test flag 10");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_10.Filename(), expected_file_name))
                            << FLAGS_test_flag_10.Filename();

        EXPECT_EQ(FLAGS_test_flag_11.Name(), "test_flag_11");
        EXPECT_EQ(FLAGS_test_flag_11.Help(), "test flag 11");
        EXPECT_TRUE(abel::ends_with(FLAGS_test_flag_11.Filename(), expected_file_name))
                            << FLAGS_test_flag_11.Filename();
    }

#endif  // !ABEL_FLAGS_STRIP_NAMES

// --------------------------------------------------------------------

    TEST_F(FlagTest, TestDefault) {
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_01), true);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_02), 1234);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_03), -34);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_04), 189);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_05), 10765);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_06), 40000);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_07), -1234567);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_08), 9876543);
        EXPECT_NEAR(abel::get_flag(FLAGS_test_flag_09), -9.876e-50, 1e-55);
        EXPECT_NEAR(abel::get_flag(FLAGS_test_flag_10), 1.234e12f, 1e5f);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_11), "");
    }

// --------------------------------------------------------------------

    TEST_F(FlagTest, TestGetSet) {
        abel::set_flag(&FLAGS_test_flag_01, false);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_01), false);

        abel::set_flag(&FLAGS_test_flag_02, 321);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_02), 321);

        abel::set_flag(&FLAGS_test_flag_03, 67);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_03), 67);

        abel::set_flag(&FLAGS_test_flag_04, 1);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_04), 1);

        abel::set_flag(&FLAGS_test_flag_05, -908);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_05), -908);

        abel::set_flag(&FLAGS_test_flag_06, 4001);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_06), 4001);

        abel::set_flag(&FLAGS_test_flag_07, -23456);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_07), -23456);

        abel::set_flag(&FLAGS_test_flag_08, 975310);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_08), 975310);

        abel::set_flag(&FLAGS_test_flag_09, 1.00001);
        EXPECT_NEAR(abel::get_flag(FLAGS_test_flag_09), 1.00001, 1e-10);

        abel::set_flag(&FLAGS_test_flag_10, -3.54f);
        EXPECT_NEAR(abel::get_flag(FLAGS_test_flag_10), -3.54f, 1e-6f);

        abel::set_flag(&FLAGS_test_flag_11, "asdf");
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_11), "asdf");
    }

// --------------------------------------------------------------------

    int GetDflt1() { return 1; }

}  // namespace

ABEL_FLAG(int, test_flag_12, GetDflt1(), "test flag 12");
ABEL_FLAG(std::string, test_flag_13, abel::string_cat("AAA", "BBB"),
          "test flag 13");

namespace {

    TEST_F(FlagTest, TestNonConstexprDefault) {
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_12), 1);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_13), "AAABBB");
    }

// --------------------------------------------------------------------

}  // namespace

ABEL_FLAG(bool, test_flag_14, true, abel::string_cat("test ", "flag ", "14"));

namespace {

#if !ABEL_FLAGS_STRIP_HELP
    TEST_F(FlagTest, TestNonConstexprHelp) {
        EXPECT_EQ(FLAGS_test_flag_14.Help(), "test flag 14");
    }

#endif  //! ABEL_FLAGS_STRIP_HELP

// --------------------------------------------------------------------

    int cb_test_value = -1;

    void TestFlagCB();

}  // namespace

ABEL_FLAG(int, test_flag_with_cb, 100, "").OnUpdate(TestFlagCB);

ABEL_FLAG(int, test_flag_with_lambda_cb, 200, "").OnUpdate([]() {
    cb_test_value = abel::get_flag(FLAGS_test_flag_with_lambda_cb) +
                    abel::get_flag(FLAGS_test_flag_with_cb);
});

namespace {

    void TestFlagCB() { cb_test_value = abel::get_flag(FLAGS_test_flag_with_cb); }

// Tests side-effects of callback invocation.
    TEST_F(FlagTest, CallbackInvocation) {
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_with_cb), 100);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_with_lambda_cb), 200);
        EXPECT_EQ(cb_test_value, 300);

        abel::set_flag(&FLAGS_test_flag_with_cb, 1);
        EXPECT_EQ(cb_test_value, 1);

        abel::set_flag(&FLAGS_test_flag_with_lambda_cb, 3);
        EXPECT_EQ(cb_test_value, 4);
    }

// --------------------------------------------------------------------

    struct CustomUDT {
        CustomUDT() : a(1), b(1) {}

        CustomUDT(int a_, int b_) : a(a_), b(b_) {}

        friend bool operator==(const CustomUDT &f1, const CustomUDT &f2) {
            return f1.a == f2.a && f1.b == f2.b;
        }

        int a;
        int b;
    };

    bool abel_parse_flag(abel::string_view in, CustomUDT *f, std::string *) {
        std::vector<abel::string_view> parts =
                abel::string_split(in, ':', abel::skip_whitespace());

        if (parts.size() != 2) return false;

        if (!abel::simple_atoi(parts[0], &f->a)) return false;

        if (!abel::simple_atoi(parts[1], &f->b)) return false;

        return true;
    }

    std::string abel_unparse_flag(const CustomUDT &f) {
        return abel::string_cat(f.a, ":", f.b);
    }

}  // namespace

ABEL_FLAG(CustomUDT, test_flag_15, CustomUDT(), "test flag 15");

namespace {

    TEST_F(FlagTest, TestCustomUDT) {
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_15), CustomUDT(1, 1));
        abel::set_flag(&FLAGS_test_flag_15, CustomUDT(2, 3));
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_15), CustomUDT(2, 3));
    }

// MSVC produces link error on the type mismatch.
// Linux does not have build errors and validations work as expected.
#if 0  // !defined(_WIN32) && GTEST_HAS_DEATH_TEST

    TEST(Flagtest, TestTypeMismatchValidations) {
      // For builtin types, get_flag() only does validation in debug mode.
      EXPECT_DEBUG_DEATH(
          abel::get_flag(FLAGS_mistyped_int_flag),
          "Flag 'mistyped_int_flag' is defined as one type and declared "
          "as another");
      EXPECT_DEATH(abel::set_flag(&FLAGS_mistyped_int_flag, 0),
                   "Flag 'mistyped_int_flag' is defined as one type and declared "
                   "as another");

      EXPECT_DEATH(abel::get_flag(FLAGS_mistyped_string_flag),
                   "Flag 'mistyped_string_flag' is defined as one type and "
                   "declared as another");
      EXPECT_DEATH(
          abel::set_flag(&FLAGS_mistyped_string_flag, std::vector<std::string>{}),
          "Flag 'mistyped_string_flag' is defined as one type and declared as "
          "another");
    }

#endif

// --------------------------------------------------------------------

// A contrived type that offers implicit and explicit conversion from specific
// source types.
    struct ConversionTestVal {
        ConversionTestVal() = default;

        explicit ConversionTestVal(int a_in) : a(a_in) {}

        enum class ViaImplicitConv {
            kTen = 10, kEleven
        };

        // NOLINTNEXTLINE
        ConversionTestVal(ViaImplicitConv from) : a(static_cast<int>(from)) {}

        int a;
    };

    bool abel_parse_flag(abel::string_view in, ConversionTestVal *val_out,
                         std::string *) {
        if (!abel::simple_atoi(in, &val_out->a)) {
            return false;
        }
        return true;
    }

    std::string abel_unparse_flag(const ConversionTestVal &val) {
        return abel::string_cat(val.a);
    }

}  // namespace

// Flag default values can be specified with a value that converts to the flag
// value type implicitly.
ABEL_FLAG(ConversionTestVal, test_flag_16,
          ConversionTestVal::ViaImplicitConv::kTen, "test flag 16");

namespace {

    TEST_F(FlagTest, CanSetViaImplicitConversion) {
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_16).a, 10);
        abel::set_flag(&FLAGS_test_flag_16,
                      ConversionTestVal::ViaImplicitConv::kEleven);
        EXPECT_EQ(abel::get_flag(FLAGS_test_flag_16).a, 11);
    }

// --------------------------------------------------------------------

    struct NonDfltConstructible {
    public:
        // This constructor tests that we can initialize the flag with int value
        NonDfltConstructible(int i) : value(i) {}  // NOLINT

        // This constructor tests that we can't initialize the flag with char value
        // but can with explicitly constructed NonDfltConstructible.
        explicit NonDfltConstructible(char c) : value(100 + static_cast<int>(c)) {}

        int value;
    };

    bool abel_parse_flag(abel::string_view in, NonDfltConstructible *ndc_out,
                         std::string *) {
        return abel::simple_atoi(in, &ndc_out->value);
    }

    std::string abel_unparse_flag(const NonDfltConstructible &ndc) {
        return abel::string_cat(ndc.value);
    }

}  // namespace

ABEL_FLAG(NonDfltConstructible, ndc_flag1, NonDfltConstructible('1'),
          "Flag with non default constructible type");
ABEL_FLAG(NonDfltConstructible, ndc_flag2, 0,
          "Flag with non default constructible type");

namespace {

    TEST_F(FlagTest, TestNonDefaultConstructibleType) {
        EXPECT_EQ(abel::get_flag(FLAGS_ndc_flag1).value, '1' + 100);
        EXPECT_EQ(abel::get_flag(FLAGS_ndc_flag2).value, 0);

        abel::set_flag(&FLAGS_ndc_flag1, NonDfltConstructible('A'));
        abel::set_flag(&FLAGS_ndc_flag2, 25);

        EXPECT_EQ(abel::get_flag(FLAGS_ndc_flag1).value, 'A' + 100);
        EXPECT_EQ(abel::get_flag(FLAGS_ndc_flag2).value, 25);
    }

// --------------------------------------------------------------------

}  // namespace

ABEL_RETIRED_FLAG(bool, old_bool_flag, true, "old descr");
ABEL_RETIRED_FLAG(int, old_int_flag, (int) std::sqrt(10), "old descr");
ABEL_RETIRED_FLAG(std::string, old_str_flag, "", abel::string_cat("old ", "descr"));

namespace {

    TEST_F(FlagTest, TestRetiredFlagRegistration) {
        bool is_bool = false;
        EXPECT_TRUE(flags::is_retired_flag("old_bool_flag", &is_bool));
        EXPECT_TRUE(is_bool);
        EXPECT_TRUE(flags::is_retired_flag("old_int_flag", &is_bool));
        EXPECT_FALSE(is_bool);
        EXPECT_TRUE(flags::is_retired_flag("old_str_flag", &is_bool));
        EXPECT_FALSE(is_bool);
        EXPECT_FALSE(flags::is_retired_flag("some_other_flag", &is_bool));
    }

}  // namespace
