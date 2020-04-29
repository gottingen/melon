#ifndef ABEL_IO_FILE_ENUMERATOR_H_
#define ABEL_IO_FILE_ENUMERATOR_H_

#include <stack>
#include <vector>
#include <abel/asl/filesystem.h>
#include <abel/base/profile.h>
#include <abel/chrono/clock.h>

#if defined(ABEL_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(ABEL_PLATFORM_POSIX)

#include <sys/stat.h>
#include <unistd.h>

#endif

namespace abel {

    // A class for enumerating the files in a provided path. The order of the
    // results is not guaranteed.
    //
    // This is blocking. Do not use on critical threads.
    //
    // Example:
    //
    //   qrpc::file_enumerator enum(my_dir, false, qrpc::file_enumerator::FILES,
    //                             FILE_PATH_LITERAL("*.txt"));
    //   for (abel::filesystem::path name = enum.Next(); !name.empty(); name = enum.Next())
    //     ...
    class ABEL_EXPORT file_enumerator {
    public:
        // Note: copy & assign supported.
        class ABEL_EXPORT enumerator_info {
        public:
            enumerator_info();

            ~enumerator_info();

            bool IsDirectory() const;

            // The name of the file. This will not include any path information. This
            // is in constrast to the value returned by file_enumerator.Next() which
            // includes the |root_path| passed into the file_enumerator constructor.
            abel::filesystem::path get_name() const;

            int64_t get_size() const;

            abel::abel_time get_last_modified_time() const;

#if defined(ABEL_PLATFORM_WINDOWS)
            // Note that the cAlternateFileName (used to hold the "short" 8.3 name)
            // of the WIN32_FIND_DATA will be empty. Since we don't use short file
            // names, we tell Windows to omit it which speeds up the query slightly.
            const WIN32_FIND_DATA& find_data() const { return find_data_; }
#elif defined(ABEL_PLATFORM_POSIX)

            const struct stat &stat() const { return stat_; }

#endif

        private:
            friend class file_enumerator;

#if defined(ABEL_PLATFORM_WINDOWS)
            WIN32_FIND_DATA find_data_;
#elif defined(ABEL_PLATFORM_POSIX)
            struct stat stat_;
            abel::filesystem::path filename_;
#endif
        };

        enum enumerator_type {
            FILES = 1 << 0,
            DIRECTORIES = 1 << 1,
            INCLUDE_DOT_DOT = 1 << 2,
#if defined(ABEL_PLATFORM_POSIX)
            SHOW_SYM_LINKS = 1 << 4,
#endif
        };

        // |root_path| is the starting directory to search for. It may or may not end
        // in a slash.
        //
        // If |recursive| is true, this will enumerate all matches in any
        // subdirectories matched as well. It does a breadth-first search, so all
        // files in one directory will be returned before any files in a
        // subdirectory.
        //
        // |file_type|, a bit mask of enumerator_type, specifies whether the enumerator
        // should match files, directories, or both.
        //
        // |pattern| is an optional pattern for which files to match. This
        // works like shell globbing. For example, "*.txt" or "Foo???.doc".
        // However, be careful in specifying patterns that aren't cross platform
        // since the underlying code uses OS-specific matching routines.  In general,
        // Windows matching is less featureful than others, so test there first.
        // If unspecified, this will match all files.
        // NOTE: the pattern only matches the contents of root_path, not files in
        // recursive subdirectories.
        // TODO(erikkay): Fix the pattern matching to work at all levels.
        file_enumerator(const abel::filesystem::path &root_path,
                       bool recursive,
                       int file_type);

        file_enumerator(const abel::filesystem::path &root_path,
                       bool recursive,
                       int file_type,
                       const std::string &pattern);

        ~file_enumerator();

        // Returns the next file or an empty string if there are no more results.
        //
        // The returned path will incorporate the |root_path| passed in the
        // constructor: "<root_path>/file_name.txt". If the |root_path| is absolute,
        // then so will be the result of Next().
        abel::filesystem::path Next();

        // Write the file info into |info|.
        enumerator_info GetInfo() const;

    private:
        // Returns true if the given path should be skipped in enumeration.
        bool should_skip(const abel::filesystem::path &path);

#if defined(ABEL_PLATFORM_WINDOWS)
        // True when find_data_ is valid.
        bool has_find_data_;
        WIN32_FIND_DATA find_data_;
        HANDLE find_handle_;
#elif defined(ABEL_PLATFORM_POSIX)

        // Read the filenames in source into the vector of DirectoryEntryInfo's
        static bool read_directory(std::vector<enumerator_info> *entries,
                                  const abel::filesystem::path &source, bool show_links);

        // The files in the current directory
        std::vector<enumerator_info> directory_entries_;

        // The next entry to use from the directory_entries_ vector
        size_t current_directory_entry_;
#endif

        abel::filesystem::path root_path_;
        bool recursive_;
        int file_type_;
        std::string pattern_;  // Empty when we want to find everything.

        // A stack that keeps track of which subdirectories we still need to
        // enumerate in the breadth-first search.
        std::stack<abel::filesystem::path> pending_paths_;
        ABEL_NON_COPYABLE(file_enumerator);
    };

}  // namespace abel

#endif  // ABEL_IO_FILE_ENUMERATOR_H_
