// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/file_util.h"
#include "melon/utility/files/file.h"
#include "melon/utility/files/scoped_temp_dir.h"
#include "melon/utility/time/time.h"
#include <gtest/gtest.h>

using mutil::File;
using mutil::FilePath;

TEST(FileTest, Create) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("create_file_1");

  {
    // Don't create a File at all.
    File file;
    EXPECT_FALSE(file.IsValid());
    EXPECT_EQ(mutil::File::FILE_ERROR_FAILED, file.error_details());

    File file2(mutil::File::FILE_ERROR_TOO_MANY_OPENED);
    EXPECT_FALSE(file2.IsValid());
    EXPECT_EQ(mutil::File::FILE_ERROR_TOO_MANY_OPENED, file2.error_details());
  }

  {
    // Open a file that doesn't exist.
    File file(file_path, mutil::File::FLAG_OPEN | mutil::File::FLAG_READ);
    EXPECT_FALSE(file.IsValid());
    EXPECT_EQ(mutil::File::FILE_ERROR_NOT_FOUND, file.error_details());
  }

  {
    // Open or create a file.
    File file(file_path, mutil::File::FLAG_OPEN_ALWAYS | mutil::File::FLAG_READ);
    EXPECT_TRUE(file.IsValid());
    EXPECT_TRUE(file.created());
    EXPECT_EQ(mutil::File::FILE_OK, file.error_details());
  }

  {
    // Open an existing file.
    File file(file_path, mutil::File::FLAG_OPEN | mutil::File::FLAG_READ);
    EXPECT_TRUE(file.IsValid());
    EXPECT_FALSE(file.created());
    EXPECT_EQ(mutil::File::FILE_OK, file.error_details());

    // This time verify closing the file.
    file.Close();
    EXPECT_FALSE(file.IsValid());
  }

  {
    // Open an existing file through Initialize
    File file;
    file.Initialize(file_path, mutil::File::FLAG_OPEN | mutil::File::FLAG_READ);
    EXPECT_TRUE(file.IsValid());
    EXPECT_FALSE(file.created());
    EXPECT_EQ(mutil::File::FILE_OK, file.error_details());

    // This time verify closing the file.
    file.Close();
    EXPECT_FALSE(file.IsValid());
  }

  {
    // Create a file that exists.
    File file(file_path, mutil::File::FLAG_CREATE | mutil::File::FLAG_READ);
    EXPECT_FALSE(file.IsValid());
    EXPECT_FALSE(file.created());
    EXPECT_EQ(mutil::File::FILE_ERROR_EXISTS, file.error_details());
  }

  {
    // Create or overwrite a file.
    File file(file_path,
              mutil::File::FLAG_CREATE_ALWAYS | mutil::File::FLAG_WRITE);
    EXPECT_TRUE(file.IsValid());
    EXPECT_TRUE(file.created());
    EXPECT_EQ(mutil::File::FILE_OK, file.error_details());
  }

  {
    // Create a delete-on-close file.
    file_path = temp_dir.path().AppendASCII("create_file_2");
    File file(file_path,
              mutil::File::FLAG_OPEN_ALWAYS | mutil::File::FLAG_READ |
                  mutil::File::FLAG_DELETE_ON_CLOSE);
    EXPECT_TRUE(file.IsValid());
    EXPECT_TRUE(file.created());
    EXPECT_EQ(mutil::File::FILE_OK, file.error_details());
  }

  EXPECT_FALSE(mutil::PathExists(file_path));
}

TEST(FileTest, Async) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("create_file");

  {
    File file(file_path, mutil::File::FLAG_OPEN_ALWAYS | mutil::File::FLAG_ASYNC);
    EXPECT_TRUE(file.IsValid());
    EXPECT_TRUE(file.async());
  }

  {
    File file(file_path, mutil::File::FLAG_OPEN_ALWAYS);
    EXPECT_TRUE(file.IsValid());
    EXPECT_FALSE(file.async());
  }
}

TEST(FileTest, DeleteOpenFile) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("create_file_1");

  // Create a file.
  File file(file_path,
            mutil::File::FLAG_OPEN_ALWAYS | mutil::File::FLAG_READ |
                mutil::File::FLAG_SHARE_DELETE);
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(file.created());
  EXPECT_EQ(mutil::File::FILE_OK, file.error_details());

  // Open an existing file and mark it as delete on close.
  File same_file(file_path,
                 mutil::File::FLAG_OPEN | mutil::File::FLAG_DELETE_ON_CLOSE |
                     mutil::File::FLAG_READ);
  EXPECT_TRUE(file.IsValid());
  EXPECT_FALSE(same_file.created());
  EXPECT_EQ(mutil::File::FILE_OK, same_file.error_details());

  // Close both handles and check that the file is gone.
  file.Close();
  same_file.Close();
  EXPECT_FALSE(mutil::PathExists(file_path));
}

