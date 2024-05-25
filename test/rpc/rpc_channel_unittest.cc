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




// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/utility/logging.h>
#include "melon/utility/files/temp_file.h"
#include <melon/rpc/socket.h>
#include <melon/rpc/acceptor.h>
#include <melon/rpc/server.h>
#include <melon/rpc/policy/melon_rpc_protocol.h>
#include <melon/proto/rpc/melon_rpc_meta.pb.h>
#include <melon/rpc/policy/most_common_message.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/details/load_balancer_with_naming.h>
#include <melon/rpc/parallel_channel.h>
#include <melon/rpc/selective_channel.h>
#include <melon/rpc/socket_map.h>
#include <melon/rpc/controller.h>
#if BAZEL_TEST
#include "test/echo.pb.h"
#else
#include "echo.pb.h"
#endif   // BAZEL_TEST
#include <melon/proto/rpc/options.pb.h>

namespace melon {
DECLARE_int32(idle_timeout_second);
DECLARE_int32(max_connection_pool_size);
class Server;
class MethodStatus;
namespace policy {
void SendRpcResponse(int64_t correlation_id, Controller* cntl, 
                     const google::protobuf::Message* req,
                     const google::protobuf::Message* res,
                     const Server* server_raw, MethodStatus *, int64_t);
} // policy
} // melon

