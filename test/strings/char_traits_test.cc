
#include <abel/strings/internal/char_traits.h>
#include <cstdlib>
#include <gtest/gtest.h>
#include <abel/strings/ascii.h>

namespace {
/*
static char* memcasechr(const char* s, int c, size_t slen) {
  c = abel::ascii_tolower(c);
  for (; slen; ++s, --slen) {
    if (abel::ascii_tolower(*s) == c) return const_cast<char*>(s);
  }
  return nullptr;
}
*/
/*
static const char* memcasematch(const char* phaystack, size_t haylen,
                                const char* pneedle, size_t neelen) {
  if (0 == neelen) {
    return phaystack;  // even if haylen is 0
  }
  if (haylen < neelen) return nullptr;

  const char* match;
  const char* hayend = phaystack + haylen - neelen + 1;
  while ((match = static_cast<char*>(
              memcasechr(phaystack, pneedle[0], hayend - phaystack)))) {
    if (abel::strings_internal::memcasecmp(match, pneedle, neelen) == 0)
      return match;
    else
      phaystack = match + 1;
  }
  return nullptr;
}*/

    TEST(CharTraitsTest, AllTests) {
        char a[1000];
        abel::strings_internal::char_cat(a, 0, "hello", sizeof("hello") - 1);
        abel::strings_internal::char_cat(a, 5, " there", sizeof(" there") - 1);

        EXPECT_EQ(abel::strings_internal::char_case_cmp(a, "heLLO there",
                                                        sizeof("hello there") - 1),
                  0);
        EXPECT_EQ(abel::strings_internal::char_case_cmp(a, "heLLO therf",
                                                        sizeof("hello there") - 1),
                  -1);
        EXPECT_EQ(abel::strings_internal::char_case_cmp(a, "heLLO therf",
                                                        sizeof("hello there") - 2),
                  0);
        EXPECT_EQ(abel::strings_internal::char_case_cmp(a, "whatever", 0), 0);

        char *p = abel::strings_internal::char_dup("hello", 5);
        free(p);

        p = abel::strings_internal::char_rchr("hello there", 'e',
                                              sizeof("hello there") - 1);
        EXPECT_TRUE(p && p[-1] == 'r');
        p = abel::strings_internal::char_rchr("hello there", 'e',
                                              sizeof("hello there") - 2);
        EXPECT_TRUE(p && p[-1] == 'h');
        p = abel::strings_internal::char_rchr("hello there", 'u',
                                              sizeof("hello there") - 1);
        EXPECT_TRUE(p == nullptr);

        int len = abel::strings_internal::char_spn("hello there",
                                                   sizeof("hello there") - 1, "hole");
        EXPECT_EQ(len, sizeof("hello") - 1);
        len = abel::strings_internal::char_spn("hello there", sizeof("hello there") - 1,
                                               "u");
        EXPECT_EQ(len, 0);
        len = abel::strings_internal::char_spn("hello there", sizeof("hello there") - 1,
                                               "");
        EXPECT_EQ(len, 0);
        len = abel::strings_internal::char_spn("hello there", sizeof("hello there") - 1,
                                               "trole h");
        EXPECT_EQ(len, sizeof("hello there") - 1);
        len = abel::strings_internal::char_spn("hello there!",
                                               sizeof("hello there!") - 1, "trole h");
        EXPECT_EQ(len, sizeof("hello there") - 1);
        len = abel::strings_internal::char_spn("hello there!",
                                               sizeof("hello there!") - 2, "trole h!");
        EXPECT_EQ(len, sizeof("hello there!") - 2);

        len = abel::strings_internal::char_cspn("hello there",
                                                sizeof("hello there") - 1, "leho");
        EXPECT_EQ(len, 0);
        len = abel::strings_internal::char_cspn("hello there",
                                                sizeof("hello there") - 1, "u");
        EXPECT_EQ(len, sizeof("hello there") - 1);
        len = abel::strings_internal::char_cspn("hello there",
                                                sizeof("hello there") - 1, "");
        EXPECT_EQ(len, sizeof("hello there") - 1);
        len = abel::strings_internal::char_cspn("hello there",
                                                sizeof("hello there") - 1, " ");
        EXPECT_EQ(len, 5);

        p = abel::strings_internal::char_pbrk("hello there", sizeof("hello there") - 1,
                                              "leho");
        EXPECT_TRUE(p && p[1] == 'e' && p[2] == 'l');
        p = abel::strings_internal::char_pbrk("hello there", sizeof("hello there") - 1,
                                              "nu");
        EXPECT_TRUE(p == nullptr);
        p = abel::strings_internal::char_pbrk("hello there!",
                                              sizeof("hello there!") - 2, "!");
        EXPECT_TRUE(p == nullptr);
        p = abel::strings_internal::char_pbrk("hello there", sizeof("hello there") - 1,
                                              " t ");
        EXPECT_TRUE(p && p[-1] == 'o' && p[1] == 't');

        {
            const char kHaystack[] = "0123456789";
            EXPECT_EQ(abel::strings_internal::char_mem(kHaystack, 0, "", 0), kHaystack);
            EXPECT_EQ(abel::strings_internal::char_mem(kHaystack, 10, "012", 3),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_mem(kHaystack, 10, "0xx", 1),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_mem(kHaystack, 10, "789", 3),
                      kHaystack + 7);
            EXPECT_EQ(abel::strings_internal::char_mem(kHaystack, 10, "9xx", 1),
                      kHaystack + 9);
            EXPECT_TRUE(abel::strings_internal::char_mem(kHaystack, 10, "9xx", 3) ==
                        nullptr);
            EXPECT_TRUE(abel::strings_internal::char_mem(kHaystack, 10, "xxx", 1) ==
                        nullptr);
        }
        {
            const char kHaystack[] = "aBcDeFgHiJ";
            EXPECT_EQ(abel::strings_internal::char_case_mem(kHaystack, 0, "", 0),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_case_mem(kHaystack, 10, "Abc", 3),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_case_mem(kHaystack, 10, "Axx", 1),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_case_mem(kHaystack, 10, "hIj", 3),
                      kHaystack + 7);
            EXPECT_EQ(abel::strings_internal::char_case_mem(kHaystack, 10, "jxx", 1),
                      kHaystack + 9);
            EXPECT_TRUE(abel::strings_internal::char_case_mem(kHaystack, 10, "jxx", 3) ==
                        nullptr);
            EXPECT_TRUE(abel::strings_internal::char_case_mem(kHaystack, 10, "xxx", 1) ==
                        nullptr);
        }
        {
            const char kHaystack[] = "0123456789";
            EXPECT_EQ(abel::strings_internal::char_match(kHaystack, 0, "", 0), kHaystack);
            EXPECT_EQ(abel::strings_internal::char_match(kHaystack, 10, "012", 3),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_match(kHaystack, 10, "0xx", 1),
                      kHaystack);
            EXPECT_EQ(abel::strings_internal::char_match(kHaystack, 10, "789", 3),
                      kHaystack + 7);
            EXPECT_EQ(abel::strings_internal::char_match(kHaystack, 10, "9xx", 1),
                      kHaystack + 9);
            EXPECT_TRUE(abel::strings_internal::char_match(kHaystack, 10, "9xx", 3) ==
                        nullptr);
            EXPECT_TRUE(abel::strings_internal::char_match(kHaystack, 10, "xxx", 1) ==
                        nullptr);
        }
    }

}  // namespace
