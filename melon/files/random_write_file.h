
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_FILES_RANDOM_WRITE_FILE_H_
#define MELON_FILES_RANDOM_WRITE_FILE_H_

#include "melon/files/filesystem.h"
#include "melon/base/profile.h"
#include "melon/base/result_status.h"
#include "melon/io/cord_buf.h"
#include <fcntl.h>

namespace melon {

    class random_write_file {
    public:
        random_write_file() = default;

        ~random_write_file();

        result_status open(const melon::file_path &path, bool truncate = true) noexcept;

        result_status write(off_t offset, std::string_view content);

        result_status write(off_t offset, const void *content, size_t size);

        result_status write(off_t offset, const melon::cord_buf &data);

        void close();

        void flush();

        const melon::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        melon::file_path _path;
    };

}  // namespace melon

#endif  // MELON_FILES_RANDOM_WRITE_FILE_H_
