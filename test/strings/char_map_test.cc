//

#include <abel/strings/internal/char_map.h>

#include <cctype>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

    constexpr abel::strings_internal::Charmap everything_map =
            ~abel::strings_internal::Charmap();
    constexpr abel::strings_internal::Charmap nothing_map{};

    TEST(Charmap, AllTests) {
        const abel::strings_internal::Charmap also_nothing_map("", 0);
        ASSERT_TRUE(everything_map.contains('\0'));
        ASSERT_TRUE(!nothing_map.contains('\0'));
        ASSERT_TRUE(!also_nothing_map.contains('\0'));
        for (unsigned char ch = 1; ch != 0; ++ch) {
            ASSERT_TRUE(everything_map.contains(ch));
            ASSERT_TRUE(!nothing_map.contains(ch));
            ASSERT_TRUE(!also_nothing_map.contains(ch));
        }

        const abel::strings_internal::Charmap symbols("&@#@^!@?", 5);
        ASSERT_TRUE(symbols.contains('&'));
        ASSERT_TRUE(symbols.contains('@'));
        ASSERT_TRUE(symbols.contains('#'));
        ASSERT_TRUE(symbols.contains('^'));
        ASSERT_TRUE(!symbols.contains('!'));
        ASSERT_TRUE(!symbols.contains('?'));
        int cnt = 0;
        for (unsigned char ch = 1; ch != 0; ++ch)
            cnt += symbols.contains(ch);
        ASSERT_EQ(cnt, 4);

        const abel::strings_internal::Charmap lets("^abcde", 3);
        const abel::strings_internal::Charmap lets2("fghij\0klmnop", 10);
        const abel::strings_internal::Charmap lets3("fghij\0klmnop");
        ASSERT_TRUE(lets2.contains('k'));
        ASSERT_TRUE(!lets3.contains('k'));

        ASSERT_TRUE(symbols.IntersectsWith(lets));
        ASSERT_TRUE(!lets2.IntersectsWith(lets));
        ASSERT_TRUE(lets.IntersectsWith(symbols));
        ASSERT_TRUE(!lets.IntersectsWith(lets2));

        ASSERT_TRUE(nothing_map.IsZero());
        ASSERT_TRUE(!lets.IsZero());
    }

    namespace {
        std::string Members(const abel::strings_internal::Charmap &m) {
            std::string r;
            for (size_t i = 0; i < 256; ++i)
                if (m.contains(i)) r.push_back(i);
            return r;
        }

        std::string ClosedRangeString(unsigned char lo, unsigned char hi) {
            // Don't depend on lo<hi. Just increment until lo==hi.
            std::string s;
            while (true) {
                s.push_back(lo);
                if (lo == hi) break;
                ++lo;
            }
            return s;
        }

    }  // namespace

    TEST(Charmap, Constexpr) {
        constexpr abel::strings_internal::Charmap kEmpty = nothing_map;
        EXPECT_THAT(Members(kEmpty), "");
        constexpr abel::strings_internal::Charmap kA =
                abel::strings_internal::Charmap::Char('A');
        EXPECT_THAT(Members(kA), "A");
        constexpr abel::strings_internal::Charmap kAZ =
                abel::strings_internal::Charmap::Range('A', 'Z');
        EXPECT_THAT(Members(kAZ), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        constexpr abel::strings_internal::Charmap kIdentifier =
                abel::strings_internal::Charmap::Range('0', '9') |
                abel::strings_internal::Charmap::Range('A', 'Z') |
                abel::strings_internal::Charmap::Range('a', 'z') |
                abel::strings_internal::Charmap::Char('_');
        EXPECT_THAT(Members(kIdentifier),
                    "0123456789"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "_"
                    "abcdefghijklmnopqrstuvwxyz");
        constexpr abel::strings_internal::Charmap kAll = everything_map;
        for (size_t i = 0; i < 256; ++i) {
            EXPECT_TRUE(kAll.contains(i)) << i;
        }
        constexpr abel::strings_internal::Charmap kHello =
                abel::strings_internal::Charmap::FromString("Hello, world!");
        EXPECT_THAT(Members(kHello), " !,Hdelorw");

        // test negation and intersection
        constexpr abel::strings_internal::Charmap kABC =
                abel::strings_internal::Charmap::Range('A', 'Z') &
                ~abel::strings_internal::Charmap::Range('D', 'Z');
        EXPECT_THAT(Members(kABC), "ABC");
    }

    TEST(Charmap, Range) {
        // Exhaustive testing takes too long, so test some of the boundaries that
        // are perhaps going to cause trouble.
        std::vector<size_t> poi = {0, 1, 2, 3, 4, 7, 8, 9, 15,
                                   16, 17, 30, 31, 32, 33, 63, 64, 65,
                                   127, 128, 129, 223, 224, 225, 254, 255};
        for (auto lo = poi.begin(); lo != poi.end(); ++lo) {
            SCOPED_TRACE(*lo);
            for (auto hi = lo; hi != poi.end(); ++hi) {
                SCOPED_TRACE(*hi);
                EXPECT_THAT(Members(abel::strings_internal::Charmap::Range(*lo, *hi)),
                            ClosedRangeString(*lo, *hi));
            }
        }
    }

    bool AsBool(int x) { return static_cast<bool>(x); }

    TEST(CharmapCtype, Match) {
        for (int c = 0; c < 256; ++c) {
            SCOPED_TRACE(c);
            SCOPED_TRACE(static_cast<char>(c));
            EXPECT_EQ(AsBool(std::isupper(c)),
                      abel::strings_internal::UpperCharmap().contains(c));
            EXPECT_EQ(AsBool(std::islower(c)),
                      abel::strings_internal::LowerCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isdigit(c)),
                      abel::strings_internal::DigitCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isalpha(c)),
                      abel::strings_internal::AlphaCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isalnum(c)),
                      abel::strings_internal::AlnumCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isxdigit(c)),
                      abel::strings_internal::XDigitCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isprint(c)),
                      abel::strings_internal::PrintCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isspace(c)),
                      abel::strings_internal::SpaceCharmap().contains(c));
            EXPECT_EQ(AsBool(std::iscntrl(c)),
                      abel::strings_internal::CntrlCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isblank(c)),
                      abel::strings_internal::BlankCharmap().contains(c));
            EXPECT_EQ(AsBool(std::isgraph(c)),
                      abel::strings_internal::GraphCharmap().contains(c));
            EXPECT_EQ(AsBool(std::ispunct(c)),
                      abel::strings_internal::PunctCharmap().contains(c));
        }
    }

}  // namespace
