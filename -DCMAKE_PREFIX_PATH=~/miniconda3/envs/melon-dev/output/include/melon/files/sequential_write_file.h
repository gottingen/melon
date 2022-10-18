
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef MELON_FILES_SEQUENTIAL_WRITE_FILE_H_
#define MELON_FILES_SEQUENTIAL_WRITE_FILE_H_

#include "melon/files/filesystem.h"
#include "melon/base/profile.h"
#include "melon/base/result_status.h"
#include "melon/io/cord_buf.h"

namespace melon {

    class sequential_write_file {
    public:
        sequential_write_file() = default;

        ~sequential_write_file();

        result_status open(const melon::file_path &path, bool truncate = true) noexcept;

        result_status write(std::string_view content);

        result_status write(void *content, size_t n) {
            return write(std::string_view((char*)content, n));
        }

        result_status write(const melon::cord_buf &data);

        void flush();

        void close();

        void reset(size_t n = 0);

        size_t has_write() const {
            return _has_write;
        }

        const melon::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        melon::file_path _path;
        size_t _has_write;
    };
}  // namespace melon
#endif  // MELON_FILES_SEQUENTIAL_WRITE_FILE_H_
