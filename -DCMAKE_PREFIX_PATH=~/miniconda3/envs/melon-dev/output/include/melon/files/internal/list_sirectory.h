
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_FILES_INTERNAL_LIST_SIRECTORY_H_
#define MELON_FILES_INTERNAL_LIST_SIRECTORY_H_

#include <vector>
#include <string_view>
#include "melon/files/filesystem.h"

namespace melon::files_internal {
    std::vector<file_path> list_directory_internal(std::string_view path, bool recursive, std::error_code &ec);
}
#endif  // MELON_FILES_INTERNAL_LIST_SIRECTORY_H_