TEST(FileTest, ReadWrite) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("read_write_file");
  File file(file_path,
            mutil::File::FLAG_CREATE | mutil::File::FLAG_READ |
                mutil::File::FLAG_WRITE);
  ASSERT_TRUE(file.IsValid());

  char data_to_write[] = "test";
  const int kTestDataSize = 4;

  // Write 0 bytes to the file.
  int bytes_written = file.Write(0, data_to_write, 0);
  EXPECT_EQ(0, bytes_written);

  // Write "test" to the file.
  bytes_written = file.Write(0, data_to_write, kTestDataSize);
  EXPECT_EQ(kTestDataSize, bytes_written);

  // Read from EOF.
  char data_read_1[32];
  int bytes_read = file.Read(kTestDataSize, data_read_1, kTestDataSize);
  EXPECT_EQ(0, bytes_read);

  // Read from somewhere in the middle of the file.
  const int kPartialReadOffset = 1;
  bytes_read = file.Read(kPartialReadOffset, data_read_1, kTestDataSize);
  EXPECT_EQ(kTestDataSize - kPartialReadOffset, bytes_read);
  for (int i = 0; i < bytes_read; i++)
    EXPECT_EQ(data_to_write[i + kPartialReadOffset], data_read_1[i]);

  // Read 0 bytes.
  bytes_read = file.Read(0, data_read_1, 0);
  EXPECT_EQ(0, bytes_read);

  // Read the entire file.
  bytes_read = file.Read(0, data_read_1, kTestDataSize);
  EXPECT_EQ(kTestDataSize, bytes_read);
  for (int i = 0; i < bytes_read; i++)
    EXPECT_EQ(data_to_write[i], data_read_1[i]);

  // Read again, but using the trivial native wrapper.
  bytes_read = file.ReadNoBestEffort(0, data_read_1, kTestDataSize);
  EXPECT_LE(bytes_read, kTestDataSize);
  for (int i = 0; i < bytes_read; i++)
    EXPECT_EQ(data_to_write[i], data_read_1[i]);

  // Write past the end of the file.
  const int kOffsetBeyondEndOfFile = 10;
  const int kPartialWriteLength = 2;
  bytes_written = file.Write(kOffsetBeyondEndOfFile,
                             data_to_write, kPartialWriteLength);
  EXPECT_EQ(kPartialWriteLength, bytes_written);

  // Make sure the file was extended.
  int64_t file_size = 0;
  EXPECT_TRUE(GetFileSize(file_path, &file_size));
  EXPECT_EQ(kOffsetBeyondEndOfFile + kPartialWriteLength, file_size);

  // Make sure the file was zero-padded.
  char data_read_2[32];
  bytes_read = file.Read(0, data_read_2, static_cast<int>(file_size));
  EXPECT_EQ(file_size, bytes_read);
  for (int i = 0; i < kTestDataSize; i++)
    EXPECT_EQ(data_to_write[i], data_read_2[i]);
  for (int i = kTestDataSize; i < kOffsetBeyondEndOfFile; i++)
    EXPECT_EQ(0, data_read_2[i]);
  for (int i = kOffsetBeyondEndOfFile; i < file_size; i++)
    EXPECT_EQ(data_to_write[i - kOffsetBeyondEndOfFile], data_read_2[i]);
}

TEST(FileTest, Append) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("append_file");
  File file(file_path, mutil::File::FLAG_CREATE | mutil::File::FLAG_APPEND);
  ASSERT_TRUE(file.IsValid());

  char data_to_write[] = "test";
  const int kTestDataSize = 4;

  // Write 0 bytes to the file.
  int bytes_written = file.Write(0, data_to_write, 0);
  EXPECT_EQ(0, bytes_written);

  // Write "test" to the file.
  bytes_written = file.Write(0, data_to_write, kTestDataSize);
  EXPECT_EQ(kTestDataSize, bytes_written);

  file.Close();
  File file2(file_path,
             mutil::File::FLAG_OPEN | mutil::File::FLAG_READ |
                 mutil::File::FLAG_APPEND);
  ASSERT_TRUE(file2.IsValid());

  // Test passing the file around.
  file = file2.Pass();
  EXPECT_FALSE(file2.IsValid());
  ASSERT_TRUE(file.IsValid());

  char append_data_to_write[] = "78";
  const int kAppendDataSize = 2;

  // Append "78" to the file.
  bytes_written = file.Write(0, append_data_to_write, kAppendDataSize);
  EXPECT_EQ(kAppendDataSize, bytes_written);

  // Read the entire file.
  char data_read_1[32];
  int bytes_read = file.Read(0, data_read_1,
                             kTestDataSize + kAppendDataSize);
  EXPECT_EQ(kTestDataSize + kAppendDataSize, bytes_read);
  for (int i = 0; i < kTestDataSize; i++)
    EXPECT_EQ(data_to_write[i], data_read_1[i]);
  for (int i = 0; i < kAppendDataSize; i++)
    EXPECT_EQ(append_data_to_write[i], data_read_1[kTestDataSize + i]);
}


