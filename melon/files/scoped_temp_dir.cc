
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/files/scoped_temp_dir.h"
#include "melon/files/temp_file.h"
#include "melon/log/logging.h"

#define BASE_FILES_TEMP_DIR_PATTERN "temp_dir_XXXXXX"

namespace melon {

    static bool create_temporary_dir_in_dir_impl(const melon::file_path &base_dir,
                                                 const std::string &name_tmpl,
                                                 melon::file_path *new_dir);

    bool create_temporary_dir_in_dir_impl(const melon::file_path &base_dir,
                                          const std::string &name_tmpl,
                                          melon::file_path *new_dir) {
        MELON_DCHECK(name_tmpl.find("XXXXXX") != std::string::npos)
                            << "Directory name template must contain \"XXXXXX\".";

        melon::file_path sub_dir = base_dir;
        sub_dir /= name_tmpl;
        std::string sub_dir_string = sub_dir.generic_string();

        // this should be OK since mkdtemp just replaces characters in place
        char *buffer = const_cast<char *>(sub_dir_string.c_str());
        std::error_code ec;
        if(!melon::exists(sub_dir, ec)) {
            if(!melon::create_directories(sub_dir, ec)) {
                return false;
            }
        }
        char *dtemp = mkdtemp(buffer);
        if (!dtemp) {
            MELON_DPLOG(ERROR) << "mkdtemp";
            return false;
        }
        *new_dir = melon::file_path(dtemp);
        return true;
    }

    bool create_new_temp_directory(const melon::file_path &prefix, melon::file_path *newPath) {
        melon::file_path tmp_dir;
        if (prefix.empty()) {
            return false;
        }
        if (prefix.is_absolute()) {
            tmp_dir = prefix;
        } else {
            std::error_code ec;
            tmp_dir = melon::temp_directory_path(ec);
            if (ec) {
                return false;
            }
            tmp_dir /= prefix;
        }
        return create_temporary_dir_in_dir_impl(tmp_dir, BASE_FILES_TEMP_DIR_PATTERN, newPath);
    }

    bool create_temporary_dir_in_dir(const melon::file_path &base, const std::string &prefix,
                                     melon::file_path *newPath) {
        std::string mkdtemp_template = prefix + "XXXXXX";
        return create_temporary_dir_in_dir_impl(base, mkdtemp_template, newPath);
    }

    scoped_temp_dir::scoped_temp_dir() {
    }

    scoped_temp_dir::~scoped_temp_dir() {
        if (!_path.empty() && !remove())
            MELON_DLOG(WARNING) << "Could not delete temp dir in dtor.";
    }

    bool scoped_temp_dir::create_unique_temp_dir() {
        if (!_path.empty())
            return false;
        // This "scoped_dir" prefix is only used on Windows and serves as a template
        // for the unique name.
        static melon::file_path kScopedDir("scoped_dir");
        if (!create_new_temp_directory(kScopedDir, &_path))
            return false;

        return true;
    }

    bool scoped_temp_dir::create_unique_temp_dir_under_path(const melon::file_path &base_path) {
        if (!_path.empty())
            return false;

        // If |base_path| does not exist, create it.
        std::error_code ec;
        if (!melon::exists(base_path, ec) && !melon::create_directories(base_path, ec))
            return false;

        // Create a new, uniquely named directory under |base_path|.
        if (!create_temporary_dir_in_dir(base_path, "scoped_dir_",
                                         &_path))
            return false;

        return true;
    }

    bool scoped_temp_dir::set(const melon::file_path &path) {
        if (!_path.empty())
            return false;

        std::error_code ec;
        if (!melon::exists(path, ec) && !melon::create_directories(path, ec))
            return false;

        _path = path;
        return true;
    }

    bool scoped_temp_dir::remove() {
        if (_path.empty())
            return false;
        std::error_code ec;
        bool ret = melon::remove_all(_path, ec);
        if (ret) {
            // We only clear the path if deleted the directory.
            _path.clear();
        }

        return ret;
    }

    melon::file_path scoped_temp_dir::take() {
        melon::file_path ret = _path;
        _path = melon::file_path();
        return ret;
    }

    bool scoped_temp_dir::is_valid() const {
        std::error_code ec;
        return !_path.empty() && melon::exists(_path, ec) && melon::is_directory(_path, ec);
    }

}  // namespace melon
