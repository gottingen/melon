//

#include <abel/strings/substitute.h>

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>
#include <abel/strings/str_cat.h>

namespace {

    TEST(SubstituteTest, Substitute) {
        // Basic.
        EXPECT_EQ("Hello, world!", abel::Substitute("$0, $1!", "Hello", "world"));

        // Non-char* types.
        EXPECT_EQ("123 0.2 0.1 foo true false x",
                  abel::Substitute("$0 $1 $2 $3 $4 $5 $6", 123, 0.2, 0.1f,
                                   std::string("foo"), true, false, 'x'));

        // All int types.
        EXPECT_EQ(
                "-32767 65535 "
                "-1234567890 3234567890 "
                "-1234567890 3234567890 "
                "-1234567890123456789 9234567890123456789",
                abel::Substitute(
                        "$0 $1 $2 $3 $4 $5 $6 $7",
                        static_cast<short>(-32767),          // NOLINT(runtime/int)
                        static_cast<unsigned short>(65535),  // NOLINT(runtime/int)
                        -1234567890, 3234567890U, -1234567890L, 3234567890UL,
                        -int64_t{1234567890123456789}, uint64_t{9234567890123456789u}));

        // Hex format
        EXPECT_EQ("0 1 f ffff0ffff 0123456789abcdef",
                  abel::Substitute("$0$1$2$3$4 $5",  //
                                   abel::hex(0), abel::hex(1, abel::kSpacePad2),
                                   abel::hex(0xf, abel::kSpacePad2),
                                   abel::hex(int16_t{-1}, abel::kSpacePad5),
                                   abel::hex(int16_t{-1}, abel::kZeroPad5),
                                   abel::hex(0x123456789abcdef, abel::kZeroPad16)));

        // dec format
        EXPECT_EQ("0 115   -1-0001 81985529216486895",
                  abel::Substitute("$0$1$2$3$4 $5",  //
                                   abel::dec(0), abel::dec(1, abel::kSpacePad2),
                                   abel::dec(0xf, abel::kSpacePad2),
                                   abel::dec(int16_t{-1}, abel::kSpacePad5),
                                   abel::dec(int16_t{-1}, abel::kZeroPad5),
                                   abel::dec(0x123456789abcdef, abel::kZeroPad16)));

        // Pointer.
        const int *int_p = reinterpret_cast<const int *>(0x12345);
        std::string str = abel::Substitute("$0", int_p);
        EXPECT_EQ(abel::string_cat("0x", abel::hex(int_p)), str);

        // Volatile Pointer.
        // Like C++ streamed I/O, such pointers implicitly become bool
        volatile int vol = 237;
        volatile int *volatile volptr = &vol;
        str = abel::Substitute("$0", volptr);
        EXPECT_EQ("true", str);

        // null is special. string_cat prints 0x0. Substitute prints NULL.
        const uint64_t *null_p = nullptr;
        str = abel::Substitute("$0", null_p);
        EXPECT_EQ("NULL", str);

        // char* is also special.
        const char *char_p = "print me";
        str = abel::Substitute("$0", char_p);
        EXPECT_EQ("print me", str);

        char char_buf[16];
        strncpy(char_buf, "print me too", sizeof(char_buf));
        str = abel::Substitute("$0", char_buf);
        EXPECT_EQ("print me too", str);

        // null char* is "doubly" special. Represented as the empty std::string.
        char_p = nullptr;
        str = abel::Substitute("$0", char_p);
        EXPECT_EQ("", str);

        // Out-of-order.
        EXPECT_EQ("b, a, c, b", abel::Substitute("$1, $0, $2, $1", "a", "b", "c"));

        // Literal $
        EXPECT_EQ("$", abel::Substitute("$$"));

        EXPECT_EQ("$1", abel::Substitute("$$1"));

        // Test all overloads.
        EXPECT_EQ("a", abel::Substitute("$0", "a"));
        EXPECT_EQ("a b", abel::Substitute("$0 $1", "a", "b"));
        EXPECT_EQ("a b c", abel::Substitute("$0 $1 $2", "a", "b", "c"));
        EXPECT_EQ("a b c d", abel::Substitute("$0 $1 $2 $3", "a", "b", "c", "d"));
        EXPECT_EQ("a b c d e",
                  abel::Substitute("$0 $1 $2 $3 $4", "a", "b", "c", "d", "e"));
        EXPECT_EQ("a b c d e f", abel::Substitute("$0 $1 $2 $3 $4 $5", "a", "b", "c",
                                                  "d", "e", "f"));
        EXPECT_EQ("a b c d e f g", abel::Substitute("$0 $1 $2 $3 $4 $5 $6", "a", "b",
                                                    "c", "d", "e", "f", "g"));
        EXPECT_EQ("a b c d e f g h",
                  abel::Substitute("$0 $1 $2 $3 $4 $5 $6 $7", "a", "b", "c", "d", "e",
                                   "f", "g", "h"));
        EXPECT_EQ("a b c d e f g h i",
                  abel::Substitute("$0 $1 $2 $3 $4 $5 $6 $7 $8", "a", "b", "c", "d",
                                   "e", "f", "g", "h", "i"));
        EXPECT_EQ("a b c d e f g h i j",
                  abel::Substitute("$0 $1 $2 $3 $4 $5 $6 $7 $8 $9", "a", "b", "c",
                                   "d", "e", "f", "g", "h", "i", "j"));
        EXPECT_EQ("a b c d e f g h i j b0",
                  abel::Substitute("$0 $1 $2 $3 $4 $5 $6 $7 $8 $9 $10", "a", "b", "c",
                                   "d", "e", "f", "g", "h", "i", "j"));

        const char *null_cstring = nullptr;
        EXPECT_EQ("Text: ''", abel::Substitute("Text: '$0'", null_cstring));
    }

