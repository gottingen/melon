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


#ifndef MELON_RAFT_PROTOBUF_FILE_H_
#define MELON_RAFT_PROTOBUF_FILE_H_

#include <string>
#include <google/protobuf/message.h>
#include <melon/raft/file_system_adaptor.h>

namespace melon::raft {

    // protobuf file format:
    // len [4B, in network order]
    // protobuf data
    class ProtoBufFile {
    public:
        ProtoBufFile(const char *path, FileSystemAdaptor *fs = NULL);

        ProtoBufFile(const std::string &path, FileSystemAdaptor *fs = NULL);

        ~ProtoBufFile() {}

        int save(const ::google::protobuf::Message *message, bool sync);

        int load(::google::protobuf::Message *message);

    private:
        std::string _path;
        scoped_refptr<FileSystemAdaptor> _fs;
    };

}

#endif // MELON_RAFT_PROTOBUF_FILE_H_
