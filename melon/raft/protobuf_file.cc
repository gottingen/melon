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

#include <melon/utility/iobuf.h>
#include <melon/utility/sys_byteorder.h>

#include "melon/raft/protobuf_file.h"

namespace melon::raft {

    ProtoBufFile::ProtoBufFile(const char *path, FileSystemAdaptor *fs)
            : _path(path), _fs(fs) {
        if (_fs == NULL) {
            _fs = default_file_system();
        }
    }

    ProtoBufFile::ProtoBufFile(const std::string &path, FileSystemAdaptor *fs)
            : _path(path), _fs(fs) {
        if (_fs == NULL) {
            _fs = default_file_system();
        }
    }

    int ProtoBufFile::save(const google::protobuf::Message *message, bool sync) {
        std::string tmp_path(_path);
        tmp_path.append(".tmp");

        mutil::File::Error e;
        FileAdaptor *file = _fs->open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, NULL, &e);
        if (!file) {
            MLOG(WARNING) << "open file failed, path: " << _path
                         << ": " << mutil::File::ErrorToString(e);
            return -1;
        }
        std::unique_ptr<FileAdaptor, DestroyObj<FileAdaptor> > guard(file);

        // serialize msg
        mutil::IOBuf header_buf;
        mutil::IOBuf msg_buf;
        mutil::IOBufAsZeroCopyOutputStream msg_wrapper(&msg_buf);
        message->SerializeToZeroCopyStream(&msg_wrapper);

        // write len
        int32_t header_len = mutil::HostToNet32(msg_buf.length());
        header_buf.append(&header_len, sizeof(int32_t));
        if (sizeof(int32_t) != file->write(header_buf, 0)) {
            MLOG(WARNING) << "write len failed, path: " << tmp_path;
            return -1;
        }

        ssize_t len = msg_buf.size();
        if (len != file->write(msg_buf, sizeof(int32_t))) {
            MLOG(WARNING) << "write failed, path: " << tmp_path;
            return -1;
        }

        // sync
        if (sync) {
            if (!file->sync()) {
                MLOG(WARNING) << "sync failed, path: " << tmp_path;
                return -1;
            }
        }

        // rename
        if (!_fs->rename(tmp_path, _path)) {
            MLOG(WARNING) << "rename failed, old: " << tmp_path << " , new: " << _path;
            return -1;
        }
        return 0;
    }

    int ProtoBufFile::load(google::protobuf::Message *message) {
        mutil::File::Error e;
        FileAdaptor *file = _fs->open(_path, O_RDONLY, NULL, &e);
        if (!file) {
            MLOG(WARNING) << "open file failed, path: " << _path
                         << ": " << mutil::File::ErrorToString(e);
            return -1;
        }

        std::unique_ptr<FileAdaptor, DestroyObj<FileAdaptor> > guard(file);

        // len
        mutil::IOPortal header_buf;
        if (sizeof(int32_t) != file->read(&header_buf, 0, sizeof(int32_t))) {
            MLOG(WARNING) << "read len failed, path: " << _path;
            return -1;
        }
        int32_t len = 0;
        header_buf.copy_to(&len, sizeof(int32_t));
        int32_t left_len = mutil::NetToHost32(len);

        // read protobuf data
        mutil::IOPortal msg_buf;
        if (left_len != file->read(&msg_buf, sizeof(int32_t), left_len)) {
            MLOG(WARNING) << "read body failed, path: " << _path;
            return -1;
        }

        // parse msg
        mutil::IOBufAsZeroCopyInputStream msg_wrapper(msg_buf);
        message->ParseFromZeroCopyStream(&msg_wrapper);

        return 0;
    }

}
