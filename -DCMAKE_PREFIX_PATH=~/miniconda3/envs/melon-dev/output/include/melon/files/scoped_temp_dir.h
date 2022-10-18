
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FILES_SCOPED_TEMP_DIR_H_
#define MELON_FILES_SCOPED_TEMP_DIR_H_

// An object representing a temporary / scratch directory that should be cleaned
// up (recursively) when this object goes out of scope.  Note that since
// deletion occurs during the destructor, no further error handling is possible
// if the directory fails to be deleted.  As a result, deletion is not
// guaranteed by this class.
//
// Multiple calls to the methods which establish a temporary directory
// (create_unique_temp_dir, create_unique_temp_dir_under_path, and Set) must have
// intervening calls to Delete or Take, or the calls will fail.

#include "melon/files/filesystem.h"
#include "melon/base/profile.h"

namespace melon {

    bool create_new_temp_directory(const melon::file_path &prefix, melon::file_path *newPath);

    bool create_temporary_dir_in_dir(const melon::file_path &base, const std::string &prefix,
                                     melon::file_path *newPath);

    class MELON_EXPORT scoped_temp_dir {
    public:
        // No directory is owned/created initially.
        scoped_temp_dir();

        // Recursively delete path.
        ~scoped_temp_dir();

        // Creates a unique directory in TempPath, and takes ownership of it.
        // See file_util::CreateNewTemporaryDirectory.
        bool create_unique_temp_dir() MELON_WARN_UNUSED_RESULT;

        // Creates a unique directory under a given path, and takes ownership of it.
        bool create_unique_temp_dir_under_path(const melon::file_path &path) MELON_WARN_UNUSED_RESULT;

        // Takes ownership of directory at |path|, creating it if necessary.
        // Don't call multiple times unless Take() has been called first.
        bool set(const melon::file_path &path) MELON_WARN_UNUSED_RESULT;

        // Deletes the temporary directory wrapped by this object.
        bool remove() MELON_WARN_UNUSED_RESULT;

        // Caller takes ownership of the temporary directory so it won't be destroyed
        // when this object goes out of scope.
        melon::file_path take();

        const melon::file_path &path() const { return _path; }

        // Returns true if _path is non-empty and exists.
        bool is_valid() const;

        operator bool() const { return is_valid(); }

    private:
        melon::file_path _path;

        MELON_DISALLOW_COPY_AND_ASSIGN(scoped_temp_dir);
    };

}  // namespace melon

#endif  // MELON_FILES_SCOPED_TEMP_DIR_H_