TEST(FileTest, Length) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("truncate_file");
  File file(file_path,
            mutil::File::FLAG_CREATE | mutil::File::FLAG_READ |
                mutil::File::FLAG_WRITE);
  ASSERT_TRUE(file.IsValid());
  EXPECT_EQ(0, file.GetLength());

  // Write "test" to the file.
  char data_to_write[] = "test";
  int kTestDataSize = 4;
  int bytes_written = file.Write(0, data_to_write, kTestDataSize);
  EXPECT_EQ(kTestDataSize, bytes_written);

  // Extend the file.
  const int kExtendedFileLength = 10;
  int64_t file_size = 0;
  EXPECT_TRUE(file.SetLength(kExtendedFileLength));
  EXPECT_EQ(kExtendedFileLength, file.GetLength());
  EXPECT_TRUE(GetFileSize(file_path, &file_size));
  EXPECT_EQ(kExtendedFileLength, file_size);

  // Make sure the file was zero-padded.
  char data_read[32];
  int bytes_read = file.Read(0, data_read, static_cast<int>(file_size));
  EXPECT_EQ(file_size, bytes_read);
  for (int i = 0; i < kTestDataSize; i++)
    EXPECT_EQ(data_to_write[i], data_read[i]);
  for (int i = kTestDataSize; i < file_size; i++)
    EXPECT_EQ(0, data_read[i]);

  // Truncate the file.
  const int kTruncatedFileLength = 2;
  EXPECT_TRUE(file.SetLength(kTruncatedFileLength));
  EXPECT_EQ(kTruncatedFileLength, file.GetLength());
  EXPECT_TRUE(GetFileSize(file_path, &file_size));
  EXPECT_EQ(kTruncatedFileLength, file_size);

  // Make sure the file was truncated.
  bytes_read = file.Read(0, data_read, kTestDataSize);
  EXPECT_EQ(file_size, bytes_read);
  for (int i = 0; i < file_size; i++)
    EXPECT_EQ(data_to_write[i], data_read[i]);
}

// Flakily fails: http://crbug.com/86494
#if defined(OS_ANDROID)
TEST(FileTest, TouchGetInfo) {
#else
TEST(FileTest, DISABLED_TouchGetInfo) {
#endif
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  File file(temp_dir.path().AppendASCII("touch_get_info_file"),
            mutil::File::FLAG_CREATE | mutil::File::FLAG_WRITE |
                mutil::File::FLAG_WRITE_ATTRIBUTES);
  ASSERT_TRUE(file.IsValid());

  // Get info for a newly created file.
  mutil::File::Info info;
  EXPECT_TRUE(file.GetInfo(&info));

  // Add 2 seconds to account for possible rounding errors on
  // filesystems that use a 1s or 2s timestamp granularity.
  mutil::Time now = mutil::Time::Now() + mutil::TimeDelta::FromSeconds(2);
  EXPECT_EQ(0, info.size);
  EXPECT_FALSE(info.is_directory);
  EXPECT_FALSE(info.is_symbolic_link);
  EXPECT_LE(info.last_accessed.ToInternalValue(), now.ToInternalValue());
  EXPECT_LE(info.last_modified.ToInternalValue(), now.ToInternalValue());
  EXPECT_LE(info.creation_time.ToInternalValue(), now.ToInternalValue());
  mutil::Time creation_time = info.creation_time;

  // Write "test" to the file.
  char data[] = "test";
  const int kTestDataSize = 4;
  int bytes_written = file.Write(0, data, kTestDataSize);
  EXPECT_EQ(kTestDataSize, bytes_written);

  // Change the last_accessed and last_modified dates.
  // It's best to add values that are multiples of 2 (in seconds)
  // to the current last_accessed and last_modified times, because
  // FATxx uses a 2s timestamp granularity.
  mutil::Time new_last_accessed =
      info.last_accessed + mutil::TimeDelta::FromSeconds(234);
  mutil::Time new_last_modified =
      info.last_modified + mutil::TimeDelta::FromMinutes(567);

  EXPECT_TRUE(file.SetTimes(new_last_accessed, new_last_modified));

  // Make sure the file info was updated accordingly.
  EXPECT_TRUE(file.GetInfo(&info));
  EXPECT_EQ(info.size, kTestDataSize);
  EXPECT_FALSE(info.is_directory);
  EXPECT_FALSE(info.is_symbolic_link);

  // ext2/ext3 and HPS/HPS+ seem to have a timestamp granularity of 1s.
#if defined(OS_POSIX)
  EXPECT_EQ(info.last_accessed.ToTimeVal().tv_sec,
            new_last_accessed.ToTimeVal().tv_sec);
  EXPECT_EQ(info.last_modified.ToTimeVal().tv_sec,
            new_last_modified.ToTimeVal().tv_sec);
#else
  EXPECT_EQ(info.last_accessed.ToInternalValue(),
            new_last_accessed.ToInternalValue());
  EXPECT_EQ(info.last_modified.ToInternalValue(),
            new_last_modified.ToInternalValue());
#endif

  EXPECT_EQ(info.creation_time.ToInternalValue(),
            creation_time.ToInternalValue());
}

TEST(FileTest, ReadAtCurrentPosition) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("read_at_current_position");
  File file(file_path,
            mutil::File::FLAG_CREATE | mutil::File::FLAG_READ |
                mutil::File::FLAG_WRITE);
  EXPECT_TRUE(file.IsValid());

  const char kData[] = "test";
  const int kDataSize = sizeof(kData) - 1;
  EXPECT_EQ(kDataSize, file.Write(0, kData, kDataSize));

  EXPECT_EQ(0, file.Seek(mutil::File::FROM_BEGIN, 0));

  char buffer[kDataSize];
  int first_chunk_size = kDataSize / 2;
  EXPECT_EQ(first_chunk_size, file.ReadAtCurrentPos(buffer, first_chunk_size));
  EXPECT_EQ(kDataSize - first_chunk_size,
            file.ReadAtCurrentPos(buffer + first_chunk_size,
                                  kDataSize - first_chunk_size));
  EXPECT_EQ(std::string(buffer, buffer + kDataSize), std::string(kData));
}

