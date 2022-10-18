
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef MELON_FILES_SEQUENTIAL_READ_FILE_H_
#define MELON_FILES_SEQUENTIAL_READ_FILE_H_

#include "melon/files/filesystem.h"
#include "melon/base/profile.h"
#include "melon/base/result_status.h"
#include "melon/io/cord_buf.h"

namespace melon {

    class sequential_read_file {
    public:
        sequential_read_file() noexcept = default;

        ~sequential_read_file();

        result_status open(const melon::file_path &path) noexcept;

        result_status read(std::string *content, size_t n = npos);

        result_status read(melon::cord_buf *buf, size_t n = npos);

        std::pair<result_status, size_t> read(void *buf, size_t n);

        result_status skip(size_t n);

        bool is_eof(result_status *frs);

        void close();

        void reset();

        size_t has_read() const {
            return _has_read;
        }

        const melon::file_path &path() const {
            return _path;
        }

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(sequential_read_file);

        static const size_t npos = std::numeric_limits<size_t>::max();
        int _fd{-1};
        melon::file_path _path;
        size_t _has_read{0};
    };

}  // namespace melon

#endif  // MELON_FILES_SEQUENTIAL_READ_FILE_H_