int main(int argc, char* argv[]) {
    melon::FLAGS_idle_timeout_second = 0;
    melon::FLAGS_max_connection_pool_size = 0;
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

namespace {
void* RunClosure(void* arg) {
    google::protobuf::Closure* done = (google::protobuf::Closure*)arg;
    done->Run();
    return NULL;
}

class DeleteOnlyOnceChannel : public melon::Channel {
public:
    DeleteOnlyOnceChannel() : _c(1) {
    }
    ~DeleteOnlyOnceChannel() {
        RELEASE_ASSERT_VERBOSE(_c.fetch_sub(1) == 1,
                               "Delete more than once!");
    }
private:
    mutil::atomic<int> _c;
};

static std::string MOCK_CREDENTIAL = "mock credential";
static std::string MOCK_CONTEXT = "mock context";

class MyAuthenticator : public melon::Authenticator {
public:
    MyAuthenticator() : count(0) {}

    int GenerateCredential(std::string* auth_str) const {
        *auth_str = MOCK_CREDENTIAL;
        count.fetch_add(1, mutil::memory_order_relaxed);
        return 0;
    }

    int VerifyCredential(const std::string&,
                         const mutil::EndPoint&,
                         melon::AuthContext* ctx) const {
        ctx->set_user(MOCK_CONTEXT);
        ctx->set_group(MOCK_CONTEXT);
        ctx->set_roles(MOCK_CONTEXT);
        ctx->set_starter(MOCK_CONTEXT);
        ctx->set_is_service(true);
        return 0;
    }
    mutable mutil::atomic<int32_t> count;
};

static bool VerifyMyRequest(const melon::InputMessageBase* msg_base) {
    const melon::policy::MostCommonMessage* msg =
        static_cast<const melon::policy::MostCommonMessage*>(msg_base);
    melon::Socket* ptr = msg->socket();
    
    melon::policy::RpcMeta meta;
    mutil::IOBufAsZeroCopyInputStream wrapper(msg->meta);
    EXPECT_TRUE(meta.ParseFromZeroCopyStream(&wrapper));

    if (meta.has_authentication_data()) {
        // Credential MUST only appear in the first packet
        EXPECT_TRUE(NULL == ptr->auth_context());
        EXPECT_EQ(meta.authentication_data(), MOCK_CREDENTIAL);
        MyAuthenticator authenticator;
        return authenticator.VerifyCredential(
            "", mutil::EndPoint(), ptr->mutable_auth_context()) == 0;
    }
    return true;
}

class CallAfterRpcObject {
public:
    explicit CallAfterRpcObject() {}

    ~CallAfterRpcObject() {
        EXPECT_EQ(str, "CallAfterRpcRespTest");
    }

    void Append(const std::string& s) {
        str.append(s);
    }

private:
    std::string str;
};

class MyEchoService : public ::test::EchoService {
    void Echo(google::protobuf::RpcController* cntl_base,
              const ::test::EchoRequest* req,
              ::test::EchoResponse* res,
              google::protobuf::Closure* done) {
        melon::Controller* cntl =
            static_cast<melon::Controller*>(cntl_base);
        std::shared_ptr<CallAfterRpcObject> str_test(new CallAfterRpcObject());
        cntl->set_after_rpc_resp_fn(std::bind(&MyEchoService::CallAfterRpc, str_test,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        melon::ClosureGuard done_guard(done);
        if (req->server_fail()) {
            cntl->SetFailed(req->server_fail(), "Server fail1");
            cntl->SetFailed(req->server_fail(), "Server fail2");
            return;
        }
        if (req->close_fd()) {
            MLOG(INFO) << "close fd...";
            cntl->CloseConnection("Close connection according to request");
            return;
        }
        if (req->sleep_us() > 0) {
            MLOG(INFO) << "sleep " << req->sleep_us() << "us...";
            fiber_usleep(req->sleep_us());
        }
        res->set_message("received " + req->message());
        if (req->code() != 0) {
            res->add_code_list(req->code());
        }
        res->set_receiving_socket_id(cntl->_current_call.sending_sock->id());
    }
    static void CallAfterRpc(std::shared_ptr<CallAfterRpcObject> str,
                        melon::Controller* cntl,
                        const google::protobuf::Message* req,
                        const google::protobuf::Message* res) {
        const test::EchoRequest* request = static_cast<const test::EchoRequest*>(req);
        const test::EchoResponse* response = static_cast<const test::EchoResponse*>(res);
        str->Append("CallAfterRpcRespTest");
        EXPECT_TRUE(nullptr != cntl);
        EXPECT_TRUE(nullptr != request);
        EXPECT_TRUE(nullptr != response);
    }
};

pthread_once_t register_mock_protocol = PTHREAD_ONCE_INIT;

class ChannelTest : public ::testing::Test{
protected:
    ChannelTest() 
        : _ep(mutil::IP_ANY, 8787)
        , _close_fd_once(false) {
        pthread_once(&register_mock_protocol, register_protocol);
        const melon::InputMessageHandler pairs[] = {
            { melon::policy::ParseMStdMessage,
              ProcessMStdRequest, VerifyMyRequest, this, "melon_std" }
        };
        EXPECT_EQ(0, _messenger.AddHandler(pairs[0]));

        EXPECT_EQ(0, _server_list.save(mutil::endpoint2str(_ep).c_str()));
        _naming_url = std::string("File://") + _server_list.fname();
    };

    virtual ~ChannelTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
        StopAndJoin();
    };

    static void register_protocol() {
        melon::Protocol dummy_protocol =
                                 { melon::policy::ParseMStdMessage,
                                   melon::SerializeRequestDefault,
                                   melon::policy::PackMStdRequest,
                                   NULL, ProcessMStdRequest,
                                   VerifyMyRequest, NULL, NULL,
                                   melon::CONNECTION_TYPE_ALL, "melon_std" };
        ASSERT_EQ(0,  RegisterProtocol((melon::ProtocolType)30, dummy_protocol));
    }

    static void ProcessMStdRequest(melon::InputMessageBase* msg_base) {
        melon::DestroyingPtr<melon::policy::MostCommonMessage> msg(
            static_cast<melon::policy::MostCommonMessage*>(msg_base));
        melon::SocketUniquePtr ptr(msg->ReleaseSocket());
        const melon::AuthContext* auth = ptr->auth_context();
        if (auth) {
            EXPECT_EQ(MOCK_CONTEXT, auth->user());
            EXPECT_EQ(MOCK_CONTEXT, auth->group());
            EXPECT_EQ(MOCK_CONTEXT, auth->roles());
            EXPECT_EQ(MOCK_CONTEXT, auth->starter());
            EXPECT_TRUE(auth->is_service());
        }
        ChannelTest* ts = (ChannelTest*)msg_base->arg();
        if (ts->_close_fd_once) {
            ts->_close_fd_once = false;
            ptr->SetFailed();
            return;
        }
        
        melon::policy::RpcMeta meta;
        mutil::IOBufAsZeroCopyInputStream wrapper(msg->meta);
        EXPECT_TRUE(meta.ParseFromZeroCopyStream(&wrapper));
        const melon::policy::RpcRequestMeta& req_meta = meta.request();
        ASSERT_EQ(ts->_svc.descriptor()->full_name(), req_meta.service_name());
        const google::protobuf::MethodDescriptor* method =
            ts->_svc.descriptor()->FindMethodByName(req_meta.method_name());
        google::protobuf::Message* req =
              ts->_svc.GetRequestPrototype(method).New();
        if (meta.attachment_size() != 0) {
            mutil::IOBuf req_buf;
            msg->payload.cutn(&req_buf, msg->payload.size() - meta.attachment_size());
            mutil::IOBufAsZeroCopyInputStream wrapper2(req_buf);
            EXPECT_TRUE(req->ParseFromZeroCopyStream(&wrapper2));
        } else {
            mutil::IOBufAsZeroCopyInputStream wrapper2(msg->payload);
            EXPECT_TRUE(req->ParseFromZeroCopyStream(&wrapper2));
        }
        melon::Controller* cntl = new melon::Controller();
        cntl->_current_call.peer_id = ptr->id();
        cntl->_current_call.sending_sock.reset(ptr.release());
        cntl->_server = &ts->_dummy;

        google::protobuf::Message* res =
              ts->_svc.GetResponsePrototype(method).New();
        google::protobuf::Closure* done =
              melon::NewCallback<
            int64_t, melon::Controller*,
            const google::protobuf::Message*,
            const google::protobuf::Message*,
            const melon::Server*,
            melon::MethodStatus*, int64_t>(
                &melon::policy::SendRpcResponse,
                meta.correlation_id(), cntl, req, res,
                &ts->_dummy, NULL, -1);
        ts->_svc.CallMethod(method, cntl, req, res, done);
    }

    int StartAccept(mutil::EndPoint ep) {
        int listening_fd = -1;
        while ((listening_fd = tcp_listen(ep)) < 0) {
            if (errno == EADDRINUSE) {
                fiber_usleep(1000);
            } else {
                return -1;
            }
        }
        if (_messenger.StartAccept(listening_fd, -1, NULL, false) != 0) {
            return -1;
        }
        return 0;
    }

    void StopAndJoin() {
        _messenger.StopAccept(0);
        _messenger.Join();
    }

    void SetUpChannel(melon::Channel* channel,
                      bool single_server,
                      bool short_connection,
                      const melon::Authenticator* auth = NULL,
                      std::string connection_group = std::string()) {
        melon::ChannelOptions opt;
        if (short_connection) {
            opt.connection_type = melon::CONNECTION_TYPE_SHORT;
        }
        opt.auth = auth;
        opt.max_retry = 0;
        opt.connection_group = connection_group;
        if (single_server) {
            EXPECT_EQ(0, channel->Init(_ep, &opt)); 
        } else {                                                 
            EXPECT_EQ(0, channel->Init(_naming_url.c_str(), "rR", &opt));
        }                                         
    }
    
    void CallMethod(melon::ChannelBase* channel,
                    melon::Controller* cntl,
                    test::EchoRequest* req, test::EchoResponse* res,
                    bool async, bool destroy = false) {
        google::protobuf::Closure* done = NULL;                     
        melon::CallId sync_id = { 0 };
        if (async) {
            sync_id = cntl->call_id();
            done = melon::DoNothing();
        }
        ::test::EchoService::Stub(channel).Echo(cntl, req, res, done);
        if (async) {
            if (destroy) {
                delete channel;
            }
            // Callback MUST be called for once and only once
            fiber_session_join(sync_id);
        }
    }

    void CallMethod(melon::ChannelBase* channel,
                    melon::Controller* cntl,
                    test::ComboRequest* req, test::ComboResponse* res,
                    bool async, bool destroy = false) {
        google::protobuf::Closure* done = NULL;
        melon::CallId sync_id = { 0 };
        if (async) {
            sync_id = cntl->call_id();
            done = melon::DoNothing();
        }
        ::test::EchoService::Stub(channel).ComboEcho(cntl, req, res, done);
        if (async) {
            if (destroy) {
                delete channel;
            }
            // Callback MUST be called for once and only once
            fiber_session_join(sync_id);
        }
    }

    void TestConnectionFailed(bool single_server, bool async, 
                              bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(ECONNREFUSED, cntl.ErrorCode()) << cntl.ErrorText();
    }
    
    void TestConnectionFailedParallel(bool single_server, bool async, 
                                      bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_TRUE(melon::ETOOMANYFAILS == cntl.ErrorCode() ||
                    ECONNREFUSED == cntl.ErrorCode()) << cntl.ErrorText();
        MLOG(INFO) << cntl.ErrorText();
    }

    void TestConnectionFailedSelective(bool single_server, bool async, 
                                       bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        const size_t NCHANS = 8;
        melon::SelectiveChannel channel;
        melon::ChannelOptions options;
        options.max_retry = 0;
        ASSERT_EQ(0, channel.Init("rr", &options));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(ECONNREFUSED, cntl.ErrorCode()) << cntl.ErrorText();
        ASSERT_EQ(1, cntl.sub_count());
        EXPECT_EQ(ECONNREFUSED, cntl.sub(0)->ErrorCode())
            << cntl.sub(0)->ErrorText();
        MLOG(INFO) << cntl.ErrorText();
    }
    
    void TestSuccess(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) 
            << single_server << ", " << async << ", " << short_connection;
        const uint64_t receiving_socket_id = res.receiving_socket_id();
        EXPECT_EQ(0, cntl.sub_count());
        EXPECT_TRUE(NULL == cntl.sub(-1));
        EXPECT_TRUE(NULL == cntl.sub(0));
        EXPECT_TRUE(NULL == cntl.sub(1));
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }
        if (single_server && !short_connection) {
            // Reuse the connection
            melon::Channel channel2;
            SetUpChannel(&channel2, single_server, short_connection);
            cntl.Reset();
            req.Clear();
            res.Clear();
            req.set_message(__FUNCTION__);
            CallMethod(&channel2, &cntl, &req, &res, async);
            EXPECT_EQ(0, cntl.ErrorCode())
                << single_server << ", " << async << ", " << short_connection;
            EXPECT_EQ(receiving_socket_id, res.receiving_socket_id());

            // A different connection_group does not reuse the connection
            melon::Channel channel3;
            SetUpChannel(&channel3, single_server, short_connection,
                         NULL, "another_group");
            cntl.Reset();
            req.Clear();
            res.Clear();
            req.set_message(__FUNCTION__);
            CallMethod(&channel3, &cntl, &req, &res, async);
            EXPECT_EQ(0, cntl.ErrorCode())
                << single_server << ", " << async << ", " << short_connection;
            const uint64_t receiving_socket_id2 = res.receiving_socket_id();
            EXPECT_NE(receiving_socket_id, receiving_socket_id2);

            // Channel in the same connection_group reuses the connection
            // note that the leading/trailing spaces should be trimed.
            melon::Channel channel4;
            SetUpChannel(&channel4, single_server, short_connection,
                         NULL, " another_group ");
            cntl.Reset();
            req.Clear();
            res.Clear();
            req.set_message(__FUNCTION__);
            CallMethod(&channel4, &cntl, &req, &res, async);
            EXPECT_EQ(0, cntl.ErrorCode())
                << single_server << ", " << async << ", " << short_connection;
            EXPECT_EQ(receiving_socket_id2, res.receiving_socket_id());
        }
        StopAndJoin();
    }

    class SetCode : public melon::CallMapper {
    public:
        melon::SubCall Map(
            int channel_index,
            const google::protobuf::MethodDescriptor* method,
            const google::protobuf::Message* req_base,
            google::protobuf::Message* response) {
            test::EchoRequest* req = melon::Clone<test::EchoRequest>(req_base);
            req->set_code(channel_index + 1/*non-zero*/);
            return melon::SubCall(method, req, response->New(),
                                melon::DELETE_REQUEST | melon::DELETE_RESPONSE);
        }
    };

    class SetCodeOnEven : public SetCode {
    public:
        melon::SubCall Map(
            int channel_index,
            const google::protobuf::MethodDescriptor* method,
            const google::protobuf::Message* req_base,
            google::protobuf::Message* response) {
            if (channel_index % 2) {
                return melon::SubCall::Skip();
            }
            return SetCode::Map(channel_index, method, req_base, response);
        }
    };


    class GetReqAndAddRes : public melon::CallMapper {
        melon::SubCall Map(
            int channel_index,
            const google::protobuf::MethodDescriptor* method,
            const google::protobuf::Message* req_base,
            google::protobuf::Message* res_base) {
            const test::ComboRequest* req =
                dynamic_cast<const test::ComboRequest*>(req_base);
            test::ComboResponse* res = dynamic_cast<test::ComboResponse*>(res_base);
            if (method->name() != "ComboEcho" ||
                res == NULL || req == NULL ||
                req->requests_size() <= channel_index) {
                return melon::SubCall::Bad();
            }
            return melon::SubCall(::test::EchoService::descriptor()->method(0),
                                &req->requests(channel_index),
                                res->add_responses(), 0);
        }
    };

    class MergeNothing : public melon::ResponseMerger {
        Result Merge(google::protobuf::Message* /*response*/,
                     const google::protobuf::Message* /*sub_response*/) {
            return melon::ResponseMerger::MERGED;
        }
    };

    void TestSuccessParallel(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          new SetCode, NULL));
        }
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_code(23);
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            EXPECT_TRUE(cntl.sub(i) && !cntl.sub(i)->Failed()) << "i=" << i;
        }
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        ASSERT_EQ(NCHANS, (size_t)res.code_list_size());
        for (size_t i = 0; i < NCHANS; ++i) {
            ASSERT_EQ((int)i+1, res.code_list(i));
        }
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }
        StopAndJoin();
    }

    void TestSuccessDuplicatedParallel(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        const size_t NCHANS = 8;
        melon::Channel* subchan = new DeleteOnlyOnceChannel;
        SetUpChannel(subchan, single_server, short_connection);
        melon::ParallelChannel channel;
        // Share the CallMapper and ResponseMerger should be fine because
        // they're intrusively shared.
        SetCode* set_code = new SetCode;
        for (size_t i = 0; i < NCHANS; ++i) {
            ASSERT_EQ(0, channel.AddChannel(
                          subchan,
                          // subchan should be deleted (for only once)
                          ((i % 2) ? melon::DOESNT_OWN_CHANNEL : melon::OWNS_CHANNEL),
                          set_code, NULL));
        }
        ASSERT_EQ((int)NCHANS, set_code->ref_count());
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_code(23);
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            EXPECT_TRUE(cntl.sub(i) && !cntl.sub(i)->Failed()) << "i=" << i;
        }
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        ASSERT_EQ(NCHANS, (size_t)res.code_list_size());
        for (size_t i = 0; i < NCHANS; ++i) {
            ASSERT_EQ((int)i+1, res.code_list(i));
        }
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }
        StopAndJoin();
    }
    
    void TestSuccessSelective(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        const size_t NCHANS = 8;
        ASSERT_EQ(0, StartAccept(_ep));
        melon::SelectiveChannel channel;
        melon::ChannelOptions options;
        options.max_retry = 0;
        ASSERT_EQ(0, channel.Init("rr", &options));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_code(23);
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(1, cntl.sub_count());
        ASSERT_EQ(0, cntl.sub(0)->ErrorCode());
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        ASSERT_EQ(1, res.code_list_size());
        ASSERT_EQ(req.code(), res.code_list(0));
        ASSERT_EQ(_ep, cntl.remote_side());
        
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }
        StopAndJoin();
    }

    void TestSkipParallel(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          new SetCodeOnEven, NULL));
        }
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_code(23);
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            if (i % 2) {
                EXPECT_TRUE(NULL == cntl.sub(i)) << "i=" << i;
            } else {
                EXPECT_TRUE(cntl.sub(i) && !cntl.sub(i)->Failed()) << "i=" << i;
            }
        }
        ASSERT_EQ(NCHANS / 2, (size_t)res.code_list_size());
        for (int i = 0; i < res.code_list_size(); ++i) {
            ASSERT_EQ(i*2 + 1, res.code_list(i));
        }
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }
        StopAndJoin();
    }

    void TestSuccessParallel2(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          new GetReqAndAddRes, new MergeNothing));
        }
        melon::Controller cntl;
        test::ComboRequest req;
        test::ComboResponse res;
        CallMethod(&channel, &cntl, &req, &res, false);
        ASSERT_TRUE(cntl.Failed()); // req does not have .requests
        ASSERT_EQ(melon::EREQUEST, cntl.ErrorCode());

        for (size_t i = 0; i < NCHANS; ++i) {
            ::test::EchoRequest* sub_req = req.add_requests();
            sub_req->set_message(mutil::string_printf("hello_%llu", (long long)i));
            sub_req->set_code(i + 1);
        }

        // non-parallel channel does not work.
        cntl.Reset();
        CallMethod(&subchans[0], &cntl, &req, &res, false);
        ASSERT_TRUE(cntl.Failed());
        ASSERT_EQ(melon::EINTERNAL, cntl.ErrorCode()) << cntl.ErrorText();
        ASSERT_TRUE(mutil::StringPiece(cntl.ErrorText()).ends_with("Method ComboEcho() not implemented."));

        // do the rpc call.
        cntl.Reset();
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        ASSERT_GT(cntl.latency_us(), 0);
        ASSERT_EQ((int)NCHANS, res.responses_size());
        for (int i = 0; i < res.responses_size(); ++i) {
            EXPECT_EQ(mutil::string_printf("received hello_%d", i),
                      res.responses(i).message());
            ASSERT_EQ(1, res.responses(i).code_list_size());
            EXPECT_EQ(i + 1, res.responses(i).code_list(0));
        }
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }
        StopAndJoin();
    }
    
    struct CancelerArg {
        int64_t sleep_before_cancel_us;
        melon::CallId cid;
    };

    static void* Canceler(void* void_arg) {
        CancelerArg* arg = static_cast<CancelerArg*>(void_arg);
        if (arg->sleep_before_cancel_us > 0) {
            fiber_usleep(arg->sleep_before_cancel_us);
        }
        MLOG(INFO) << "Start to cancel cid=" << arg->cid.value;
        melon::StartCancel(arg->cid);
        return NULL;
    }


    void CancelBeforeCallMethod(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        melon::StartCancel(cid);
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(ECANCELED, cntl.ErrorCode()) << cntl.ErrorText();
        StopAndJoin();
    }

    void CancelBeforeCallMethodParallel(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        melon::StartCancel(cid);
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(ECANCELED, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        EXPECT_TRUE(NULL == cntl.sub(1));
        EXPECT_TRUE(NULL == cntl.sub(0));
        StopAndJoin();
    }

    void CancelBeforeCallMethodSelective(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::SelectiveChannel channel;
        ASSERT_EQ(0, channel.Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        melon::StartCancel(cid);
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(ECANCELED, cntl.ErrorCode()) << cntl.ErrorText();
        StopAndJoin();
    }

    void CancelDuringCallMethod(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        pthread_t th;
        CancelerArg carg = { 10000, cid };
        ASSERT_EQ(0, pthread_create(&th, NULL, Canceler, &carg));
        req.set_sleep_us(carg.sleep_before_cancel_us * 2);
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_LT(labs(tm.u_elapsed() - carg.sleep_before_cancel_us), 10000);
        ASSERT_EQ(0, pthread_join(th, NULL));
        EXPECT_EQ(ECANCELED, cntl.ErrorCode());
        EXPECT_EQ(0, cntl.sub_count());
        EXPECT_TRUE(NULL == cntl.sub(1));
        EXPECT_TRUE(NULL == cntl.sub(0));
        StopAndJoin();
    }
    
    void CancelDuringCallMethodParallel(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        pthread_t th;
        CancelerArg carg = { 10000, cid };
        ASSERT_EQ(0, pthread_create(&th, NULL, Canceler, &carg));
        req.set_sleep_us(carg.sleep_before_cancel_us * 2);
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_LT(labs(tm.u_elapsed() - carg.sleep_before_cancel_us), 10000);
        ASSERT_EQ(0, pthread_join(th, NULL));
        EXPECT_EQ(ECANCELED, cntl.ErrorCode());
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            EXPECT_EQ(ECANCELED, cntl.sub(i)->ErrorCode()) << "i=" << i;
        }
        EXPECT_LT(labs(cntl.latency_us() - carg.sleep_before_cancel_us), 10000);
        StopAndJoin();
    }

    void CancelDuringCallMethodSelective(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::SelectiveChannel channel;
        ASSERT_EQ(0, channel.Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        pthread_t th;
        CancelerArg carg = { 10000, cid };
        ASSERT_EQ(0, pthread_create(&th, NULL, Canceler, &carg));
        req.set_sleep_us(carg.sleep_before_cancel_us * 2);
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_LT(labs(tm.u_elapsed() - carg.sleep_before_cancel_us), 10000);
        ASSERT_EQ(0, pthread_join(th, NULL));
        EXPECT_EQ(ECANCELED, cntl.ErrorCode());
        EXPECT_EQ(1, cntl.sub_count());
        EXPECT_EQ(ECANCELED, cntl.sub(0)->ErrorCode());
        StopAndJoin();
    }
    
    void CancelAfterCallMethod(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(0, cntl.ErrorCode());
        EXPECT_EQ(0, cntl.sub_count());
        ASSERT_EQ(EINVAL, fiber_session_error(cid, ECANCELED));
        StopAndJoin();
    }

    void CancelAfterCallMethodParallel(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        const melon::CallId cid = cntl.call_id();
        ASSERT_TRUE(cid.value != 0);
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(0, cntl.ErrorCode());
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            EXPECT_TRUE(cntl.sub(i) && !cntl.sub(i)->Failed()) << "i=" << i;
        }
        ASSERT_EQ(EINVAL, fiber_session_error(cid, ECANCELED));
        StopAndJoin();
    }

    void TestAttachment(bool async, bool short_connection) {
        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, true, short_connection);
                
        melon::Controller cntl;
        cntl.request_attachment().append("attachment");
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(0, cntl.ErrorCode())  << short_connection;
        EXPECT_FALSE(cntl.request_attachment().empty())
            << ", " << async << ", " << short_connection;
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        if (short_connection) {
            // Sleep to let `_messenger' detect `Socket' being `SetFailed'
            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            EXPECT_GE(1ul, _messenger.ConnectionCount());
        }            
        StopAndJoin();
    }

    void TestRequestNotInit(bool single_server, bool async,
                            bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(melon::EREQUEST, cntl.ErrorCode()) << cntl.ErrorText();
        StopAndJoin();
    }

    void TestRequestNotInitParallel(bool single_server, bool async,
                                    bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }
        
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(melon::EREQUEST, cntl.ErrorCode()) << cntl.ErrorText();
        MLOG(WARNING) << cntl.ErrorText();
        StopAndJoin();
    }

    void TestRequestNotInitSelective(bool single_server, bool async,
                                     bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::SelectiveChannel channel;
        ASSERT_EQ(0, channel.Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }
        
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(melon::EREQUEST, cntl.ErrorCode()) << cntl.ErrorText();
        MLOG(WARNING) << cntl.ErrorText();
        ASSERT_EQ(1, cntl.sub_count());
        ASSERT_EQ(melon::EREQUEST, cntl.sub(0)->ErrorCode());
        StopAndJoin();
    }
    
    void TestRPCTimeout(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_sleep_us(70000); // 70ms
        cntl.set_timeout_ms(17);
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_EQ(melon::ERPCTIMEDOUT, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_LT(labs(tm.m_elapsed() - cntl.timeout_ms()), 15);
        StopAndJoin();
    }

    void TestRPCTimeoutParallel(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));
        
        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        cntl.set_timeout_ms(17);
        req.set_sleep_us(70000); // 70ms
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_EQ(melon::ERPCTIMEDOUT, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            EXPECT_EQ(ECANCELED, cntl.sub(i)->ErrorCode()) << "i=" << i;
        }
        EXPECT_LT(labs(tm.m_elapsed() - cntl.timeout_ms()), 15);
        StopAndJoin();
    }

    class MakeTheRequestTimeout : public melon::CallMapper {
    public:
        melon::SubCall Map(
            int /*channel_index*/,
            const google::protobuf::MethodDescriptor* method,
            const google::protobuf::Message* req_base,
            google::protobuf::Message* response) {
            test::EchoRequest* req = melon::Clone<test::EchoRequest>(req_base);
            req->set_sleep_us(70000); // 70ms
            return melon::SubCall(method, req, response->New(),
                                melon::DELETE_REQUEST | melon::DELETE_RESPONSE);
        }
    };

    void TimeoutStillChecksSubChannelsParallel(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));
        
        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          ((i % 2) ? new MakeTheRequestTimeout : NULL), NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        cntl.set_timeout_ms(30);
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(NCHANS, (size_t)cntl.sub_count());
        for (int i = 0; i < cntl.sub_count(); ++i) {
            if (i % 2) {
                EXPECT_EQ(ECANCELED, cntl.sub(i)->ErrorCode());
            } else {
                EXPECT_EQ(0, cntl.sub(i)->ErrorCode());
            }
        }
        EXPECT_LT(labs(tm.m_elapsed() - cntl.timeout_ms()), 15);
        StopAndJoin();
    }

    void TestRPCTimeoutSelective(
        bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;
        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::SelectiveChannel channel;
        ASSERT_EQ(0, channel.Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        cntl.set_timeout_ms(17);
        req.set_sleep_us(70000); // 70ms
        mutil::Timer tm;
        tm.start();
        CallMethod(&channel, &cntl, &req, &res, async);
        tm.stop();
        EXPECT_EQ(melon::ERPCTIMEDOUT, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(1, cntl.sub_count());
        EXPECT_EQ(melon::ERPCTIMEDOUT, cntl.sub(0)->ErrorCode());
        EXPECT_LT(labs(tm.m_elapsed() - cntl.timeout_ms()), 15);
        EXPECT_EQ(-1, cntl.sub(0)->_timeout_ms);
        EXPECT_EQ(17, cntl.sub(0)->_real_timeout_ms);
        StopAndJoin();
    }
    
    void TestCloseFD(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_close_fd(true);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(melon::EEOF, cntl.ErrorCode()) << cntl.ErrorText();
        StopAndJoin();
    }

    void TestCloseFDParallel(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_close_fd(true);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_TRUE(melon::EEOF == cntl.ErrorCode() ||
                    melon::ETOOMANYFAILS == cntl.ErrorCode() ||
                    ECONNRESET == cntl.ErrorCode()) << cntl.ErrorText();
        StopAndJoin();
    }

    void TestCloseFDSelective(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::SelectiveChannel channel;
        melon::ChannelOptions options;
        options.max_retry = 0;
        ASSERT_EQ(0, channel.Init("rr", &options));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_close_fd(true);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(melon::EEOF, cntl.ErrorCode()) << cntl.ErrorText();
        ASSERT_EQ(1, cntl.sub_count());
        ASSERT_EQ(melon::EEOF, cntl.sub(0)->ErrorCode());

        StopAndJoin();
    }
    
    void TestServerFail(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_server_fail(melon::EINTERNAL);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(melon::EINTERNAL, cntl.ErrorCode()) << cntl.ErrorText();
        StopAndJoin();
    }

    void TestServerFailParallel(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 8;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (size_t i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_server_fail(melon::EINTERNAL);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(melon::EINTERNAL, cntl.ErrorCode()) << cntl.ErrorText();
        MLOG(INFO) << cntl.ErrorText();
        StopAndJoin();
    }

    void TestServerFailSelective(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));

        const size_t NCHANS = 5;
        melon::SelectiveChannel channel;
        ASSERT_EQ(0, channel.Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }

        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_server_fail(melon::EINTERNAL);
        CallMethod(&channel, &cntl, &req, &res, async);
        
        EXPECT_EQ(melon::EINTERNAL, cntl.ErrorCode()) << cntl.ErrorText();
        ASSERT_EQ(1, cntl.sub_count());
        ASSERT_EQ(melon::EINTERNAL, cntl.sub(0)->ErrorCode());

        MLOG(INFO) << cntl.ErrorText();
        StopAndJoin();
    }
    
    void TestDestroyChannel(bool single_server, bool short_connection) {
        std::cout << "*** single=" << single_server
                  << ", short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel* channel = new melon::Channel();
        SetUpChannel(channel, single_server, short_connection);
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        req.set_sleep_us(10000);
        CallMethod(channel, &cntl, &req, &res, true, true/*destroy*/);

        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        // Sleep to let `_messenger' detect `Socket' being `SetFailed'
        const int64_t start_time = mutil::gettimeofday_us();
        while (_messenger.ConnectionCount() != 0) {
            EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
            fiber_usleep(1000);
        }

        StopAndJoin();
    }
    
    void TestDestroyChannelParallel(bool single_server, bool short_connection) {
        std::cout << "*** single=" << single_server
                  << ", short=" << short_connection << std::endl;

        const size_t NCHANS = 5;
        ASSERT_EQ(0, StartAccept(_ep));
        melon::ParallelChannel* channel = new melon::ParallelChannel;
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel();
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel->AddChannel(
                          subchan, melon::OWNS_CHANNEL, NULL, NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_sleep_us(10000);
        req.set_message(__FUNCTION__);
        CallMethod(channel, &cntl, &req, &res, true, true/*destroy*/);
        
        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        // Sleep to let `_messenger' detect `Socket' being `SetFailed'
        const int64_t start_time = mutil::gettimeofday_us();
        while (_messenger.ConnectionCount() != 0) {
            EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
            fiber_usleep(1000);
        }
        StopAndJoin();
    }

    void TestDestroyChannelSelective(bool single_server, bool short_connection) {
        std::cout << "*** single=" << single_server
                  << ", short=" << short_connection << std::endl;

        const size_t NCHANS = 5;
        ASSERT_EQ(0, StartAccept(_ep));
        melon::SelectiveChannel* channel = new melon::SelectiveChannel;
        ASSERT_EQ(0, channel->Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel();
            SetUpChannel(subchan, single_server, short_connection);
            ASSERT_EQ(0, channel->AddChannel(subchan, NULL));
        }
                
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_sleep_us(10000);
        req.set_message(__FUNCTION__);
        CallMethod(channel, &cntl, &req, &res, true, true/*destroy*/);
        
        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
        ASSERT_EQ(_ep, cntl.remote_side());
        ASSERT_EQ(1, cntl.sub_count());
        ASSERT_EQ(0, cntl.sub(0)->ErrorCode());

        // Sleep to let `_messenger' detect `Socket' being `SetFailed'
        const int64_t start_time = mutil::gettimeofday_us();
        while (_messenger.ConnectionCount() != 0) {
            EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
            fiber_usleep(1000);
        }
        StopAndJoin();
    }

    void RPCThread(melon::ChannelBase* channel, bool async) {
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(__FUNCTION__);
        CallMethod(channel, &cntl, &req, &res, async);

        ASSERT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ("received " + std::string(__FUNCTION__), res.message());
    }

    void RPCThread(melon::ChannelBase* channel, bool async, int count) {
        melon::Controller cntl;
        for (int i = 0; i < count; ++i) {
            test::EchoRequest req;
            test::EchoResponse res;
            req.set_message(__FUNCTION__);
            CallMethod(channel, &cntl, &req, &res, async);

            ASSERT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
            ASSERT_EQ("received " + std::string(__FUNCTION__), res.message());
            cntl.Reset();
        }
    }

    void RPCThread(bool single_server, bool async, bool short_connection,
                   const melon::Authenticator* auth, int count) {
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection, auth);
        melon::Controller cntl;
        for (int i = 0; i < count; ++i) {
            test::EchoRequest req;
            test::EchoResponse res;
            req.set_message(__FUNCTION__);
            CallMethod(&channel, &cntl, &req, &res, async);

            ASSERT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
            ASSERT_EQ("received " + std::string(__FUNCTION__), res.message());
            cntl.Reset();
        }
    }

    void TestAuthentication(bool single_server, 
                            bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        MyAuthenticator auth;
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection, &auth);

        const int NUM = 10;
        pthread_t tids[NUM];
        for (int i = 0; i < NUM; ++i) {
            google::protobuf::Closure* thrd_func = 
                melon::NewCallback(
                    this, &ChannelTest::RPCThread, (melon::ChannelBase*)&channel, async);
            EXPECT_EQ(0, pthread_create(&tids[i], NULL,
                                        RunClosure, thrd_func));
        }
        for (int i = 0; i < NUM; ++i) {
            pthread_join(tids[i], NULL);
        }
        
        if (short_connection) {
            EXPECT_EQ(NUM, auth.count.load());
        } else {
            EXPECT_EQ(1, auth.count.load());
        }
        StopAndJoin();
    }

    void TestAuthenticationParallel(bool single_server, 
                                    bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        MyAuthenticator auth;

        const int NCHANS = 5;
        melon::Channel subchans[NCHANS];
        melon::ParallelChannel channel;
        for (int i = 0; i < NCHANS; ++i) {
            SetUpChannel(&subchans[i], single_server, short_connection, &auth);
            ASSERT_EQ(0, channel.AddChannel(
                          &subchans[i], melon::DOESNT_OWN_CHANNEL,
                          NULL, NULL));
        }
        
        const int NUM = 10;
        pthread_t tids[NUM];
        for (int i = 0; i < NUM; ++i) {
            google::protobuf::Closure* thrd_func = 
                melon::NewCallback(
                    this, &ChannelTest::RPCThread, (melon::ChannelBase*)&channel, async);
            EXPECT_EQ(0, pthread_create(&tids[i], NULL,
                                        RunClosure, thrd_func));
        }
        for (int i = 0; i < NUM; ++i) {
            pthread_join(tids[i], NULL);
        }
        
        if (short_connection) {
            EXPECT_EQ(NUM * NCHANS, auth.count.load());
        } else {
            EXPECT_EQ(1, auth.count.load());
        }
        StopAndJoin();
    }

    void TestAuthenticationSelective(bool single_server, 
                                    bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        MyAuthenticator auth;

        const size_t NCHANS = 5;
        melon::SelectiveChannel channel;
        ASSERT_EQ(0, channel.Init("rr", NULL));
        for (size_t i = 0; i < NCHANS; ++i) {
            melon::Channel* subchan = new melon::Channel;
            SetUpChannel(subchan, single_server, short_connection, &auth);
            ASSERT_EQ(0, channel.AddChannel(subchan, NULL)) << "i=" << i;
        }
        
        const int NUM = 10;
        pthread_t tids[NUM];
        for (int i = 0; i < NUM; ++i) {
            google::protobuf::Closure* thrd_func = 
                melon::NewCallback(
                    this, &ChannelTest::RPCThread, (melon::ChannelBase*)&channel, async);
            EXPECT_EQ(0, pthread_create(&tids[i], NULL,
                                        RunClosure, thrd_func));
        }
        for (int i = 0; i < NUM; ++i) {
            pthread_join(tids[i], NULL);
        }
        
        if (short_connection) {
            EXPECT_EQ(NUM, auth.count.load());
        } else {
            EXPECT_EQ(1, auth.count.load());
        }
        StopAndJoin();
    }
    
    void TestRetry(bool single_server, bool async, bool short_connection) {
        std::cout << " *** single=" << single_server
                  << " async=" << async
                  << " short=" << short_connection << std::endl;

        ASSERT_EQ(0, StartAccept(_ep));
        melon::Channel channel;
        SetUpChannel(&channel, single_server, short_connection);

        const int RETRY_NUM = 3;
        test::EchoRequest req;
        test::EchoResponse res;
        melon::Controller cntl;
        req.set_message(__FUNCTION__);

        // No retry when timeout
        cntl.set_max_retry(RETRY_NUM);
        cntl.set_timeout_ms(10);  // 10ms
        req.set_sleep_us(70000); // 70ms
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(melon::ERPCTIMEDOUT, cntl.ErrorCode()) << cntl.ErrorText();
        EXPECT_EQ(0, cntl.retried_count());
        fiber_usleep(100000);  // wait for the sleep task to finish

        // Retry when connection broken
        cntl.Reset();
        cntl.set_max_retry(RETRY_NUM);
        _close_fd_once = true;
        req.set_sleep_us(0);
        CallMethod(&channel, &cntl, &req, &res, async);

        if (short_connection) {
            // Always succeed
            EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
            EXPECT_EQ(1, cntl.retried_count());

            const int64_t start_time = mutil::gettimeofday_us();
            while (_messenger.ConnectionCount() != 0) {
                EXPECT_LT(mutil::gettimeofday_us(), start_time + 100000L/*100ms*/);
                fiber_usleep(1000);
            }
        } else {
            // May fail if health checker can't revive in time
            if (cntl.Failed()) {
                EXPECT_EQ(EHOSTDOWN, cntl.ErrorCode()) << single_server << ", " << async;
                EXPECT_EQ(RETRY_NUM, cntl.retried_count());
            } else {
                EXPECT_TRUE(cntl.retried_count() > 0);
            }
        }   
        StopAndJoin();
        fiber_usleep(100000);  // wait for stop
        
        // Retry when connection failed
        cntl.Reset();
        cntl.set_max_retry(RETRY_NUM);
        CallMethod(&channel, &cntl, &req, &res, async);
        EXPECT_EQ(EHOSTDOWN, cntl.ErrorCode());
        EXPECT_EQ(RETRY_NUM, cntl.retried_count());
    }

    void TestRetryOtherServer(bool async, bool short_connection) {
        ASSERT_EQ(0, StartAccept(_ep));

        melon::Channel channel;
        melon::ChannelOptions opt;
        opt.timeout_ms = 1000;
        if (short_connection) {
            opt.connection_type = melon::CONNECTION_TYPE_SHORT;
        }
        mutil::TempFile server_list;
        EXPECT_EQ(0, server_list.save_format(
                      "127.0.0.1:100\n"
                      "127.0.0.1:200\n"
                      "%s", endpoint2str(_ep).c_str()));
        std::string naming_url = std::string("fIle://")
            + server_list.fname();
        EXPECT_EQ(0, channel.Init(naming_url.c_str(), "RR", &opt)); 

        const int RETRY_NUM = 3;
        test::EchoRequest req;
        test::EchoResponse res;
        melon::Controller cntl;
        req.set_message(__FUNCTION__);
        cntl.set_max_retry(RETRY_NUM);
        CallMethod(&channel, &cntl, &req, &res, async);

        EXPECT_EQ(0, cntl.ErrorCode()) << async << ", " << short_connection;
        StopAndJoin();
    }

    struct TestRetryBackoffInfo {
        TestRetryBackoffInfo(ChannelTest* channel_test_param,
                             bool async_param,
                             bool short_connection_param,
                             bool fixed_backoff_param)
            : channel_test(channel_test_param)
            , async(async_param)
            , short_connection(short_connection_param)
            , fixed_backoff(fixed_backoff_param) {}

        ChannelTest* channel_test;
        int async;
        int short_connection;
        int fixed_backoff;
    };

    static void* TestRetryBackoffFiber(void* void_args) {
        auto args = static_cast<TestRetryBackoffInfo*>(void_args);
        args->channel_test->TestRetryBackoff(args->async, args->short_connection,
                                             args->fixed_backoff, false);
        return NULL;
    }

    void TestRetryBackoff(bool async, bool short_connection, bool fixed_backoff,
                          bool retry_backoff_in_pthread) {
        ASSERT_EQ(0, StartAccept(_ep));

        const int32_t backoff_time_ms = 100;
        const int32_t no_backoff_remaining_rpc_time_ms = 100;
        std::unique_ptr<melon::RetryPolicy> retry_ptr;
        if (fixed_backoff) {
            retry_ptr.reset(
                    new melon::RpcRetryPolicyWithFixedBackoff(backoff_time_ms,
                                                             no_backoff_remaining_rpc_time_ms,
                                                             retry_backoff_in_pthread));
        } else {
            retry_ptr.reset(
                    new melon::RpcRetryPolicyWithJitteredBackoff(backoff_time_ms,
                                                                backoff_time_ms + 20,
                                                                no_backoff_remaining_rpc_time_ms,
                                                                retry_backoff_in_pthread));
        }

        melon::Channel channel;
        melon::ChannelOptions opt;
        opt.timeout_ms = 1000;
        opt.retry_policy = retry_ptr.get();
        if (short_connection) {
            opt.connection_type = melon::CONNECTION_TYPE_SHORT;
        }
        mutil::TempFile server_list;
        EXPECT_EQ(0, server_list.save_format(
            "127.0.0.1:100\n"
            "127.0.0.1:200\n"
            "%s", endpoint2str(_ep).c_str()));
        std::string naming_url = std::string("fIle://")
            + server_list.fname();
        EXPECT_EQ(0, channel.Init(naming_url.c_str(), "RR", &opt));

        const int RETRY_NUM = 3;
        test::EchoRequest req;
        test::EchoResponse res;
        melon::Controller cntl;
        req.set_message(__FUNCTION__);
        cntl.set_max_retry(RETRY_NUM);
        CallMethod(&channel, &cntl, &req, &res, async);
        if (cntl.retried_count() > 0) {
            EXPECT_GT(cntl.latency_us(), ((int64_t)backoff_time_ms * 1000) * cntl.retried_count())
                << "latency_us=" << cntl.latency_us() << " retried_count=" << cntl.retried_count()
                << " enable_retry_backoff_in_pthread=" << retry_backoff_in_pthread;
        }
        EXPECT_EQ(0, cntl.ErrorCode()) << async << ", " << short_connection;
        StopAndJoin();
    }

    mutil::EndPoint _ep;
    mutil::TempFile _server_list;
    std::string _naming_url;
    
    melon::Acceptor _messenger;
    // Dummy server for `Server::AddError'
    melon::Server _dummy;
    std::string _mock_fail_str;

    bool _close_fd_once;
    
    MyEchoService _svc;
};