    TEST(SubstituteTest, SubstituteAndAppend) {
        std::string str = "Hello";
        abel::SubstituteAndAppend(&str, ", $0!", "world");
        EXPECT_EQ("Hello, world!", str);

        // Test all overloads.
        str.clear();
        abel::SubstituteAndAppend(&str, "$0", "a");
        EXPECT_EQ("a", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1", "a", "b");
        EXPECT_EQ("a b", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2", "a", "b", "c");
        EXPECT_EQ("a b c", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3", "a", "b", "c", "d");
        EXPECT_EQ("a b c d", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4", "a", "b", "c", "d", "e");
        EXPECT_EQ("a b c d e", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5", "a", "b", "c", "d", "e",
                                  "f");
        EXPECT_EQ("a b c d e f", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6", "a", "b", "c", "d",
                                  "e", "f", "g");
        EXPECT_EQ("a b c d e f g", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6 $7", "a", "b", "c", "d",
                                  "e", "f", "g", "h");
        EXPECT_EQ("a b c d e f g h", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6 $7 $8", "a", "b", "c",
                                  "d", "e", "f", "g", "h", "i");
        EXPECT_EQ("a b c d e f g h i", str);
        str.clear();
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6 $7 $8 $9", "a", "b",
                                  "c", "d", "e", "f", "g", "h", "i", "j");
        EXPECT_EQ("a b c d e f g h i j", str);
    }

    TEST(SubstituteTest, VectorBoolRef) {
        std::vector<bool> v = {true, false};
        const auto &cv = v;
        EXPECT_EQ("true false true false",
                  abel::Substitute("$0 $1 $2 $3", v[0], v[1], cv[0], cv[1]));

        std::string str = "Logic be like: ";
        abel::SubstituteAndAppend(&str, "$0 $1 $2 $3", v[0], v[1], cv[0], cv[1]);
        EXPECT_EQ("Logic be like: true false true false", str);
    }

#ifdef GTEST_HAS_DEATH_TEST

    TEST(SubstituteDeathTest, SubstituteDeath) {
        EXPECT_DEBUG_DEATH(
                static_cast<void>(abel::Substitute(abel::string_view("-$2"), "a", "b")),
                "Invalid abel::Substitute\\(\\) format std::string: asked for \"\\$2\", "
                "but only 2 args were given.");
        EXPECT_DEBUG_DEATH(
                static_cast<void>(abel::Substitute(abel::string_view("-$z-"))),
                "Invalid abel::Substitute\\(\\) format std::string: \"-\\$z-\"");
        EXPECT_DEBUG_DEATH(
                static_cast<void>(abel::Substitute(abel::string_view("-$"))),
                "Invalid abel::Substitute\\(\\) format std::string: \"-\\$\"");
    }

#endif  // GTEST_HAS_DEATH_TEST

}  // namespace
