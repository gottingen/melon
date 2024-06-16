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

#include <melon/proto/raft/file_service.pb.h>
#include <melon/raft/file_reader.h>
#include <melon/raft/util.h>

namespace melon::raft {

    class MELON_CACHELINE_ALIGNMENT FileServiceImpl : public FileService {
    public:
        static FileServiceImpl *GetInstance() {
            return Singleton<FileServiceImpl>::get();
        }

        void get_file(::google::protobuf::RpcController *controller,
                      const ::melon::raft::GetFileRequest *request,
                      ::melon::raft::GetFileResponse *response,
                      ::google::protobuf::Closure *done);

        int add_reader(FileReader *reader, int64_t *reader_id);

        int remove_reader(int64_t reader_id);

    private:
        friend struct DefaultSingletonTraits<FileServiceImpl>;

        FileServiceImpl();

        ~FileServiceImpl() {}

        typedef std::map<int64_t, scoped_refptr<FileReader> > Map;
        raft_mutex_t _mutex;
        int64_t _next_id;
        Map _reader_map;
    };

    inline FileServiceImpl *file_service() { return FileServiceImpl::GetInstance(); }

    inline int file_service_add(FileReader *reader, int64_t *reader_id) {
        FileServiceImpl *const fs = file_service();
        return fs->add_reader(reader, reader_id);
    }

    inline int file_service_remove(int64_t reader_id) {
        FileServiceImpl *const fs = file_service();
        return fs->remove_reader(reader_id);
    }

}  //  namespace melon::raft
