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

//          Jiashun Zhu(zhujiashun2010@gmail.com)

#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>
#include "melon/log/logging.h"                       // MELON_LOG()
#include "melon/times/time.h"
#include "melon/io/cord_buf.h"                         // melon::cord_buf
#include "melon/rpc/controller.h"               // Controller
#include "melon/rpc/details/controller_private_accessor.h"
#include "melon/rpc/socket.h"                   // Socket
#include "melon/rpc/server.h"                   // Server
#include "melon/rpc/details/server_private_accessor.h"
#include "melon/rpc/span.h"
#include "melon/rpc/redis.h"
#include "melon/rpc/redis_command.h"
#include "melon/rpc/policy/redis_protocol.h"
#include "melon/strings/utility.h"

namespace melon::rpc {

    DECLARE_bool(enable_rpcz);
    DECLARE_bool(usercode_in_pthread);

    namespace policy {

        DEFINE_bool(redis_verbose, false,
                    "[DEBUG] Print EVERY redis request/response");

        struct InputResponse : public InputMessageBase {
            fiber_token_t id_wait;
            RedisResponse response;

            // @InputMessageBase
            void DestroyImpl() {
                delete this;
            }
        };

        // This class is as parsing_context in socket.
        class RedisConnContext : public Destroyable {
        public:
            explicit RedisConnContext(const RedisService *rs)
                    : redis_service(rs), batched_size(0) {}

            ~RedisConnContext();

            // @Destroyable
            void Destroy() override;

            const RedisService *redis_service;
            // If user starts a transaction, transaction_handler indicates the
            // handler pointer that runs the transaction command.
            std::unique_ptr<RedisCommandHandler> transaction_handler;
            // >0 if command handler is run in batched mode.
            int batched_size;

            RedisCommandParser parser;
            melon::Arena arena;
        };

