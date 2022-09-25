
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_FILES_RANDOM_ACCESS_FILE_H_
#define MELON_FILES_RANDOM_ACCESS_FILE_H_

#include "melon/files/filesystem.h"
#include "melon/base/profile.h"
#include "melon/base/result_status.h"
#include "melon/io/cord_buf.h"
#include <fcntl.h>

namespace melon {

    class random_access_file {
    public:
        random_access_file() noexcept = default;

        ~random_access_file();

        result_status open(const melon::file_path &path) noexcept;

        result_status read(size_t n, off_t offset, std::string *content);

        result_status read(size_t n, off_t offset, melon::cord_buf *buf);

        result_status read(size_t n, off_t offset, char *buf);

        bool is_eof(off_t off, size_t has_read, result_status *frs);

        void close();

        const melon::file_path &path() const {
            return _path;
        }

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(random_access_file);
        melon::file_path _path;
        int _fd{-1};
    };

}  // namespace melon

#endif // MELON_FILES_RANDOM_ACCESS_FILE_H_
