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