class MyShared : public melon::SharedObject {
public:
    MyShared() { ++ nctor; }
    MyShared(const MyShared&) : melon::SharedObject() { ++ nctor; }
    ~MyShared() override { ++ ndtor; }

    static int nctor;
    static int ndtor;
};
int MyShared::nctor = 0;
int MyShared::ndtor = 0;

TEST_F(ChannelTest, intrusive_ptr_sanity) {
    MyShared::nctor = 0;
    MyShared::ndtor = 0;
    {
        MyShared* s1 = new MyShared;
        ASSERT_EQ(0, s1->ref_count());
        mutil::intrusive_ptr<MyShared> p1 = s1;
        ASSERT_EQ(1, p1->ref_count());
        {
            mutil::intrusive_ptr<MyShared> p2 = s1;
            ASSERT_EQ(2, p2->ref_count());
            ASSERT_EQ(2, p1->ref_count());
        }
        ASSERT_EQ(1, p1->ref_count());
    }
    ASSERT_EQ(1, MyShared::nctor);
    ASSERT_EQ(1, MyShared::ndtor);
}

TEST_F(ChannelTest, init_as_single_server) {
    {
        melon::Channel channel;
        ASSERT_EQ(-1, channel.Init("127.0.0.1:12345:asdf", NULL));
        ASSERT_EQ(-1, channel.Init("127.0.0.1:99999", NULL)); 
        ASSERT_EQ(0, channel.Init("127.0.0.1:8888", NULL));
    }
    {
        melon::Channel channel;
        ASSERT_EQ(-1, channel.Init("127.0.0.1asdf", 12345, NULL));
        ASSERT_EQ(-1, channel.Init("127.0.0.1", 99999, NULL));
        ASSERT_EQ(0, channel.Init("127.0.0.1", 8888, NULL));
    }

    mutil::EndPoint ep;
    melon::Channel channel;
    ASSERT_EQ(0, str2endpoint("127.0.0.1:8888", &ep));
    ASSERT_EQ(0, channel.Init(ep, NULL));
    ASSERT_TRUE(channel.SingleServer());
    ASSERT_EQ(ep, channel._server_address);

    melon::SocketId id;
    ASSERT_EQ(0, melon::SocketMapFind(melon::SocketMapKey(ep), &id));
    ASSERT_EQ(id, channel._server_id);

    const int NUM = 10;
    melon::Channel channels[NUM];
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(0, channels[i].Init(ep, NULL));
        // Share the same server socket
        ASSERT_EQ(id, channels[i]._server_id);
    }
}

