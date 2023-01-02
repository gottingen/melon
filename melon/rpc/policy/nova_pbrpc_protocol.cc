// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <inttypes.h>
#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>

#include "melon/times/time.h"
#include "melon/io/cord_buf.h"                        // melon::cord_buf

#include "melon/rpc/controller.h"               // Controller
#include "melon/rpc/socket.h"                   // Socket
#include "melon/rpc/server.h"                   // Server
#include "melon/rpc/details/server_private_accessor.h"
#include "melon/rpc/span.h"
#include "melon/rpc/errno.pb.h"                 // EREQUEST, ERESPONSE
#include "melon/rpc/details/controller_private_accessor.h"
#include "melon/rpc/policy/most_common_message.h"
#include "melon/rpc/policy/nova_pbrpc_protocol.h"
#include "melon/rpc/compress.h"


namespace melon::rpc {
    namespace policy {

        // Protocol of NOVA PBRPC:
        //  - It doesn't support authentication, attachment and compression
        //  - The head is nshead, followed by the request body, without meta field,
        //    fields in nshead are NOT in network order.
        //  - Can not send feedback on failure.
        //  - There should be only one user service in server because there's no service
        //    information in the request, |reserved| field in nshead is the method index
        //  - |id|, |version| and |provider| in head are undefined, |magic_num| should be
        //    NSHEAD_MAGIC in both request and response
        static const unsigned short NOVA_SNAPPY_COMPRESS_FLAG = 0x1u;

        void NovaServiceAdaptor::ParseNsheadMeta(
                const Server &svr, const NsheadMessage &request, Controller *cntl,
                NsheadMeta *out_meta) const {
            google::protobuf::Service *service = svr.first_service();
            if (!service) {
                cntl->SetFailed(ENOSERVICE, "No first_service in this server");
                return;
            }
            const int method_index = request.head.reserved;
            const google::protobuf::ServiceDescriptor *sd = service->GetDescriptor();
            if (method_index < 0 || method_index >= sd->method_count()) {
                cntl->SetFailed(ENOMETHOD, "Fail to find method by index=%d", method_index);
                return;
            }
            const google::protobuf::MethodDescriptor *method = sd->method(method_index);
            out_meta->set_full_method_name(method->full_name());
            if (request.head.version & NOVA_SNAPPY_COMPRESS_FLAG) {
                out_meta->set_compress_type(COMPRESS_TYPE_SNAPPY);
            }
        }

        void NovaServiceAdaptor::ParseRequestFromCordBuf(
                const NsheadMeta &meta, const NsheadMessage &raw_req,
                Controller *cntl, google::protobuf::Message *pb_req) const {
            CompressType type = meta.compress_type();
            if (!ParseFromCompressedData(raw_req.body, pb_req, type)) {
                cntl->SetFailed(EREQUEST, "Fail to parse request message, "
                                          "CompressType=%s, request_size=%" PRIu64,
                                CompressTypeToCStr(type),
                                (uint64_t) raw_req.body.length());
            } else {
                cntl->set_request_compress_type(type);
            }
        }

        void NovaServiceAdaptor::SerializeResponseToCordBuf(
                const NsheadMeta &, Controller *cntl,
                const google::protobuf::Message *pb_res, NsheadMessage *raw_res) const {
            if (cntl->Failed()) {
                cntl->CloseConnection("Close connection due to previous error");
                return;
            }
            CompressType type = cntl->response_compress_type();
            if (type == COMPRESS_TYPE_SNAPPY) {
                raw_res->head.version = NOVA_SNAPPY_COMPRESS_FLAG;
            } else if (type != COMPRESS_TYPE_NONE) {
                MELON_LOG(WARNING) << "nova_pbrpc protocol doesn't support "
                                   << "compress_type=" << type;
                type = COMPRESS_TYPE_NONE;
            }
            if (!SerializeAsCompressedData(*pb_res, &raw_res->body, type)) {
                cntl->CloseConnection("Close connection due to failure of serialization");
                return;
            }
        }

