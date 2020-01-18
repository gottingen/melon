//

#include <abel/strings/internal/resize_uninitialized.h>

#include <gtest/gtest.h>

namespace {

int resize_call_count = 0;

struct resizable_string {
  void resize(size_t) { resize_call_count += 1; }
};

int resize_default_init_call_count = 0;

struct resize_default_init_string {
  void resize(size_t) { resize_call_count += 1; }
  void __resize_default_init(size_t) { resize_default_init_call_count += 1; }
};

TEST(ResizeUninit, WithAndWithout) {
  resize_call_count = 0;
  resize_default_init_call_count = 0;
  {
    resizable_string rs;

    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_FALSE(
        abel::strings_internal::STLStringSupportsNontrashingResize(&rs));
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    abel::strings_internal::STLStringResizeUninitialized(&rs, 237);
    EXPECT_EQ(resize_call_count, 1);
    EXPECT_EQ(resize_default_init_call_count, 0);
  }

  resize_call_count = 0;
  resize_default_init_call_count = 0;
  {
    resize_default_init_string rus;

    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_TRUE(
        abel::strings_internal::STLStringSupportsNontrashingResize(&rus));
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    abel::strings_internal::STLStringResizeUninitialized(&rus, 237);
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 1);
  }
}

}  // namespace