TEST_F(ChannelTest, init_using_unknown_naming_service) {
    melon::Channel channel;
    ASSERT_EQ(-1, channel.Init("unknown://unknown", "unknown", NULL));
}

TEST_F(ChannelTest, init_using_unexist_fns) {
    melon::Channel channel;
    ASSERT_EQ(-1, channel.Init("fiLe://no_such_file", "rr", NULL));
}

TEST_F(ChannelTest, init_using_empty_fns) {
    melon::ChannelOptions opt;
    opt.succeed_without_server = false;
    melon::Channel channel;
    mutil::TempFile server_list;
    ASSERT_EQ(0, server_list.save(""));
    std::string naming_url = std::string("file://") + server_list.fname();
    // empty file list results in error.
    ASSERT_EQ(-1, channel.Init(naming_url.c_str(), "rr", &opt));

    ASSERT_EQ(0, server_list.save("blahblah"));
    // No valid address.
    ASSERT_EQ(-1, channel.Init(naming_url.c_str(), "rr", NULL));
}

TEST_F(ChannelTest, init_using_empty_lns) {
    melon::ChannelOptions opt;
    opt.succeed_without_server = false;
    melon::Channel channel;
    ASSERT_EQ(-1, channel.Init("list:// ", "rr", &opt));
    ASSERT_EQ(-1, channel.Init("list://", "rr", &opt));
    ASSERT_EQ(-1, channel.Init("list://blahblah", "rr", &opt)); 
}

