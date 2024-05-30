//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#pragma once
#include <fcntl.h>
#include <melon/utility/file_util.h>
#include <melon/utility/files/file.h>                        // mutil::File
#include <melon/utility/files/dir_reader_posix.h>            // mutil::DirReaderPosix
#include <melon/utility/memory/ref_counted.h>                // mutil::RefCountedThreadSafe
#include <melon/utility/memory/singleton.h>                  // Singleton
#include <google/protobuf/message.h>                // google::protobuf::Message
#include <melon/raft/util.h>
#include <melon/raft/fsync.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC   02000000    /*  define close_on_exec if not defined in fcntl.h*/  
#endif

namespace melon::raft {

    // DirReader iterates a directory to get sub directories and files, `.' and `..'
    // should be ignored
    class DirReader {
    public:
        DirReader() {}

        virtual ~DirReader() {}

        // Check if this dir reader is valid
        virtual bool is_valid() const = 0;

        // Move to next entry(directory or file) in the directory
        // Return true if a entry can be found, false otherwise
        virtual bool next() = 0;

        // Get the name of current entry
        virtual const char *name() const = 0;

    private:
        DISALLOW_COPY_AND_ASSIGN(DirReader);
    };

    template<typename T>
    struct DestroyObj {
        void operator()(T *const obj) {
            obj->close();
            delete obj;
        }
    };


    class FileAdaptor {
    public:
        virtual ~FileAdaptor() {}

        // Write to the file. Different from posix ::pwrite(), write will retry automatically
        // when occur EINTR.
        // Return |data.size()| if successful, -1 otherwise.
        virtual ssize_t write(const mutil::IOBuf &data, off_t offset) = 0;

        // Read from the file. Different from posix ::pread(), read will retry automatically
        // when occur EINTR.
        // Return a non-negative integer less than or equal to |size| if successful, -1 otherwise.
        // In the case of EOF, the return value is a non-negative integer less than |size|.
        virtual ssize_t read(mutil::IOPortal *portal, off_t offset, size_t size) = 0;

        // Get the size of the file
        virtual ssize_t size() = 0;

        // Sync data of the file to disk device
        virtual bool sync() = 0;

        // Close the descriptor of this file adaptor
        virtual bool close() = 0;

    protected:

        FileAdaptor() {}

    private:
        DISALLOW_COPY_AND_ASSIGN(FileAdaptor);
    };

    class FileSystemAdaptor : public mutil::RefCountedThreadSafe<FileSystemAdaptor> {
    public:
        FileSystemAdaptor() {}

        virtual ~FileSystemAdaptor() {}

        // Open a file, oflag can be any valid combination of flags used by posix ::open(),
        // file_meta can be used to pass additinal metadata, it won't be modified, and should
        // be valid until the file is destroyed.
        virtual FileAdaptor *open(const std::string &path, int oflag,
                                  const ::google::protobuf::Message *file_meta,
                                  mutil::File::Error *e) = 0;

        // Deletes the given path, whether it's a file or a directory. If it's a directory,
        // it's perfectly happy to delete all of the directory's contents. Passing true to
        // recursive deletes subdirectories and their contents as well.
        // Returns true if successful, false otherwise. It is considered successful
        // to attempt to delete a file that does not exist.
        virtual bool delete_file(const std::string &path, bool recursive) = 0;

        // The same as posix ::rename(), will change the name of the old path.
        virtual bool rename(const std::string &old_path, const std::string &new_path) = 0;

        // The same as posix ::link(), will link the old path to the new path.
        virtual bool link(const std::string &old_path, const std::string &new_path) = 0;

        // Creates a directory. If create_parent_directories is true, parent directories
        // will be created if not exist, otherwise, the create operation will fail.
        // Returns 'true' on successful creation, or if the directory already exists.
        virtual bool create_directory(const std::string &path,
                                      mutil::File::Error *error,
                                      bool create_parent_directories) = 0;

        // Returns true if the given path exists on the filesystem, false otherwise.
        virtual bool path_exists(const std::string &path) = 0;

        // Returns true if the given path exists and is a directory, false otherwise.
        virtual bool directory_exists(const std::string &path) = 0;

        // Get a directory reader to read all sub entries inside a directory. It will
        // not recursively search the directory.
        virtual DirReader *directory_reader(const std::string &path) = 0;

        // This method will be called at the very begin before read snapshot file.
        // The default implemention is return 'true' directly.
        virtual bool open_snapshot(const std::string & /*snapshot_path*/) { return true; }

        // This method will be called after read all snapshot files or failed.
        // The default implemention is return directly.
        virtual void close_snapshot(const std::string & /*snapshot_path*/) {}

    private:
        DISALLOW_COPY_AND_ASSIGN(FileSystemAdaptor);
    };

// DirReader iterates a directory to get names of all sub directories and files,
// except `.' and `..'.
    class PosixDirReader : public DirReader {
        friend class PosixFileSystemAdaptor;

    public:
        virtual ~PosixDirReader() {}

