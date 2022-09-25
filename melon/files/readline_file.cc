
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "melon/files/readline_file.h"
#include "melon/files/sequential_read_file.h"
#include "melon/base/errno.h"
#include "melon/strings/str_split.h"
#include "melon/strings/utility.h"

namespace melon {

    result_status readline_file::open(const melon::file_path &path,  readline_option option) {
        MELON_CHECK(_path.empty()) << "do not reopen";
        sequential_read_file file;
        _path = path;
        _status = file.open(_path);
        if (!_status.is_ok()) {
            MELON_LOG(ERROR) << "open file :"<<_path<<" eroor "<< melon_error();
            return _status;
        }

        _status = file.read(&_content);
        if (!_status.is_ok()) {
            MELON_LOG(ERROR) << "read file :"<<_path<<" eroor "<< melon_error();
            return _status;
        }
        if(option == readline_option::eSkipEmptyLine) {
            _lines = melon::string_split(_content, melon::by_any_char("\n"), melon::skip_empty());
        } else if(option == readline_option::eTrimWhitespace) {
            _lines = melon::string_split(_content, melon::by_any_char("\n"), melon::skip_whitespace());
        } else {
            _lines = melon::string_split(_content, melon::by_any_char("\n"));
            _lines.pop_back();
        }
        for(size_t i = 0; i < _lines.size(); i++) {
            if(_lines[i].empty() && melon::back_char(_lines[i]) == '\r') {
                _lines[i].remove_suffix(1);
            }

        }
        return _status;
    }

}  // namespace melon