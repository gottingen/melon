// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <melon/utility/fd_utility.h>                        // mutil::make_close_on_exec
#include <melon/utility/memory/singleton_on_pthread_once.h>  // mutil::get_leaky_singleton
#include "melon/raft/file_system_adaptor.h"

namespace melon::raft {

    bool PosixDirReader::is_valid() const {
        return _dir_reader.IsValid();
    }

    bool PosixDirReader::next() {
        bool rc = _dir_reader.Next();
        while (rc && (strcmp(name(), ".") == 0 || strcmp(name(), "..") == 0)) {
            rc = _dir_reader.Next();
        }
        return rc;
    }

    const char *PosixDirReader::name() const {
        return _dir_reader.name();
    }

    PosixFileAdaptor::~PosixFileAdaptor() {
    }

    ssize_t PosixFileAdaptor::write(const mutil::IOBuf &data, off_t offset) {
        return melon::raft::file_pwrite(data, _fd, offset);
    }

    ssize_t PosixFileAdaptor::read(mutil::IOPortal *portal, off_t offset, size_t size) {
        return melon::raft::file_pread(portal, _fd, offset, size);
    }

    ssize_t PosixFileAdaptor::size() {
        off_t sz = lseek(_fd, 0, SEEK_END);
        return ssize_t(sz);
    }

    bool PosixFileAdaptor::sync() {
        return raft_fsync(_fd) == 0;
    }

    bool PosixFileAdaptor::close() {
        if (_fd > 0) {
            bool res = ::close(_fd) == 0;
            _fd = -1;
            return res;
        }
        return true;
    }

    ssize_t BufferedSequentialReadFileAdaptor::read(mutil::IOPortal *portal, off_t offset, size_t size) {
        if (_error) {
            errno = _error;
            return -1;
        }

        BRAFT_VLOG << "begin read offset " << offset << " count " << size
                   << ", buffer_offset " << _buffer_offset
                   << " buffer_size " << _buffer_size;
        if (offset < _buffer_offset || offset > off_t(_buffer_offset + _buffer_size)) {
            MLOG(WARNING) << "Fail to read from buffered file adaptor with invalid range"
                         << ", buffer_offset: " << _buffer_offset
                         << ", buffer_size: " << _buffer_size
                         << ", read offset: " << offset
                         << ", read size: " << size;
            errno = EINVAL;
            return -1;
        }
        if (offset > _buffer_offset) {
            _buffer.pop_front(std::min(size_t(offset - _buffer_offset), _buffer.size()));
            _buffer_size -= (offset - _buffer_offset);
            _buffer_offset = offset;
        }
        off_t end_offset = offset + size;
        if (!_reach_file_eof && end_offset > off_t(_buffer_offset + _buffer_size)) {
            mutil::IOPortal tmp_portal;
            size_t need_count = end_offset - _buffer_offset - _buffer_size;
            size_t read_count = 0;
            int rc = do_read(&tmp_portal, need_count, &read_count);
            if (rc != 0) {
                _error = ((errno != 0) ? errno : EIO);
                errno = _error;
                return -1;
            }
            _reach_file_eof = (read_count < need_count);
            if (!tmp_portal.empty()) {
                _buffer.resize(_buffer_size);
                _buffer.append(tmp_portal);
            }
            _buffer_size += read_count;
        }
        ssize_t nread = std::min(_buffer_size, size);
        if (!_buffer.empty()) {
            _buffer.append_to(portal, std::min(_buffer.size(), size_t(nread)));
        }
        return nread;
    }

    ssize_t BufferedSequentialWriteFileAdaptor::write(const mutil::IOBuf &data, off_t offset) {
        if (_error) {
            errno = _error;
            return -1;
        }

        BRAFT_VLOG << "begin write offset " << offset << ", data_size " << data.size()
                   << ", buffer_offset " << _buffer_offset
                   << ", buffer_size " << _buffer_size;
        if (offset < _buffer_offset + _buffer_size) {
            MLOG(WARNING) << "Fail to write into buffered file adaptor with invalid range"
                         << ", offset: " << offset
                         << ", data_size: " << data.size()
                         << ", buffer_offset: " << _buffer_offset
                         << ", buffer_size: " << _buffer_size;
            errno = EINVAL;
            return -1;
        } else if (offset > _buffer_offset + _buffer_size) {
            // passby hole
            MCHECK(_buffer_size == 0);
            BRAFT_VLOG << "seek to new offset " << offset << " as there is hole";
            seek(offset);
        }
        const size_t saved_size = data.size();
        _buffer.append(data);
        _buffer_size = _buffer.size();
        if (_buffer.size() > 0) {
            size_t write_count = 0;
            int rc = do_write(_buffer, &write_count);
            if (rc < 0) {
                _error = ((errno != 0) ? errno : EIO);
                errno = _error;
                return -1;
            }
            _buffer_offset += write_count;
            _buffer_size -= write_count;
            _buffer.pop_front(write_count);
            MCHECK_EQ(_buffer_size, _buffer.size());
        }
        return saved_size;
    }

    static pthread_once_t s_check_cloexec_once = PTHREAD_ONCE_INIT;
    static bool s_support_cloexec_on_open = false;