TEST_F(ChannelTest, init_using_naming_service) {
    melon::Channel* channel = new melon::Channel();
    mutil::TempFile server_list;
    ASSERT_EQ(0, server_list.save("127.0.0.1:8888"));
    std::string naming_url = std::string("filE://") + server_list.fname();
    // Rr are intended to test case-insensitivity.
    ASSERT_EQ(0, channel->Init(naming_url.c_str(), "Rr", NULL));
    ASSERT_FALSE(channel->SingleServer());

    melon::LoadBalancerWithNaming* lb =
        dynamic_cast<melon::LoadBalancerWithNaming*>(channel->_lb.get());
    ASSERT_TRUE(lb != NULL);
    melon::NamingServiceThread* ns = lb->_nsthread_ptr.get();

    {
        const int NUM = 10;
        melon::Channel channels[NUM];
        for (int i = 0; i < NUM; ++i) {
            // Share the same naming thread
            ASSERT_EQ(0, channels[i].Init(naming_url.c_str(), "rr", NULL));
            melon::LoadBalancerWithNaming* lb2 =
                dynamic_cast<melon::LoadBalancerWithNaming*>(channels[i]._lb.get());
            ASSERT_TRUE(lb2 != NULL);
            ASSERT_EQ(ns, lb2->_nsthread_ptr.get());
        }
    }

    // `lb' should be valid even if `channel' has destroyed
    // since we hold another reference to it
    mutil::intrusive_ptr<melon::SharedLoadBalancer>
        another_ctx = channel->_lb;
    delete channel;
    ASSERT_EQ(lb, another_ctx.get());
    ASSERT_EQ(1, another_ctx->_nref.load());
    // `lb' should be destroyed after
}

