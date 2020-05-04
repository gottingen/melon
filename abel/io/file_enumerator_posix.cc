// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <abel/io/file_enumerator.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <abel/log/abel_logging.h>

namespace abel {

// file_enumerator::enumerator_info ----------------------------------------------------

    file_enumerator::enumerator_info::enumerator_info() {
        memset(&stat_, 0, sizeof(stat_));
    }

    bool file_enumerator::enumerator_info::IsDirectory() const {
        return S_ISDIR(stat_.st_mode);
    }

    abel::filesystem::path file_enumerator::enumerator_info::get_name() const {
        return filename_;
    }

    int64_t file_enumerator::enumerator_info::get_size() const {
        return stat_.st_size;
    }

    abel::abel_time file_enumerator::enumerator_info::get_last_modified_time() const {
        return abel::from_unix_seconds(stat_.st_mtime);
    }

// file_enumerator --------------------------------------------------------------

    file_enumerator::file_enumerator(const abel::filesystem::path &root_path,
                                   bool recursive,
                                   int file_type)
            : current_directory_entry_(0),
              root_path_(root_path),
              recursive_(recursive),
              file_type_(file_type) {
        // INCLUDE_DOT_DOT must not be specified if recursive.
        ABEL_ASSERT(!(recursive && (INCLUDE_DOT_DOT & file_type_)));
        pending_paths_.push(root_path);
    }

    file_enumerator::file_enumerator(const abel::filesystem::path &root_path,
                                   bool recursive,
                                   int file_type,
                                   const std::string &pattern)
            : current_directory_entry_(0),
              root_path_(root_path),
              recursive_(recursive),
              file_type_(file_type),
              pattern_((root_path / pattern).generic_string()) {
        // INCLUDE_DOT_DOT must not be specified if recursive.
        ABEL_ASSERT(!(recursive && (INCLUDE_DOT_DOT & file_type_)));
        // The Windows version of this code appends the pattern to the root_path,
        // potentially only matching against items in the top-most directory.
        // Do the same here.
        if (pattern.empty())
            pattern_ = std::string();
        pending_paths_.push(root_path);
    }

    file_enumerator::~file_enumerator() {
    }

    abel::filesystem::path file_enumerator::Next() {
        ++current_directory_entry_;

        // While we've exhausted the entries in the current directory, do the next
        while (current_directory_entry_ >= directory_entries_.size()) {
            if (pending_paths_.empty())
                return abel::filesystem::path();

            root_path_ = pending_paths_.top();
            pending_paths_.pop();

            std::vector<enumerator_info> entries;
            if (!read_directory(&entries, root_path_, file_type_ & SHOW_SYM_LINKS))
                continue;

            directory_entries_.clear();
            current_directory_entry_ = 0;
            for (std::vector<enumerator_info>::const_iterator i = entries.begin();
                 i != entries.end(); ++i) {
                abel::filesystem::path full_path = root_path_ / i->filename_;
                if (should_skip(full_path))
                    continue;

                if (pattern_.size() &&
                    fnmatch(pattern_.c_str(), full_path.c_str(), FNM_NOESCAPE))
                    continue;

                if (recursive_ && S_ISDIR(i->stat_.st_mode))
                    pending_paths_.push(full_path);

                if ((S_ISDIR(i->stat_.st_mode) && (file_type_ & DIRECTORIES)) ||
                    (!S_ISDIR(i->stat_.st_mode) && (file_type_ & FILES)))
                    directory_entries_.push_back(*i);
            }
        }

        return root_path_ / directory_entries_[current_directory_entry_].filename_;
    }

    file_enumerator::enumerator_info file_enumerator::GetInfo() const {
        return directory_entries_[current_directory_entry_];
    }

    bool file_enumerator::read_directory(std::vector<enumerator_info> *entries,
                                       const abel::filesystem::path &source, bool show_links) {
        //qrpc::ThreadRestrictions::AssertIOAllowed();
        DIR *dir = opendir(source.c_str());
        if (!dir)
            return false;


        struct dirent *dent;
        // readdir_r is marked as deprecated since glibc 2.24.
        // Using readdir on _different_ DIR* object is already thread-safe in
        // most modern libc implementations.
#if defined(__GLIBC__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 24))
        while ((dent = readdir(dir))) {
#else
        struct dirent dent_buf;
        while (readdir_r(dir, &dent_buf, &dent) == 0 && dent) {
#endif
            enumerator_info info;
            info.filename_ = abel::filesystem::path(dent->d_name);

            abel::filesystem::path full_name = source / (dent->d_name);
            int ret;
            if (show_links)
                ret = lstat(full_name.c_str(), &info.stat_);
            else
                ret = stat(full_name.c_str(), &info.stat_);
            if (ret < 0) {
                // Print the stat() error message unless it was ENOENT and we're
                // following symlinks.
                if (!(errno == ENOENT && !show_links)) {
                    ABEL_RAW_ERROR("Couldn't stat {}", (source / (dent->d_name)).generic_string());
                }
                memset(&info.stat_, 0, sizeof(info.stat_));
            }
            entries->push_back(info);
        }

        closedir(dir);
        return true;
    }

}  // namespace abel