TEST(FileTest, WriteAtCurrentPosition) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("write_at_current_position");
  File file(file_path,
            mutil::File::FLAG_CREATE | mutil::File::FLAG_READ |
                mutil::File::FLAG_WRITE);
  EXPECT_TRUE(file.IsValid());

  const char kData[] = "test";
  const int kDataSize = sizeof(kData) - 1;

  int first_chunk_size = kDataSize / 2;
  EXPECT_EQ(first_chunk_size, file.WriteAtCurrentPos(kData, first_chunk_size));
  EXPECT_EQ(kDataSize - first_chunk_size,
            file.WriteAtCurrentPos(kData + first_chunk_size,
                                   kDataSize - first_chunk_size));

  char buffer[kDataSize];
  EXPECT_EQ(kDataSize, file.Read(0, buffer, kDataSize));
  EXPECT_EQ(std::string(buffer, buffer + kDataSize), std::string(kData));
}

TEST(FileTest, Seek) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath file_path = temp_dir.path().AppendASCII("seek_file");
  File file(file_path,
            mutil::File::FLAG_CREATE | mutil::File::FLAG_READ |
                mutil::File::FLAG_WRITE);
  ASSERT_TRUE(file.IsValid());

  const int64_t kOffset = 10;
  EXPECT_EQ(kOffset, file.Seek(mutil::File::FROM_BEGIN, kOffset));
  EXPECT_EQ(2 * kOffset, file.Seek(mutil::File::FROM_CURRENT, kOffset));
  EXPECT_EQ(kOffset, file.Seek(mutil::File::FROM_CURRENT, -kOffset));
  EXPECT_TRUE(file.SetLength(kOffset * 2));
  EXPECT_EQ(kOffset, file.Seek(mutil::File::FROM_END, -kOffset));
}

#if defined(OS_WIN)
TEST(FileTest, GetInfoForDirectory) {
  mutil::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  FilePath empty_dir = temp_dir.path().Append(FILE_PATH_LITERAL("gpfi_test"));
  ASSERT_TRUE(CreateDirectory(empty_dir));

  mutil::File dir(
      ::CreateFile(empty_dir.value().c_str(),
                   FILE_ALL_ACCESS,
                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                   NULL,
                   OPEN_EXISTING,
                   FILE_FLAG_BACKUP_SEMANTICS,  // Needed to open a directory.
                   NULL));
  ASSERT_TRUE(dir.IsValid());

  mutil::File::Info info;
  EXPECT_TRUE(dir.GetInfo(&info));
  EXPECT_TRUE(info.is_directory);
  EXPECT_FALSE(info.is_symbolic_link);
  EXPECT_EQ(0, info.size);
}
#endif  // defined(OS_WIN)