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


#include <melon/raft/file_service.h>

#include <inttypes.h>
#include <stack>
#include <melon/utility/file_util.h>
#include <melon/utility/files/file_path.h>
#include <melon/utility/files/file_enumerator.h>
#include <melon/rpc/closure_guard.h>
#include <melon/rpc/controller.h>
#include <melon/raft/util.h>

namespace melon::raft {

    DEFINE_bool(raft_file_check_hole, false, "file service check hole switch, default disable");

    void FileServiceImpl::get_file(::google::protobuf::RpcController *controller,
                                   const ::melon::raft::GetFileRequest *request,
                                   ::melon::raft::GetFileResponse *response,
                                   ::google::protobuf::Closure *done) {
        scoped_refptr<FileReader> reader;
        melon::ClosureGuard done_gurad(done);
        melon::Controller *cntl = (melon::Controller *) controller;
        std::unique_lock<raft_mutex_t> lck(_mutex);
        Map::const_iterator iter = _reader_map.find(request->reader_id());
        if (iter == _reader_map.end()) {
            lck.unlock();
            cntl->SetFailed(ENOENT, "Fail to find reader=%" PRId64, request->reader_id());
            return;
        }
        // Don't touch iter ever after
        reader = iter->second;
        lck.unlock();
        BRAFT_VLOG << "get_file for " << cntl->remote_side() << " path=" << reader->path()
                   << " filename=" << request->filename()
                   << " offset=" << request->offset() << " count=" << request->count();

        if (request->count() <= 0 || request->offset() < 0) {
            cntl->SetFailed(melon::EREQUEST, "Invalid request=%s",
                            request->ShortDebugString().c_str());
            return;
        }

        mutil::IOBuf buf;
        bool is_eof = false;
        size_t read_count = 0;

        const int rc = reader->read_file(
                &buf, request->filename(),
                request->offset(), request->count(),
                request->read_partly(),
                &read_count,
                &is_eof);
        if (rc != 0) {
            cntl->SetFailed(rc, "Fail to read from path=%s filename=%s : %s",
                            reader->path().c_str(), request->filename().c_str(), berror(rc));
            return;
        }

        response->set_eof(is_eof);
        response->set_read_size(read_count);
        // skip empty data
        if (buf.size() == 0) {
            return;
        }

        FileSegData seg_data;
        if (!FLAGS_raft_file_check_hole) {
            seg_data.append(buf, request->offset());
        } else {
            off_t buf_off = request->offset();
            while (!buf.empty()) {
                mutil::StringPiece p = buf.backing_block(0);
                if (!is_zero(p.data(), p.size())) {
                    mutil::IOBuf piece_buf;
                    buf.cutn(&piece_buf, p.size());
                    seg_data.append(piece_buf, buf_off);
                } else {
                    // skip zero IOBuf block
                    buf.pop_front(p.size());
                }
                buf_off += p.size();
            }
        }
        cntl->response_attachment().swap(seg_data.data());
    }

    FileServiceImpl::FileServiceImpl() {
        _next_id = ((int64_t) getpid() << 45) | (mutil::gettimeofday_us() << 17 >> 17);
    }

    int FileServiceImpl::add_reader(FileReader *reader, int64_t *reader_id) {
        MELON_SCOPED_LOCK(_mutex);
        *reader_id = _next_id++;
        _reader_map[*reader_id] = reader;
        return 0;
    }

    int FileServiceImpl::remove_reader(int64_t reader_id) {
        MELON_SCOPED_LOCK(_mutex);
        return _reader_map.erase(reader_id) == 1 ? 0 : -1;
    }

}  //  namespace melon::raft
