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



#ifndef BRPC_POLICY_NSHEAD_MCPACK_PROTOCOL_H
#define BRPC_POLICY_NSHEAD_MCPACK_PROTOCOL_H

#include "melon/rpc/nshead_pb_service_adaptor.h"
#include "melon/rpc/policy/nshead_protocol.h"


namespace melon {
namespace policy {

// Actions to a (server) response in nshead+mcpack format.
void ProcessNsheadMcpackResponse(InputMessageBase* msg);

void SerializeNsheadMcpackRequest(mutil::IOBuf* buf, Controller* cntl,
                                 const google::protobuf::Message* request);

// Pack `request' to `method' into `buf'.
void PackNsheadMcpackRequest(mutil::IOBuf* buf,
                             SocketMessage**,
                             uint64_t correlation_id,
                             const google::protobuf::MethodDescriptor* method,
                             Controller* controller,
                             const mutil::IOBuf& request,
                             const Authenticator* auth);

class NsheadMcpackAdaptor : public NsheadPbServiceAdaptor {
public:
    void ParseNsheadMeta(const Server& svr,
                        const NsheadMessage& request,
                        Controller*,
                        NsheadMeta* out_meta) const;

    void ParseRequestFromIOBuf(
        const NsheadMeta& meta, const NsheadMessage& ns_req,
        Controller* controller, google::protobuf::Message* pb_req) const;

    void SerializeResponseToIOBuf(
        const NsheadMeta& meta,
        Controller* controller,
        const google::protobuf::Message* pb_res,
        NsheadMessage* ns_res) const;
};

}  // namespace policy
} // namespace melon


#endif // BRPC_POLICY_NSHEAD_MCPACK_PROTOCOL_H
