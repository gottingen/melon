//

#include <abel/strings/ascii.h>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include <gtest/gtest.h>
#include <abel/base/profile.h>

namespace {

    TEST(AsciiIsFoo, All) {
        for (int i = 0; i < 256; i++) {
            if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z'))
                EXPECT_TRUE(abel::ascii::is_alpha(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_alpha(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if ((i >= '0' && i <= '9'))
                EXPECT_TRUE(abel::ascii::is_digit(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_digit(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (abel::ascii::is_alpha(i) || abel::ascii::is_digit(i))
                EXPECT_TRUE(abel::ascii::is_alpha_numeric(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_alpha_numeric(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i != '\0' && strchr(" \r\n\t\v\f", i))
                EXPECT_TRUE(abel::ascii::is_space(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_space(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i >= 32 && i < 127)
                EXPECT_TRUE(abel::ascii::is_print(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_print(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (abel::ascii::is_print(i) && !abel::ascii::is_space(i) &&
                !abel::ascii::is_alpha_numeric(i))
                EXPECT_TRUE(abel::ascii::is_punct(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_punct(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i == ' ' || i == '\t')
                EXPECT_TRUE(abel::ascii::is_blank(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_blank(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i < 32 || i == 127)
                EXPECT_TRUE(abel::ascii::is_control(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_control(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (abel::ascii::is_digit(i) || (i >= 'A' && i <= 'F') ||
                (i >= 'a' && i <= 'f'))
                EXPECT_TRUE(abel::ascii::is_hex_digit(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_hex_digit(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i > 32 && i < 127)
                EXPECT_TRUE(abel::ascii::is_graph(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_graph(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i >= 'A' && i <= 'Z')
                EXPECT_TRUE(abel::ascii::is_upper(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_upper(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 256; i++) {
            if (i >= 'a' && i <= 'z')
                EXPECT_TRUE(abel::ascii::is_lower(i)) << ": failed on " << i;
            else
                EXPECT_TRUE(!abel::ascii::is_lower(i)) << ": failed on " << i;
        }
        for (int i = 0; i < 128; i++) {
            EXPECT_TRUE(abel::ascii::is_ascii(i)) << ": failed on " << i;
        }
        for (int i = 128; i < 256; i++) {
            EXPECT_TRUE(!abel::ascii::is_ascii(i)) << ": failed on " << i;
        }

        // The official is* functions don't accept negative signed chars, but
        // our abel::ascii_is* functions do.
        for (int i = 0; i < 256; i++) {
            signed char sc = static_cast<signed char>(static_cast<unsigned char>(i));
            EXPECT_EQ(abel::ascii::is_alpha(i), abel::ascii::is_alpha(sc)) << i;
            EXPECT_EQ(abel::ascii::is_digit(i), abel::ascii::is_digit(sc)) << i;
            EXPECT_EQ(abel::ascii::is_alpha_numeric(i), abel::ascii::is_alpha_numeric(sc)) << i;
            EXPECT_EQ(abel::ascii::is_space(i), abel::ascii::is_space(sc)) << i;
            EXPECT_EQ(abel::ascii::is_punct(i), abel::ascii::is_punct(sc)) << i;
            EXPECT_EQ(abel::ascii::is_white(i), abel::ascii::is_white(sc)) << i;
            EXPECT_EQ(abel::ascii::is_control(i), abel::ascii::is_control(sc)) << i;
            EXPECT_EQ(abel::ascii::is_hex_digit(i), abel::ascii::is_hex_digit(sc)) << i;
            EXPECT_EQ(abel::ascii::is_print(i), abel::ascii::is_print(sc)) << i;
            EXPECT_EQ(abel::ascii::is_graph(i), abel::ascii::is_graph(sc)) << i;
            EXPECT_EQ(abel::ascii::is_upper(i), abel::ascii::is_upper(sc)) << i;
            EXPECT_EQ(abel::ascii::is_lower(i), abel::ascii::is_lower(sc)) << i;
            EXPECT_EQ(abel::ascii::is_ascii(i), abel::ascii::is_ascii(sc)) << i;
        }
    }

// Checks that abel::ascii_isfoo returns the same value as isfoo in the C
// locale.
    TEST(AsciiIsFoo, SameAsIsFoo) {
#ifndef __ANDROID__
        // temporarily change locale to C. It should already be C, but just for safety
        const char *old_locale = setlocale(LC_CTYPE, "C");
        ASSERT_TRUE(old_locale != nullptr);
#endif

        for (int i = 0; i < 256; i++) {
            EXPECT_EQ(isalpha(i) != 0, abel::ascii::is_alpha(i)) << i;
            EXPECT_EQ(isdigit(i) != 0, abel::ascii::is_digit(i)) << i;
            EXPECT_EQ(isalnum(i) != 0, abel::ascii::is_alpha_numeric(i)) << i;
            EXPECT_EQ(isspace(i) != 0, abel::ascii::is_space(i)) << i;
            EXPECT_EQ(ispunct(i) != 0, abel::ascii::is_punct(i)) << i;
            EXPECT_EQ(isblank(i) != 0, abel::ascii::is_blank(i)) << i;
            EXPECT_EQ(iscntrl(i) != 0, abel::ascii::is_control(i)) << i;
            EXPECT_EQ(isxdigit(i) != 0, abel::ascii::is_hex_digit(i)) << i;
            EXPECT_EQ(isprint(i) != 0, abel::ascii::is_print(i)) << i;
            EXPECT_EQ(isgraph(i) != 0, abel::ascii::is_graph(i)) << i;
            EXPECT_EQ(isupper(i) != 0, abel::ascii::is_upper(i)) << i;
            EXPECT_EQ(islower(i) != 0, abel::ascii::is_lower(i)) << i;
            EXPECT_EQ(isascii(i) != 0, abel::ascii::is_ascii(i)) << i;
        }

#ifndef __ANDROID__
        // restore the old locale.
        ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
    }

    TEST(AsciiToFoo, All) {
#ifndef __ANDROID__
        // temporarily change locale to C. It should already be C, but just for safety
        const char *old_locale = setlocale(LC_CTYPE, "C");
        ASSERT_TRUE(old_locale != nullptr);
#endif

        for (int i = 0; i < 256; i++) {
            if (abel::ascii::is_lower(i))
                EXPECT_EQ(abel::ascii::to_upper(i), 'A' + (i - 'a')) << i;
            else
                EXPECT_EQ(abel::ascii::to_upper(i), static_cast<char>(i)) << i;

            if (abel::ascii::is_upper(i))
                EXPECT_EQ(abel::ascii::to_lower(i), 'a' + (i - 'A')) << i;
            else
                EXPECT_EQ(abel::ascii::to_lower(i), static_cast<char>(i)) << i;

            // These CHECKs only hold in a C locale.
            EXPECT_EQ(static_cast<char>(tolower(i)), abel::ascii::to_lower(i)) << i;
            EXPECT_EQ(static_cast<char>(toupper(i)), abel::ascii::to_upper(i)) << i;

            // The official to* functions don't accept negative signed chars, but
            // our abel::ascii_to* functions do.
            signed char sc = static_cast<signed char>(static_cast<unsigned char>(i));
            EXPECT_EQ(abel::ascii::to_lower(i), abel::ascii::to_lower(sc)) << i;
            EXPECT_EQ(abel::ascii::to_upper(i), abel::ascii::to_upper(sc)) << i;
        }
#ifndef __ANDROID__
        // restore the old locale.
        ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
    }

}  // namespace