        void ProcessNovaResponse(InputMessageBase *msg_base) {
            const int64_t start_parse_us = melon::get_current_time_micros();
            DestroyingPtr<MostCommonMessage> msg(static_cast<MostCommonMessage *>(msg_base));
            Socket *socket = msg->socket();

            // Fetch correlation id that we saved before in `PackNovaRequest'
            const fiber_token_t cid = {static_cast<uint64_t>(socket->correlation_id())};
            Controller *cntl = nullptr;
            const int rc = fiber_token_lock(cid, (void **) &cntl);
            if (rc != 0) {
                MELON_LOG_IF(ERROR, rc != EINVAL && rc != EPERM)
                                << "Fail to lock correlation_id=" << cid << ": " << melon_error(rc);
                return;
            }

            ControllerPrivateAccessor accessor(cntl);
            Span *span = accessor.span();
            if (span) {
                span->set_base_real_us(msg->base_real_us());
                span->set_received_us(msg->received_us());
                span->set_response_size(msg->meta.size() + msg->payload.size());
                span->set_start_parse_us(start_parse_us);
            }
            const int saved_error = cntl->ErrorCode();
            // Fetch compress flag from nshead
            char buf[sizeof(nshead_t)];
            const char *p = (const char *) msg->meta.fetch(buf, sizeof(buf));
            if (nullptr == p) {
                MELON_LOG(WARNING) << "Fail to fetch nshead from client="
                                   << socket->remote_side();
                return;
            }
            const nshead_t *nshead = (const nshead_t *) p;
            CompressType type = (nshead->version & NOVA_SNAPPY_COMPRESS_FLAG ?
                                 COMPRESS_TYPE_SNAPPY : COMPRESS_TYPE_NONE);
            if (!ParseFromCompressedData(msg->payload, cntl->response(), type)) {
                cntl->SetFailed(ERESPONSE, "Fail to parse response message");
            } else {
                cntl->set_response_compress_type(type);
            }
            // Unlocks correlation_id inside. Revert controller's
            // error code if it version check of `cid' fails
            msg.reset();  // optional, just release resourse ASAP
            accessor.OnResponse(cid, saved_error);
        }

        void SerializeNovaRequest(melon::cord_buf *buf, Controller *cntl,
                                  const google::protobuf::Message *request) {
            CompressType type = cntl->request_compress_type();
            if (type != COMPRESS_TYPE_NONE && type != COMPRESS_TYPE_SNAPPY) {
                cntl->SetFailed(EREQUEST, "nova_pbrpc protocol doesn't support "
                                          "compress_type=%d", type);
                return;
            }
            return SerializeRequestDefault(buf, cntl, request);
        }

        void PackNovaRequest(melon::cord_buf *buf,
                             SocketMessage **,
                             uint64_t correlation_id,
                             const google::protobuf::MethodDescriptor *method,
                             Controller *controller,
                             const melon::cord_buf &request,
                             const Authenticator * /*not supported*/) {
            ControllerPrivateAccessor accessor(controller);
            if (controller->connection_type() == CONNECTION_TYPE_SINGLE) {
                return controller->SetFailed(
                        EINVAL, "nova_pbrpc can't work with CONNECTION_TYPE_SINGLE");
            }
            // Store `correlation_id' into Socket since nova_pbrpc protocol
            // doesn't contain this field
            accessor.get_sending_socket()->set_correlation_id(correlation_id);

            nshead_t nshead;
            memset(&nshead, 0, sizeof(nshead_t));
            nshead.log_id = controller->log_id();
            nshead.magic_num = NSHEAD_MAGICNUM;
            nshead.reserved = method->index();
            nshead.body_len = request.size();
            // Set compress flag
            if (controller->request_compress_type() == COMPRESS_TYPE_SNAPPY) {
                nshead.version = NOVA_SNAPPY_COMPRESS_FLAG;
            }
            buf->append(&nshead, sizeof(nshead));

            // Span* span = accessor.span();
            // if (span) {
            //     request_meta->set_trace_id(span->trace_id());
            //     request_meta->set_span_id(span->span_id());
            //     request_meta->set_parent_span_id(span->parent_span_id());
            // }
            buf->append(request);
        }

    }  // namespace policy
} // namespace melon::rpc
