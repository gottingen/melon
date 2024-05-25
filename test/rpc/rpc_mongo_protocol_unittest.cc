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




// Date: Thu Oct 15 21:08:31 CST 2015

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/acceptor.h>
#include <melon/rpc/server.h>
#include <melon/rpc/policy/mongo_protocol.h>
#include <melon/rpc/policy/most_common_message.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/mongo/mongo_head.h>
#include <melon/rpc/mongo/mongo_service_adaptor.h>
#include <melon/proto/rpc/mongo.pb.h>

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

namespace {

static const std::string EXP_REQUEST = "hello";
static const std::string EXP_RESPONSE = "world";

class MyEchoService : public ::melon::policy::MongoService {
    void default_method(::google::protobuf::RpcController*,
                        const ::melon::policy::MongoRequest* req,
                        ::melon::policy::MongoResponse* res,
                        ::google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);

        EXPECT_EQ(EXP_REQUEST, req->message());

        res->mutable_header()->set_message_length(
                sizeof(melon::mongo_head_t) + sizeof(int32_t) * 3 +
                sizeof(int64_t) + EXP_REQUEST.length());
        res->set_message(EXP_RESPONSE);
    }
};

class MyContext : public ::melon::MongoContext {
};

class MyMongoAdaptor : public melon::MongoServiceAdaptor {
public:
    virtual void SerializeError(int /*response_to*/, mutil::IOBuf* out_buf) const {
        melon::mongo_head_t header = {
            (int32_t)(sizeof(melon::mongo_head_t) + sizeof(int32_t) * 3 +
                sizeof(int64_t) + EXP_REQUEST.length()),
            0, 0, 0};
        out_buf->append(static_cast<const void*>(&header), sizeof(melon::mongo_head_t));
        int32_t response_flags = 0;
        int64_t cursor_id = 0;
        int32_t starting_from = 0;
        int32_t number_returned = 0;
        out_buf->append(&response_flags, sizeof(response_flags));
        out_buf->append(&cursor_id, sizeof(cursor_id));
        out_buf->append(&starting_from, sizeof(starting_from));
        out_buf->append(&number_returned, sizeof(number_returned));
        out_buf->append(EXP_RESPONSE);
    }

    virtual ::melon::MongoContext* CreateSocketContext() const {
        return new MyContext;
    }
};

class MongoTest : public ::testing::Test{
protected:
    MongoTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, melon::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = melon::Server::RUNNING;
        _server._options.mongo_service_adaptor = &_adaptor;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        melon::SocketId id;
        melon::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, melon::Socket::Create(options, &id));
        EXPECT_EQ(0, melon::Socket::Address(id, &_socket));
    };

    virtual ~MongoTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void ProcessMessage(void (*process)(melon::InputMessageBase*),
                        melon::InputMessageBase* msg, bool set_eof) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        _socket->PostponeEOF();
        if (set_eof) {
            _socket->SetEOF();
        }
        (*process)(msg);
    }

    melon::policy::MostCommonMessage* MakeRequestMessage(
        melon::mongo_head_t* head) {
        head->message_length = sizeof(head) + EXP_REQUEST.length();
        head->op_code = melon::MONGO_OPCODE_REPLY;
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        msg->meta.append(&head, sizeof(head));
        msg->payload.append(EXP_REQUEST);
        return msg;
    }

    void CheckEmptyResponse() {
        int bytes_in_pipe = 0;
        ioctl(_pipe_fds[0], FIONREAD, &bytes_in_pipe);
        EXPECT_EQ(0, bytes_in_pipe);
    }

    int _pipe_fds[2];
    melon::SocketUniquePtr _socket;
    melon::Server _server;
    MyMongoAdaptor _adaptor;

    MyEchoService _svc;
};

TEST_F(MongoTest, process_request_logoff) {
    melon::mongo_head_t header = { 0, 0, 0, 0 };
    header.op_code = melon::MONGO_OPCODE_REPLY;
    header.message_length = sizeof(header) + EXP_REQUEST.length();
    mutil::IOBuf total_buf;
    total_buf.append(static_cast<const void*>(&header), sizeof(header));
    total_buf.append(EXP_REQUEST);
    melon::ParseResult req_pr = melon::policy::ParseMongoMessage(
        &total_buf, _socket.get(), false, &_server);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    _server._status = melon::Server::READY;
    ProcessMessage(melon::policy::ProcessMongoRequest, req_pr.message(), false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
}

TEST_F(MongoTest, process_request_failed_socket) {
    melon::mongo_head_t header = { 0, 0, 0, 0 };
    header.op_code = melon::MONGO_OPCODE_REPLY;
    header.message_length = sizeof(header) + EXP_REQUEST.length();
    mutil::IOBuf total_buf;
    total_buf.append(static_cast<const void*>(&header), sizeof(header));
    total_buf.append(EXP_REQUEST);
    melon::ParseResult req_pr = melon::policy::ParseMongoMessage(
        &total_buf, _socket.get(), false, &_server);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    _socket->SetFailed();
    ProcessMessage(melon::policy::ProcessMongoRequest, req_pr.message(), false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
}

TEST_F(MongoTest, complete_flow) {
    mutil::IOBuf request_buf;
    mutil::IOBuf total_buf;
    melon::Controller cntl;
    melon::policy::MongoRequest req;
    melon::policy::MongoResponse res;
    cntl._response = &res;

    // Assemble request
    melon::mongo_head_t header = { 0, 0, 0, 0 };
    header.message_length = sizeof(header) + EXP_REQUEST.length();
    total_buf.append(static_cast<const void*>(&header), sizeof(header));
    total_buf.append(EXP_REQUEST);

    const size_t old_size = total_buf.size();
    // Handle request
    melon::ParseResult req_pr =
            melon::policy::ParseMongoMessage(&total_buf, _socket.get(), false, &_server);
    // head.op_code does not fit.
    ASSERT_EQ(melon::PARSE_ERROR_TRY_OTHERS, req_pr.error());
    // no data should be consumed.
    ASSERT_EQ(old_size, total_buf.size());
    header.op_code = melon::MONGO_OPCODE_REPLY;
    total_buf.clear();
    total_buf.append(static_cast<const void*>(&header), sizeof(header));
    total_buf.append(EXP_REQUEST);
    req_pr = melon::policy::ParseMongoMessage(&total_buf, _socket.get(), false, &_server);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    ProcessMessage(melon::policy::ProcessMongoRequest, req_msg, false);

    // Read response from pipe
    mutil::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    char buf[sizeof(melon::mongo_head_t)];
    const melon::mongo_head_t *phead = static_cast<const melon::mongo_head_t*>(
            response_buf.fetch(buf, sizeof(buf)));
    response_buf.cutn(&header, sizeof(header));
    response_buf.cutn(buf, sizeof(int32_t));
    response_buf.cutn(buf, sizeof(int64_t));
    response_buf.cutn(buf, sizeof(int32_t));
    response_buf.cutn(buf, sizeof(int32_t));
    char msg_buf[200];
    int msg_length = phead->message_length - sizeof(melon::mongo_head_t) - sizeof(int32_t) * 3 -
        sizeof(int64_t);
    response_buf.cutn(msg_buf, msg_length);
    msg_buf[msg_length] = '\0';

    ASSERT_FALSE(cntl.Failed());
    ASSERT_STREQ(EXP_RESPONSE.c_str(), msg_buf);
}
} //namespace