TEST_F(ChannelTest, parse_hostname) {
    melon::ChannelOptions opt;
    opt.succeed_without_server = false;
    opt.protocol = melon::PROTOCOL_HTTP;
    melon::Channel channel;

    ASSERT_EQ(-1, channel.Init("", 8888, &opt));
    ASSERT_EQ("", channel._service_name);
    ASSERT_EQ(-1, channel.Init("", &opt));
    ASSERT_EQ("", channel._service_name);

    ASSERT_EQ(0, channel.Init("http://127.0.0.1", 8888, &opt));
    ASSERT_EQ("127.0.0.1:8888", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://127.0.0.1:8888", &opt));
    ASSERT_EQ("127.0.0.1:8888", channel._service_name);

    ASSERT_EQ(0, channel.Init("localhost", 8888, &opt));
    ASSERT_EQ("localhost:8888", channel._service_name);
    ASSERT_EQ(0, channel.Init("localhost:8888", &opt));
    ASSERT_EQ("localhost:8888", channel._service_name);

    ASSERT_EQ(0, channel.Init("http://www.baidu.com", &opt));
    ASSERT_EQ("www.baidu.com", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com:80", &opt));
    ASSERT_EQ("www.baidu.com:80", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com", 80, &opt));
    ASSERT_EQ("www.baidu.com:80", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com:8888", &opt));
    ASSERT_EQ("www.baidu.com:8888", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com", 8888, &opt));
    ASSERT_EQ("www.baidu.com:8888", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com", "rr", &opt));
    ASSERT_EQ("www.baidu.com", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com:80", "rr", &opt));
    ASSERT_EQ("www.baidu.com:80", channel._service_name);
    ASSERT_EQ(0, channel.Init("http://www.baidu.com:8888", "rr", &opt));
    ASSERT_EQ("www.baidu.com:8888", channel._service_name);

    ASSERT_EQ(0, channel.Init("https://www.baidu.com", &opt));
    ASSERT_EQ("www.baidu.com", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com:443", &opt));
    ASSERT_EQ("www.baidu.com:443", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com", 443, &opt));
    ASSERT_EQ("www.baidu.com:443", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com:1443", &opt));
    ASSERT_EQ("www.baidu.com:1443", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com", 1443, &opt));
    ASSERT_EQ("www.baidu.com:1443", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com", "rr", &opt));
    ASSERT_EQ("www.baidu.com", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com:443", "rr", &opt));
    ASSERT_EQ("www.baidu.com:443", channel._service_name);
    ASSERT_EQ(0, channel.Init("https://www.baidu.com:1443", "rr", &opt));
    ASSERT_EQ("www.baidu.com:1443", channel._service_name);

    const char *address_list[] =  {
        "10.127.0.1:1234",
        "10.128.0.1:1234 enable",
        "10.129.0.1:1234",
        "localhost:1234",
        "www.baidu.com:1234"
    };
    mutil::TempFile tmp_file;
    {
        FILE* fp = fopen(tmp_file.fname(), "w");
        for (size_t i = 0; i < ARRAY_SIZE(address_list); ++i) {
            ASSERT_TRUE(fprintf(fp, "%s\n", address_list[i]));
        }
        fclose(fp);
    }
    melon::Channel ns_channel;
    std::string ns = std::string("file://") + tmp_file.fname();
    ASSERT_EQ(0, ns_channel.Init(ns.c_str(), "rr", &opt));
    ASSERT_EQ(tmp_file.fname(), ns_channel._service_name);
}

TEST_F(ChannelTest, connection_failed) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestConnectionFailed(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, empty_parallel_channel) {
    melon::ParallelChannel channel;

    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(__FUNCTION__);
    CallMethod(&channel, &cntl, &req, &res, false);
    EXPECT_EQ(EPERM, cntl.ErrorCode()) << cntl.ErrorText();
}

TEST_F(ChannelTest, empty_selective_channel) {
    melon::SelectiveChannel channel;
    ASSERT_EQ(0, channel.Init("rr", NULL));

    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(__FUNCTION__);
    CallMethod(&channel, &cntl, &req, &res, false);
    EXPECT_EQ(ENODATA, cntl.ErrorCode()) << cntl.ErrorText();
}

class BadCall : public melon::CallMapper {
    melon::SubCall Map(int,
                     const google::protobuf::MethodDescriptor*,
                     const google::protobuf::Message*,
                     google::protobuf::Message*) {
        return melon::SubCall::Bad();
    }
};

TEST_F(ChannelTest, returns_bad_parallel) {
    const size_t NCHANS = 5;
    melon::ParallelChannel channel;
    for (size_t i = 0; i < NCHANS; ++i) {
        melon::Channel* subchan = new melon::Channel();
        SetUpChannel(subchan, true, false);
        ASSERT_EQ(0, channel.AddChannel(
                      subchan, melon::OWNS_CHANNEL, new BadCall, NULL));
    }
                
    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(__FUNCTION__);
    CallMethod(&channel, &cntl, &req, &res, false);
    EXPECT_EQ(melon::EREQUEST, cntl.ErrorCode()) << cntl.ErrorText();
}

class SkipCall : public melon::CallMapper {
    melon::SubCall Map(int,
                     const google::protobuf::MethodDescriptor*,
                     const google::protobuf::Message*,
                     google::protobuf::Message*) {
        return melon::SubCall::Skip();
    }
};

TEST_F(ChannelTest, skip_all_channels) {
    const size_t NCHANS = 5;
    melon::ParallelChannel channel;
    for (size_t i = 0; i < NCHANS; ++i) {
        melon::Channel* subchan = new melon::Channel();
        SetUpChannel(subchan, true, false);
        ASSERT_EQ(0, channel.AddChannel(
                      subchan, melon::OWNS_CHANNEL, new SkipCall, NULL));
    }
                
    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(__FUNCTION__);
    CallMethod(&channel, &cntl, &req, &res, false);
        
    EXPECT_EQ(ECANCELED, cntl.ErrorCode()) << cntl.ErrorText();
    EXPECT_EQ((int)NCHANS, cntl.sub_count());
    for (int i = 0; i < cntl.sub_count(); ++i) {
        EXPECT_TRUE(NULL == cntl.sub(i)) << "i=" << i;
    }
}

TEST_F(ChannelTest, connection_failed_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestConnectionFailedParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, connection_failed_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestConnectionFailedSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, success) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestSuccess(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, success_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestSuccessParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, success_duplicated_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestSuccessDuplicatedParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, success_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestSuccessSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, skip_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestSkipParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, success_parallel2) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestSuccessParallel2(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_before_callmethod) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelBeforeCallMethod(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_before_callmethod_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelBeforeCallMethodParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_before_callmethod_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelBeforeCallMethodSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_during_callmethod) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelDuringCallMethod(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_during_callmethod_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelDuringCallMethodParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_during_callmethod_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelDuringCallMethodSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_after_callmethod) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelAfterCallMethod(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, cancel_after_callmethod_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                CancelAfterCallMethodParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, request_not_init) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRequestNotInit(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, request_not_init_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRequestNotInitParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, request_not_init_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRequestNotInitSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, timeout) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRPCTimeout(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, timeout_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRPCTimeoutParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, timeout_still_checks_sub_channels_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TimeoutStillChecksSubChannelsParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, timeout_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRPCTimeoutSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, close_fd) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestCloseFD(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, close_fd_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestCloseFDParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, close_fd_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestCloseFDSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, server_fail) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestServerFail(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, server_fail_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestServerFailParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, server_fail_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestServerFailSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, authentication) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestAuthentication(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, authentication_parallel) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestAuthenticationParallel(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, authentication_selective) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestAuthenticationSelective(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, retry) {
    for (int i = 0; i <= 1; ++i) { // Flag SingleServer 
        for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
            for (int k = 0; k <=1; ++k) { // Flag ShortConnection
                TestRetry(i, j, k);
            }
        }
    }
}

TEST_F(ChannelTest, retry_other_servers) {
    for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
        for (int k = 0; k <=1; ++k) { // Flag ShortConnection
            TestRetryOtherServer(j, k);
        }
    }
}

TEST_F(ChannelTest, retry_backoff) {
    for (int j = 0; j <= 1; ++j) { // Flag Asynchronous
        for (int k = 0; k <= 1; ++k) { // Flag ShortConnection
            for (int l = 0; l <= 1; ++l) { // Flag FixedRetryBackoffPolicy or JitteredRetryBackoffPolicy
                for (int m = 0; m <= 1; ++m) { // Flag retry backoff in fiber or pthread
                    if (m % 2 == 0) {
                        fiber_t th;
                        fiber_attr_t attr = FIBER_ATTR_NORMAL;
                        std::unique_ptr<TestRetryBackoffInfo> test_retry_backoff(
                                new TestRetryBackoffInfo(this, j, k, l));
                        // Retry backoff in fiber.
                        fiber_start_background(&th, &attr, TestRetryBackoffFiber, test_retry_backoff.get());
                        fiber_join(th, NULL);
                    } else {
                        // Retry backoff in pthread.
                        TestRetryBackoff(j, k, l, true);
                    }
                }
            }
        }
    }
}

TEST_F(ChannelTest, multiple_threads_single_channel) {
    srand(time(NULL));
    ASSERT_EQ(0, StartAccept(_ep));
    MyAuthenticator auth;
    const int NUM = 10;
    const int COUNT = 10000;
    pthread_t tids[NUM];

    // Cause massive connect/close log if setting to true
    bool short_connection = false;
    for (int single_server = 0; single_server <= 1; ++single_server) {
        for (int need_auth = 0; need_auth <= 1; ++need_auth) {
            for (int async = 0; async <= 1; ++async) {
                std::cout << " *** short=" << short_connection
                          << " single=" << single_server
                          << " auth=" << need_auth
                          << " async=" << async << std::endl;
                melon::Channel channel;
                SetUpChannel(&channel, single_server, 
                             short_connection, (need_auth ? &auth : NULL));
                for (int i = 0; i < NUM; ++i) {
                    google::protobuf::Closure* thrd_func = 
                        melon::NewCallback(
                            this, &ChannelTest::RPCThread, 
                            (melon::ChannelBase*)&channel,
                            (bool)async, COUNT);
                    EXPECT_EQ(0, pthread_create(&tids[i], NULL,
                                                RunClosure, thrd_func));
                }
                for (int i = 0; i < NUM; ++i) {
                    pthread_join(tids[i], NULL);
                }
            }
        }
    }
}

TEST_F(ChannelTest, multiple_threads_multiple_channels) {
    srand(time(NULL));
    ASSERT_EQ(0, StartAccept(_ep));
    MyAuthenticator auth;
    const int NUM = 10;
    const int COUNT = 10000;
    pthread_t tids[NUM];

    // Cause massive connect/close log if setting to true
    bool short_connection = false;

    for (int single_server = 0; single_server <= 1; ++single_server) {
        for (int need_auth = 0; need_auth <= 1; ++need_auth) {
            for (int async = 0; async <= 1; ++async) {
                std::cout << " *** short=" << short_connection
                          << " single=" << single_server
                          << " auth=" << need_auth
                          << " async=" << async << std::endl;
                for (int i = 0; i < NUM; ++i) {
                    google::protobuf::Closure* thrd_func = 
                        melon::NewCallback<
                        ChannelTest, ChannelTest*,
                        bool, bool, bool, const melon::Authenticator*, int>
                        (this, &ChannelTest::RPCThread, single_server,
                         async, short_connection, (need_auth ? &auth : NULL), COUNT);
                    EXPECT_EQ(0, pthread_create(&tids[i], NULL,
                                                RunClosure, thrd_func));
                }
                for (int i = 0; i < NUM; ++i) {
                    pthread_join(tids[i], NULL);
                }
            }
        }
    }
}

TEST_F(ChannelTest, clear_attachment_after_retry) {
    for (int j = 0; j <= 1; ++j) {
        for (int k = 0; k <= 1; ++k) {
            TestAttachment(j, k);
        }
    }
}

TEST_F(ChannelTest, destroy_channel) {
    for (int i = 0; i <= 1; ++i) {
        for (int j = 0; j <= 1; ++j) {
            TestDestroyChannel(i, j);
        }
    }
}

TEST_F(ChannelTest, destroy_channel_parallel) {
    for (int i = 0; i <= 1; ++i) {
        for (int j = 0; j <= 1; ++j) {
            TestDestroyChannelParallel(i, j);
        }
    }
}

TEST_F(ChannelTest, destroy_channel_selective) {
    for (int i = 0; i <= 1; ++i) {
        for (int j = 0; j <= 1; ++j) {
            TestDestroyChannelSelective(i, j);
        }
    }
}

TEST_F(ChannelTest, sizeof) {
    MLOG(INFO) << "Size of Channel is " << sizeof(melon::Channel)
               << ", Size of ParallelChannel is " << sizeof(melon::ParallelChannel)
               << ", Size of Controller is " << sizeof(melon::Controller)
               << ", Size of vector is " << sizeof(std::vector<melon::Controller>);
}

melon::Channel g_chan;

TEST_F(ChannelTest, global_channel_should_quit_successfully) {
    g_chan.Init("bns://qa-pbrpc.SAT.tjyx", "rr", NULL);
}

TEST_F(ChannelTest, unused_call_id) {
    {
        melon::Controller cntl;
    }
    {
        melon::Controller cntl;
        cntl.Reset();
    }
    melon::CallId cid1 = { 0 };
    {
        melon::Controller cntl;
        cid1 = cntl.call_id();
    }
    ASSERT_EQ(EINVAL, fiber_session_error(cid1, ECANCELED));

    {
        melon::CallId cid2 = { 0 };
        melon::Controller cntl;
        cid2 = cntl.call_id();
        cntl.Reset();
        ASSERT_EQ(EINVAL, fiber_session_error(cid2, ECANCELED));
    }
}

TEST_F(ChannelTest, adaptive_connection_type) {
    melon::AdaptiveConnectionType ctype;
    ASSERT_EQ(melon::CONNECTION_TYPE_UNKNOWN, ctype);
    ASSERT_FALSE(ctype.has_error());
    ASSERT_STREQ("unknown", ctype.name());

    ctype = melon::CONNECTION_TYPE_SINGLE;
    ASSERT_EQ(melon::CONNECTION_TYPE_SINGLE, ctype);
    ASSERT_STREQ("single", ctype.name());

    ctype = "shorT";
    ASSERT_EQ(melon::CONNECTION_TYPE_SHORT, ctype);
    ASSERT_STREQ("short", ctype.name());
    
    ctype = "PooLed";
    ASSERT_EQ(melon::CONNECTION_TYPE_POOLED, ctype);
    ASSERT_STREQ("pooled", ctype.name());

    ctype = "SINGLE";
    ASSERT_EQ(melon::CONNECTION_TYPE_SINGLE, ctype);
    ASSERT_FALSE(ctype.has_error());
    ASSERT_STREQ("single", ctype.name());

    ctype = "blah";
    ASSERT_EQ(melon::CONNECTION_TYPE_UNKNOWN, ctype);
    ASSERT_TRUE(ctype.has_error());
    ASSERT_STREQ("unknown", ctype.name());

    ctype = "single";
    ASSERT_EQ(melon::CONNECTION_TYPE_SINGLE, ctype);
    ASSERT_FALSE(ctype.has_error());
    ASSERT_STREQ("single", ctype.name());
}

TEST_F(ChannelTest, adaptive_protocol_type) {
    melon::AdaptiveProtocolType ptype;
    ASSERT_EQ(melon::PROTOCOL_UNKNOWN, ptype);
    ASSERT_STREQ("unknown", ptype.name());
    ASSERT_FALSE(ptype.has_param());
    ASSERT_EQ("", ptype.param());

    ptype = melon::PROTOCOL_HTTP;
    ASSERT_EQ(melon::PROTOCOL_HTTP, ptype);
    ASSERT_STREQ("http", ptype.name());
    ASSERT_FALSE(ptype.has_param());
    ASSERT_EQ("", ptype.param());

    ptype = "http:xyz ";
    ASSERT_EQ(melon::PROTOCOL_HTTP, ptype);
    ASSERT_STREQ("http", ptype.name());
    ASSERT_TRUE(ptype.has_param());
    ASSERT_EQ("xyz ", ptype.param());

    ptype = "HuLu_pbRPC";
    ASSERT_EQ(melon::PROTOCOL_HULU_PBRPC, ptype);
    ASSERT_STREQ("hulu_pbrpc", ptype.name());
    ASSERT_FALSE(ptype.has_param());
    ASSERT_EQ("", ptype.param());
    
    ptype = "blah";
    ASSERT_EQ(melon::PROTOCOL_UNKNOWN, ptype);
    ASSERT_STREQ("blah", ptype.name());
    ASSERT_FALSE(ptype.has_param());
    ASSERT_EQ("", ptype.param());

    ptype = "Melon_STD";
    ASSERT_EQ(melon::PROTOCOL_MELON_STD, ptype);
    ASSERT_STREQ("melon_std", ptype.name());
    ASSERT_FALSE(ptype.has_param());
    ASSERT_EQ("", ptype.param());
}

} //namespace