    static void check_cloexec(void) {
        int fd = ::open("/dev/zero", O_RDONLY | O_CLOEXEC, 0644);
        s_support_cloexec_on_open = (fd != -1);
        if (fd != -1) {
            ::close(fd);
        }
    }

    FileAdaptor *PosixFileSystemAdaptor::open(const std::string &path, int oflag,
                                              const ::google::protobuf::Message *file_meta,
                                              mutil::File::Error *e) {
        (void) file_meta;
        pthread_once(&s_check_cloexec_once, check_cloexec);
        bool cloexec = (oflag & O_CLOEXEC);
        if (cloexec && !s_support_cloexec_on_open) {
            oflag &= (~O_CLOEXEC);
        }
        int fd = ::open(path.c_str(), oflag, 0644);
        if (e) {
            *e = (fd == -1) ? mutil::File::OSErrorToFileError(errno) : mutil::File::FILE_OK;
        }
        if (fd == -1) {
            return NULL;
        }
        if (cloexec && !s_support_cloexec_on_open) {
            mutil::make_close_on_exec(fd);
        }
        return new PosixFileAdaptor(fd);
    }

    bool PosixFileSystemAdaptor::delete_file(const std::string &path, bool recursive) {
        mutil::FilePath file_path(path);
        return mutil::DeleteFile(file_path, recursive);
    }

    bool PosixFileSystemAdaptor::rename(const std::string &old_path, const std::string &new_path) {
        return ::rename(old_path.c_str(), new_path.c_str()) == 0;
    }

    bool PosixFileSystemAdaptor::link(const std::string &old_path, const std::string &new_path) {
        return ::link(old_path.c_str(), new_path.c_str()) == 0;
    }

    bool PosixFileSystemAdaptor::create_directory(const std::string &path,
                                                  mutil::File::Error *error,
                                                  bool create_parent_directories) {
        mutil::FilePath dir(path);
        return mutil::CreateDirectoryAndGetError(dir, error, create_parent_directories);
    }

    bool PosixFileSystemAdaptor::path_exists(const std::string &path) {
        mutil::FilePath file_path(path);
        return mutil::PathExists(file_path);
    }

    bool PosixFileSystemAdaptor::directory_exists(const std::string &path) {
        mutil::FilePath file_path(path);
        return mutil::DirectoryExists(file_path);
    }

    DirReader *PosixFileSystemAdaptor::directory_reader(const std::string &path) {
        return new PosixDirReader(path.c_str());
    }

    FileSystemAdaptor *default_file_system() {
        static scoped_refptr<PosixFileSystemAdaptor> fs =
                mutil::get_leaky_singleton<PosixFileSystemAdaptor>();
        return fs.get();
    }

    int file_error_to_os_error(mutil::File::Error e) {
        switch (e) {
            case mutil::File::FILE_OK:
                return 0;
            case mutil::File::FILE_ERROR_IN_USE:
                return EAGAIN;
            case mutil::File::FILE_ERROR_ACCESS_DENIED:
                return EACCES;
            case mutil::File::FILE_ERROR_EXISTS:
                return EEXIST;
            case mutil::File::FILE_ERROR_NOT_FOUND:
                return ENOENT;
            case mutil::File::FILE_ERROR_TOO_MANY_OPENED:
                return EMFILE;
            case mutil::File::FILE_ERROR_NO_MEMORY:
                return ENOMEM;
            case mutil::File::FILE_ERROR_NO_SPACE:
                return ENOSPC;
            case mutil::File::FILE_ERROR_NOT_A_DIRECTORY:
                return ENOTDIR;
            case mutil::File::FILE_ERROR_IO:
                return EIO;
            default:
                return EINVAL;
        };
    }

    bool create_sub_directory(const std::string &parent_path,
                              const std::string &sub_path,
                              FileSystemAdaptor *fs,
                              mutil::File::Error *error) {
        if (!fs) {
            fs = default_file_system();
        }
        if (!fs->directory_exists(parent_path)) {
            if (error) {
                *error = mutil::File::FILE_ERROR_NOT_FOUND;
            }
            return false;
        }
        mutil::FilePath sub_dir_path(sub_path);
        if (sub_dir_path.ReferencesParent()) {
            if (error) {
                *error = mutil::File::FILE_ERROR_INVALID_URL;
            }
            return false;
        }
        std::vector<mutil::FilePath> subpaths;

        // Collect a list of all parent directories.
        mutil::FilePath last_path = sub_dir_path;
        subpaths.push_back(sub_dir_path.BaseName());
        for (mutil::FilePath path = last_path.DirName();
             path.value() != last_path.value(); path = path.DirName()) {
            subpaths.push_back(path.BaseName());
            last_path = path;
        }
        mutil::FilePath full_path(parent_path);
        for (std::vector<mutil::FilePath>::reverse_iterator i = subpaths.rbegin();
             i != subpaths.rend(); ++i) {
            if (i->value() == "/") {
                continue;
            }
            if (i->value() == ".") {
                continue;
            }
            full_path = full_path.Append(*i);
            DMLOG(INFO) << "Creating " << full_path.value();
            if (!fs->create_directory(full_path.value(), error, false)) {
                return false;
            }
        }
        return true;
    }


} //  namespace melon::raft