        int ConsumeCommand(RedisConnContext *ctx,
                           const std::vector<std::string_view> &args,
                           bool flush_batched,
                           melon::cord_buf_appender *appender) {
            RedisReply output(&ctx->arena);
            RedisCommandHandlerResult result = REDIS_CMD_HANDLED;
            if (ctx->transaction_handler) {
                result = ctx->transaction_handler->Run(args, &output, flush_batched);
                if (result == REDIS_CMD_HANDLED) {
                    ctx->transaction_handler.reset(nullptr);
                } else if (result == REDIS_CMD_BATCHED) {
                    MELON_LOG(ERROR) << "BATCHED should not be returned by a transaction handler.";
                    return -1;
                }
            } else {
                RedisCommandHandler *ch = ctx->redis_service->FindCommandHandler(args[0]);
                if (!ch) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "ERR unknown command `%s`", melon::as_string(args[0]).c_str());
                    output.SetError(buf);
                } else {
                    result = ch->Run(args, &output, flush_batched);
                    if (result == REDIS_CMD_CONTINUE) {
                        if (ctx->batched_size != 0) {
                            MELON_LOG(ERROR) << "CONTINUE should not be returned in a batched process.";
                            return -1;
                        }
                        ctx->transaction_handler.reset(ch->NewTransactionHandler());
                    } else if (result == REDIS_CMD_BATCHED) {
                        ctx->batched_size++;
                    }
                }
            }
            if (result == REDIS_CMD_HANDLED) {
                if (ctx->batched_size) {
                    if ((int) output.size() != (ctx->batched_size + 1)) {
                        MELON_LOG(ERROR) << "reply array size can't be matched with batched size, "
                                         << " expected=" << ctx->batched_size + 1 << " actual=" << output.size();
                        return -1;
                    }
                    for (int i = 0; i < (int) output.size(); ++i) {
                        output[i].SerializeTo(appender);
                    }
                    ctx->batched_size = 0;
                } else {
                    output.SerializeTo(appender);
                }
            } else if (result == REDIS_CMD_CONTINUE) {
                output.SerializeTo(appender);
            } else if (result == REDIS_CMD_BATCHED) {
                // just do nothing and wait handler to return OK.
            } else {
                MELON_LOG(ERROR) << "unknown status=" << result;
                return -1;
            }
            return 0;
        }

        // ========== impl of RedisConnContext ==========

        RedisConnContext::~RedisConnContext() {}

        void RedisConnContext::Destroy() {
            delete this;
        }

        // ========== impl of RedisConnContext ==========

        ParseResult ParseRedisMessage(melon::cord_buf *source, Socket *socket,
                                      bool read_eof, const void *arg) {
            if (read_eof || source->empty()) {
                return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
            }
            const Server *server = static_cast<const Server *>(arg);
            if (server) {
                const RedisService *const rs = server->options().redis_service;
                if (!rs) {
                    return MakeParseError(PARSE_ERROR_TRY_OTHERS);
                }
                RedisConnContext *ctx = static_cast<RedisConnContext *>(socket->parsing_context());
                if (ctx == nullptr) {
                    ctx = new RedisConnContext(rs);
                    socket->reset_parsing_context(ctx);
                }
                std::vector<std::string_view> current_args;
                melon::cord_buf_appender appender;
                ParseError err = PARSE_OK;

                err = ctx->parser.Consume(*source, &current_args, &ctx->arena);
                if (err != PARSE_OK) {
                    return MakeParseError(err);
                }
                while (true) {
                    std::vector<std::string_view> next_args;
                    err = ctx->parser.Consume(*source, &next_args, &ctx->arena);
                    if (err != PARSE_OK) {
                        break;
                    }
                    if (ConsumeCommand(ctx, current_args, false, &appender) != 0) {
                        return MakeParseError(PARSE_ERROR_ABSOLUTELY_WRONG);
                    }
                    current_args.swap(next_args);
                }
                if (ConsumeCommand(ctx, current_args,
                                   true /*must be the last message*/, &appender) != 0) {
                    return MakeParseError(PARSE_ERROR_ABSOLUTELY_WRONG);
                }
                melon::cord_buf sendbuf;
                appender.move_to(sendbuf);
                MELON_CHECK(!sendbuf.empty());
                Socket::WriteOptions wopt;
                wopt.ignore_eovercrowded = true;
                MELON_LOG_IF(WARNING, socket->Write(&sendbuf, &wopt) != 0)
                                << "Fail to send redis reply";
                ctx->arena.clear();
                return MakeParseError(err);
            } else {
                // NOTE(gejun): PopPipelinedInfo() is actually more contended than what
                // I thought before. The Socket._pipeline_q is a SPSC queue pushed before
                // sending and popped when response comes back, being protected by a
                // mutex. Previously the mutex is shared with Socket._id_wait_list. When
                // 200 fibers access one redis-server, ~1.5s in total is spent on
                // contention in 10-second duration. If the mutex is separated, the time
                // drops to ~0.25s. I further replaced PeekPipelinedInfo() with
                // GivebackPipelinedInfo() to lock only once(when receiving response)
                // in most cases, and the time decreases to ~0.14s.
                PipelinedInfo pi;
                if (!socket->PopPipelinedInfo(&pi)) {
                    MELON_LOG(WARNING) << "No corresponding PipelinedInfo in socket";
                    return MakeParseError(PARSE_ERROR_TRY_OTHERS);
                }

                do {
                    InputResponse *msg = static_cast<InputResponse *>(socket->parsing_context());
                    if (msg == nullptr) {
                        msg = new InputResponse;
                        socket->reset_parsing_context(msg);
                    }

                    const int consume_count = (pi.with_auth ? 1 : pi.count);

                    ParseError err = msg->response.ConsumePartialCordBuf(*source, consume_count);
                    if (err != PARSE_OK) {
                        socket->GivebackPipelinedInfo(pi);
                        return MakeParseError(err);
                    }

                    if (pi.with_auth) {
                        if (msg->response.reply_size() != 1 ||
                            !(msg->response.reply(0).type() == melon::rpc::REDIS_REPLY_STATUS &&
                              msg->response.reply(0).data().compare("OK") == 0)) {
                            MELON_LOG(ERROR) << "Redis Auth failed: " << msg->response;
                            return MakeParseError(PARSE_ERROR_NO_RESOURCE,
                                                  "Fail to authenticate with Redis");
                        }

                        DestroyingPtr<InputResponse> auth_msg(
                                static_cast<InputResponse *>(socket->release_parsing_context()));
                        pi.with_auth = false;
                        continue;
                    }

                    MELON_CHECK_EQ((uint32_t) msg->response.reply_size(), pi.count);
                    msg->id_wait = pi.id_wait;
                    socket->release_parsing_context();
                    return MakeMessage(msg);
                } while (true);
            }

            return MakeParseError(PARSE_ERROR_ABSOLUTELY_WRONG);
        }

        void ProcessRedisResponse(InputMessageBase *msg_base) {
            const int64_t start_parse_us = melon::get_current_time_micros();
            DestroyingPtr<InputResponse> msg(static_cast<InputResponse *>(msg_base));

            const fiber_token_t cid = msg->id_wait;
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
                span->set_response_size(msg->response.ByteSize());
                span->set_start_parse_us(start_parse_us);
            }
            const int saved_error = cntl->ErrorCode();
            if (cntl->response() != nullptr) {
                if (cntl->response()->GetDescriptor() != RedisResponse::descriptor()) {
                    cntl->SetFailed(ERESPONSE, "Must be RedisResponse");
                } else {
                    // We work around ParseFrom of pb which is just a placeholder.
                    if (msg->response.reply_size() != (int) accessor.pipelined_count()) {
                        cntl->SetFailed(ERESPONSE, "pipelined_count=%d of response does "
                                                   "not equal request's=%d",
                                        msg->response.reply_size(), accessor.pipelined_count());
                    }
                    ((RedisResponse *) cntl->response())->Swap(&msg->response);
                    if (FLAGS_redis_verbose) {
                        MELON_LOG(INFO) << "\n[REDIS RESPONSE] "
                                        << *((RedisResponse *) cntl->response());
                    }
                }
            } // silently ignore the response.

            // Unlocks correlation_id inside. Revert controller's
            // error code if it version check of `cid' fails
            msg.reset();  // optional, just release resourse ASAP
            accessor.OnResponse(cid, saved_error);
        }

        void ProcessRedisRequest(InputMessageBase *msg_base) {}

        void SerializeRedisRequest(melon::cord_buf *buf,
                                   Controller *cntl,
                                   const google::protobuf::Message *request) {
            if (request == nullptr) {
                return cntl->SetFailed(EREQUEST, "request is nullptr");
            }
            if (request->GetDescriptor() != RedisRequest::descriptor()) {
                return cntl->SetFailed(EREQUEST, "The request is not a RedisRequest");
            }
            const RedisRequest *rr = (const RedisRequest *) request;
            // We work around SerializeTo of pb which is just a placeholder.
            if (!rr->SerializeTo(buf)) {
                return cntl->SetFailed(EREQUEST, "Fail to serialize RedisRequest");
            }
            ControllerPrivateAccessor(cntl).set_pipelined_count(rr->command_size());
            if (FLAGS_redis_verbose) {
                MELON_LOG(INFO) << "\n[REDIS REQUEST] " << *rr;
            }
        }

        void PackRedisRequest(melon::cord_buf *buf,
                              SocketMessage **,
                              uint64_t /*correlation_id*/,
                              const google::protobuf::MethodDescriptor *,
                              Controller *cntl,
                              const melon::cord_buf &request,
                              const Authenticator *auth) {
            if (auth) {
                std::string auth_str;
                if (auth->GenerateCredential(&auth_str) != 0) {
                    return cntl->SetFailed(EREQUEST, "Fail to generate credential");
                }
                buf->append(auth_str);
                ControllerPrivateAccessor(cntl).add_with_auth();
            } else {
                ControllerPrivateAccessor(cntl).clear_with_auth();
            }

            buf->append(request);
        }

        const std::string &GetRedisMethodName(
                const google::protobuf::MethodDescriptor *,
                const Controller *) {
            const static std::string REDIS_SERVER_STR = "redis-server";
            return REDIS_SERVER_STR;
        }

    }  // namespace policy
} // namespace melon::rpc