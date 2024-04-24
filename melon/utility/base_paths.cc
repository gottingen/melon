// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/base_paths.h"

#include "melon/utility/file_util.h"
#include "melon/utility/files/file_path.h"
#include "melon/utility/path_service.h"

namespace mutil {

bool PathProvider(int key, FilePath* result) {
  // NOTE: DIR_CURRENT is a special case in PathService::Get

  switch (key) {
    case DIR_EXE:
      PathService::Get(FILE_EXE, result);
      *result = result->DirName();
      return true;
    case DIR_MODULE:
      PathService::Get(FILE_MODULE, result);
      *result = result->DirName();
      return true;
    case DIR_TEMP:
      if (!GetTempDir(result))
        return false;
      return true;
    case mutil::DIR_HOME:
      *result = GetHomeDir();
      return true;
    case DIR_TEST_DATA:
      if (!PathService::Get(DIR_SOURCE_ROOT, result))
        return false;
      *result = result->Append(FILE_PATH_LITERAL("base"));
      *result = result->Append(FILE_PATH_LITERAL("test"));
      *result = result->Append(FILE_PATH_LITERAL("data"));
      if (!PathExists(*result))  // We don't want to create this.
        return false;
      return true;
    default:
      return false;
  }
}

}  // namespace mutil