        // Check if the dir reader is valid
        virtual bool is_valid() const;

        // Move to next entry in the directory
        // Return true if a entry can be found, false otherwise
        virtual bool next();

        // Get the name of current entry
        virtual const char *name() const;

    protected:
        PosixDirReader(const std::string &path) : _dir_reader(path.c_str()) {}

    private:
        mutil::DirReaderPosix _dir_reader;
    };

    class PosixFileAdaptor : public FileAdaptor {
        friend class PosixFileSystemAdaptor;

    public:
        virtual ~PosixFileAdaptor();

        virtual ssize_t write(const mutil::IOBuf &data, off_t offset);

        virtual ssize_t read(mutil::IOPortal *portal, off_t offset, size_t size);

        virtual ssize_t size();

        virtual bool sync();

        virtual bool close();

    protected:
        PosixFileAdaptor(int fd) : _fd(fd) {}

    private:
        int _fd;
    };

    class BufferedSequentialReadFileAdaptor : public FileAdaptor {
    public:
        virtual ~BufferedSequentialReadFileAdaptor() {}

        virtual ssize_t write(const mutil::IOBuf &data, off_t offset) {
            CHECK(false);
            return -1;
        }

        virtual ssize_t read(mutil::IOPortal *portal, off_t offset, size_t size);

        virtual bool sync() {
            CHECK(false);
            return false;
        }

        virtual bool close() {
            return true;
        }

        virtual ssize_t size() {
            return _reach_file_eof ? (_buffer_offset + _buffer_size) : SSIZE_MAX;
        }

    protected:
        BufferedSequentialReadFileAdaptor()
                : _buffer_offset(0), _buffer_size(0), _reach_file_eof(false), _error(0) {}

        // The |need_count| here is just a suggestion, |nread| can be larger than |need_count|
        // actually, if needed. This is useful for some special cases, in which data must be
        // read atomically. If |nread| < |need_count|, the wrapper think eof is reached.
        virtual int do_read(mutil::IOPortal *portal, size_t need_count, size_t *nread) = 0;

    private:
        mutil::IOBuf _buffer;
        off_t _buffer_offset;
        size_t _buffer_size;
        bool _reach_file_eof;
        int _error;
    };

    class BufferedSequentialWriteFileAdaptor : public FileAdaptor {
    public:
        virtual ~BufferedSequentialWriteFileAdaptor() {}

        virtual ssize_t write(const mutil::IOBuf &data, off_t offset);

        virtual ssize_t read(mutil::IOPortal *portal, off_t offset, size_t size) {
            CHECK(false);
            return -1;
        }

        virtual bool sync() {
            CHECK(false);
            return false;
        }

        virtual bool close() {
            // All data should be written into underlayer device
            return _buffer.size() == 0;
        }

        virtual ssize_t size() {
            CHECK(false);
            return -1;
        }

    protected:
        BufferedSequentialWriteFileAdaptor()
                : _buffer_offset(0), _buffer_size(0), _error(0) {}

        // Write |data| into underlayer device with its current offset, |nwrite|
        // is set to the number of written bytes
        // Return 0 on success
        virtual int do_write(const mutil::IOBuf &data, size_t *nwrite) = 0;

        // Seek to a given offset when there is hole
        // NOTICE: Only seek to bigger offset
        virtual void seek(off_t offset) {
            _buffer_offset = offset;
        }

        mutil::IOBuf _buffer;
        off_t _buffer_offset;
        size_t _buffer_size;
        int _error;
    };

    class PosixFileSystemAdaptor : public FileSystemAdaptor {
    public:
        PosixFileSystemAdaptor() {}

        virtual ~PosixFileSystemAdaptor() {}

        virtual FileAdaptor *open(const std::string &path, int oflag,
                                  const ::google::protobuf::Message *file_meta,
                                  mutil::File::Error *e);

        virtual bool delete_file(const std::string &path, bool recursive);

        virtual bool rename(const std::string &old_path, const std::string &new_path);

        virtual bool link(const std::string &old_path, const std::string &new_path);

        virtual bool create_directory(const std::string &path,
                                      mutil::File::Error *error,
                                      bool create_parent_directories);

        virtual bool path_exists(const std::string &path);

        virtual bool directory_exists(const std::string &path);

        virtual DirReader *directory_reader(const std::string &path);
    };

    // Get a default file system adapotor, it's a singleton PosixFileSystemAdaptor.
    FileSystemAdaptor *default_file_system();

    // Convert mutil::File::Error to os error
    int file_error_to_os_error(mutil::File::Error e);

    // Create a sub directory of an existing |parent_path|. Requiring that
    // |parent_path| must exist.
    // Returns true on successful creation, or if the directory already exists.
    // Returns false on failure and sets *error appropriately, if it is non-NULL.
    bool create_sub_directory(const std::string &parent_path,
                              const std::string &sub_path,
                              FileSystemAdaptor *fs = NULL,
                              mutil::File::Error *error = NULL);

} //  namespace melon::raft

