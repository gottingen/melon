// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/files/dir_reader_posix.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "melon/utility/logging.h"
#include <gtest/gtest.h>

#if defined(OS_ANDROID)
#include "melon/utility/os_compat_android.h"
#endif

namespace mutil {

TEST(DirReaderPosixUnittest, Read) {
  static const unsigned kNumFiles = 100;

  if (DirReaderPosix::IsFallback())
    return;

  char kDirTemplate[] = "/tmp/org.chromium.dir-reader-posix-XXXXXX";
  const char* dir = mkdtemp(kDirTemplate);
  ASSERT_TRUE(dir);

  const int prev_wd = open(".", O_RDONLY | O_DIRECTORY);
  DMCHECK_GE(prev_wd, 0);

  PMCHECK(chdir(dir) == 0);

  for (unsigned i = 0; i < kNumFiles; i++) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", i);
    const int fd = open(buf, O_CREAT | O_RDONLY | O_EXCL, 0600);
    PMCHECK(fd >= 0);
    PMCHECK(close(fd) == 0);
  }

  std::set<unsigned> seen;

  DirReaderPosix reader(dir);
  EXPECT_TRUE(reader.IsValid());

  if (!reader.IsValid())
    return;

  bool seen_dot = false, seen_dotdot = false;

  for (; reader.Next(); ) {
    if (strcmp(reader.name(), ".") == 0) {
      seen_dot = true;
      continue;
    }
    if (strcmp(reader.name(), "..") == 0) {
      seen_dotdot = true;
      continue;
    }

    SCOPED_TRACE(testing::Message() << "reader.name(): " << reader.name());

    char *endptr;
    const unsigned long value = strtoul(reader.name(), &endptr, 10);

    EXPECT_FALSE(*endptr);
    EXPECT_LT(value, kNumFiles);
    EXPECT_EQ(0u, seen.count(value));
    seen.insert(value);
  }

  for (unsigned i = 0; i < kNumFiles; i++) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", i);
    PMCHECK(unlink(buf) == 0);
  }

  PMCHECK(rmdir(dir) == 0);

  PMCHECK(fchdir(prev_wd) == 0);
  PMCHECK(close(prev_wd) == 0);

  EXPECT_TRUE(seen_dot);
  EXPECT_TRUE(seen_dotdot);
  EXPECT_EQ(kNumFiles, seen.size());
}

}  // namespace mutil
