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




// Date: Tue Oct 9 20:27:18 CST 2018

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <melon/fiber/fiber.h>
#include <melon/utility/atomicops.h>
#include <melon/rpc/policy/http_rpc_protocol.h>
#include <melon/rpc/policy/http2_rpc_protocol.h>
#include <melon/utility/gperftools_profiler.h>

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(H2UnsentMessage, request_throughput) {
    melon::Controller cntl;
    mutil::IOBuf request_buf;
    cntl.http_request().uri() = "0.0.0.0:8010/HttpService/Echo";
    melon::policy::SerializeHttpRequest(&request_buf, &cntl, NULL);

    melon::SocketId id;
    melon::SocketUniquePtr h2_client_sock;
    melon::SocketOptions h2_client_options;
    h2_client_options.user = melon::get_client_side_messenger();
    EXPECT_EQ(0, melon::Socket::Create(h2_client_options, &id));
    EXPECT_EQ(0, melon::Socket::Address(id, &h2_client_sock));

    melon::policy::H2Context* ctx =
        new melon::policy::H2Context(h2_client_sock.get(), NULL);
    MCHECK_EQ(ctx->Init(), 0);
    h2_client_sock->initialize_parsing_context(&ctx);
    ctx->_last_sent_stream_id = 0;
    ctx->_remote_window_left = melon::H2Settings::MAX_WINDOW_SIZE;

    int64_t ntotal = 500000;

    // calc H2UnsentRequest throughput
    mutil::IOBuf dummy_buf;
    ProfilerStart("h2_unsent_req.prof");
    int64_t start_us = mutil::gettimeofday_us();
    for (int i = 0; i < ntotal; ++i) {
        melon::policy::H2UnsentRequest* req = melon::policy::H2UnsentRequest::New(&cntl);
        req->AppendAndDestroySelf(&dummy_buf, h2_client_sock.get());
    }
    int64_t end_us = mutil::gettimeofday_us();
    ProfilerStop();
    int64_t elapsed = end_us - start_us;
    MLOG(INFO) << "H2UnsentRequest average qps="
        << (ntotal * 1000000L) / elapsed << "/s, data throughput="
        << dummy_buf.size() * 1000000L / elapsed << "/s";

    // calc H2UnsentResponse throughput
    dummy_buf.clear();
    start_us = mutil::gettimeofday_us();
    for (int i = 0; i < ntotal; ++i) {
        // H2UnsentResponse::New would release cntl.http_response() and swap
        // cntl.response_attachment()
        cntl.http_response().set_content_type("text/plain");
        cntl.response_attachment().append("0123456789abcedef");
        melon::policy::H2UnsentResponse* res = melon::policy::H2UnsentResponse::New(&cntl, 0, false);
        res->AppendAndDestroySelf(&dummy_buf, h2_client_sock.get());
    }
    end_us = mutil::gettimeofday_us();
    elapsed = end_us - start_us;
    MLOG(INFO) << "H2UnsentResponse average qps="
        << (ntotal * 1000000L) / elapsed << "/s, data throughput="
        << dummy_buf.size() * 1000000L / elapsed << "/s";
}
