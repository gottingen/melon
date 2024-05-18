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


#include "melon/raft/file_reader.h"
#include "melon/raft/util.h"

namespace melon::raft {

    LocalDirReader::~LocalDirReader() {
        if (_current_file) {
            _current_file->close();
            delete _current_file;
            _current_file = nullptr;
        }
        _fs->close_snapshot(_path);
    }

    bool LocalDirReader::open() {
        return _fs->open_snapshot(_path);
    }

    int LocalDirReader::read_file(mutil::IOBuf *out,
                                  const std::string &filename,
                                  off_t offset,
                                  size_t max_count,
                                  bool read_partly,
                                  size_t *read_count,
                                  bool *is_eof) const {
        return read_file_with_meta(out, filename, nullptr, offset,
                                   max_count, read_count, is_eof);
    }

    int LocalDirReader::read_file_with_meta(mutil::IOBuf *out,
                                            const std::string &filename,
                                            google::protobuf::Message *file_meta,
                                            off_t offset,
                                            size_t max_count,
                                            size_t *read_count,
                                            bool *is_eof) const {
        std::unique_lock<raft_mutex_t> lck(_mutex);
        if (_is_reading) {
            // Just let follower retry, if there already a reading request in process.
            lck.unlock();
            BRAFT_VMLOG << "A courrent read file is in process, path: " << _path;
            return EAGAIN;
        }
        int ret = EINVAL;
        if (filename != _current_filename) {
            if (!_eof_reached || offset != 0) {
                lck.unlock();
                BRAFT_VMLOG << "Out of order read request, path: " << _path
                           << " filename: " << filename << " offset: " << offset
                           << " max_count: " << max_count;
                return EINVAL;
            }
            if (_current_file) {
                _current_file->close();
                delete _current_file;
                _current_file = nullptr;
                _current_filename.clear();
            }
            std::string file_path(_path + "/" + filename);
            mutil::File::Error e;
            FileAdaptor *file = _fs->open(file_path, O_RDONLY | O_CLOEXEC, file_meta, &e);
            if (!file) {
                return file_error_to_os_error(e);
            }
            _current_filename = filename;
            _current_file = file;
            _eof_reached = false;
        }
        _is_reading = true;
        lck.unlock();

        do {
            mutil::IOPortal buf;
            ssize_t nread = _current_file->read(&buf, offset, max_count);
            if (nread < 0) {
                ret = EIO;
                break;
            }
            ret = 0;
            *read_count = nread;
            *is_eof = false;
            if ((size_t) nread < max_count) {
                *is_eof = true;
            } else {
                ssize_t size = _current_file->size();
                if (size < 0) {
                    return EIO;
                }
                if (size == ssize_t(offset + max_count)) {
                    *is_eof = true;
                }
            }
            out->swap(buf);
        } while (false);

        lck.lock();
        _is_reading = false;
        if (!ret) {
            _eof_reached = *is_eof;
        }
        return ret;
    }

}  //  namespace melon::raft
