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

#include <melon/proto/raft/raft.pb.h>

namespace melon::raft {

    class RaftServiceImpl : public RaftService {
    public:
        explicit RaftServiceImpl(mutil::EndPoint addr)
                : _addr(addr) {}

        ~RaftServiceImpl();

        void pre_vote(google::protobuf::RpcController *controller,
                      const RequestVoteRequest *request,
                      RequestVoteResponse *response,
                      google::protobuf::Closure *done);

        void request_vote(google::protobuf::RpcController *controller,
                          const RequestVoteRequest *request,
                          RequestVoteResponse *response,
                          google::protobuf::Closure *done);

        void append_entries(google::protobuf::RpcController *controller,
                            const AppendEntriesRequest *request,
                            AppendEntriesResponse *response,
                            google::protobuf::Closure *done);

        void install_snapshot(google::protobuf::RpcController *controller,
                              const InstallSnapshotRequest *request,
                              InstallSnapshotResponse *response,
                              google::protobuf::Closure *done);

        void timeout_now(::google::protobuf::RpcController *controller,
                         const ::melon::raft::TimeoutNowRequest *request,
                         ::melon::raft::TimeoutNowResponse *response,
                         ::google::protobuf::Closure *done);

    private:
        mutil::EndPoint _addr;
    };

}  //  namespace melon::raft
