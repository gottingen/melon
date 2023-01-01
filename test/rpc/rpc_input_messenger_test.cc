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



// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>                   //
#include "testing/gtest_wrap.h"
#include "melon/base/gperftools_profiler.h"
#include "melon/times/time.h"
#include "melon/base/fd_utility.h"
#include "melon/base/unix_socket.h"
#include "melon/base/fd_guard.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/policy/hulu_pbrpc_protocol.h"

void EmptyProcessHuluRequest(melon::rpc::InputMessageBase *msg_base) {
    melon::rpc::DestroyingPtr<melon::rpc::InputMessageBase> a(msg_base);
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    melon::rpc::Protocol dummy_protocol =
            {melon::rpc::policy::ParseHuluMessage,
             melon::rpc::SerializeRequestDefault,
             melon::rpc::policy::PackHuluRequest,
             EmptyProcessHuluRequest, EmptyProcessHuluRequest,
             nullptr, nullptr, nullptr,
             melon::rpc::CONNECTION_TYPE_ALL, "dummy_hulu"};
    EXPECT_EQ(0, RegisterProtocol((melon::rpc::ProtocolType) 30, dummy_protocol));
    return RUN_ALL_TESTS();
}

class MessengerTest : public ::testing::Test {
protected:
    MessengerTest() {
    };

    virtual ~MessengerTest() {};

    virtual void SetUp() {
    };

    virtual void TearDown() {
    };
};

#define USE_UNIX_DOMAIN_SOCKET 1

const size_t NEPOLL = 1;
const size_t NCLIENT = 6;
const size_t NMESSAGE = 1024;
const size_t MESSAGE_SIZE = 32;

inline uint32_t fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

volatile bool client_stop = false;

struct MELON_CACHELINE_ALIGNMENT ClientMeta {
    size_t times;
    size_t bytes;
};

std::atomic<size_t> client_index(0);

void *client_thread(void *arg) {
    ClientMeta *m = (ClientMeta *) arg;
    size_t offset = 0;
    m->times = 0;
    m->bytes = 0;
    const size_t buf_cap = NMESSAGE * MESSAGE_SIZE;
    char *buf = (char *) malloc(buf_cap);
    for (size_t i = 0; i < NMESSAGE; ++i) {
        memcpy(buf + i * MESSAGE_SIZE, "HULU", 4);
        // HULU use host byte order directly...
        *(uint32_t *) (buf + i * MESSAGE_SIZE + 4) = MESSAGE_SIZE - 12;
        *(uint32_t *) (buf + i * MESSAGE_SIZE + 8) = 4;
    }
#ifdef USE_UNIX_DOMAIN_SOCKET
    const size_t id = client_index.fetch_add(1);
    char socket_name[64];
    snprintf(socket_name, sizeof(socket_name), "input_messenger.socket%lu",
             (id % NEPOLL));
    melon::base::fd_guard fd(melon::base::unix_socket_connect(socket_name));
    if (fd < 0) {
        MELON_PLOG(FATAL) << "Fail to connect to " << socket_name;
        return nullptr;
    }
#else
    melon::base::end_point point(melon::base::IP_ANY, 7878);
    melon::base::fd_guard fd(melon::base::tcp_connect(point, nullptr));
    if (fd < 0) {
        MELON_PLOG(FATAL) << "Fail to connect to " << point;
        return nullptr;
    }
#endif

    while (!client_stop) {
        ssize_t n;
        if (offset == 0) {
            n = write(fd, buf, buf_cap);
        } else {
            iovec v[2];
            v[0].iov_base = buf + offset;
            v[0].iov_len = buf_cap - offset;
            v[1].iov_base = buf;
            v[1].iov_len = offset;
            n = writev(fd, v, 2);
        }
        if (n < 0) {
            if (errno != EINTR) {
                MELON_PLOG(FATAL) << "Fail to write fd=" << fd;
                return nullptr;
            }
        } else {
            ++m->times;
            m->bytes += n;
            offset += n;
            if (offset >= buf_cap) {
                offset -= buf_cap;
            }
        }
    }
    return nullptr;
}

TEST_F(MessengerTest, dispatch_tasks) {
    client_stop = false;

    melon::rpc::Acceptor messenger[NEPOLL];
    pthread_t cth[NCLIENT];
    ClientMeta *cm[NCLIENT];

    const melon::rpc::InputMessageHandler pairs[] = {
            {melon::rpc::policy::ParseHuluMessage,
                    EmptyProcessHuluRequest, nullptr, nullptr, "dummy_hulu"}
    };

    for (size_t i = 0; i < NEPOLL; ++i) {
#ifdef USE_UNIX_DOMAIN_SOCKET
        char buf[64];
        snprintf(buf, sizeof(buf), "input_messenger.socket%lu", i);
        int listening_fd = melon::base::unix_socket_listen(buf);
#else
        int listening_fd = tcp_listen(melon::base::end_point(melon::base::IP_ANY, 7878));
#endif
        ASSERT_TRUE(listening_fd > 0);
        melon::base::make_non_blocking(listening_fd);
        ASSERT_EQ(0, messenger[i].AddHandler(pairs[0]));
        ASSERT_EQ(0, messenger[i].StartAccept(listening_fd, -1, nullptr));
    }

    for (size_t i = 0; i < NCLIENT; ++i) {
        cm[i] = new ClientMeta;
        cm[i]->times = 0;
        cm[i]->bytes = 0;
        ASSERT_EQ(0, pthread_create(&cth[i], nullptr, client_thread, cm[i]));
    }

    sleep(1);

    MELON_LOG(INFO) << "Begin to profile... (5 seconds)";
    ProfilerStart("input_messenger.prof");

    size_t start_client_bytes = 0;
    for (size_t i = 0; i < NCLIENT; ++i) {
        start_client_bytes += cm[i]->bytes;
    }
    melon::stop_watcher tm;
    tm.start();

    sleep(5);

    tm.stop();
    ProfilerStop();
    MELON_LOG(INFO) << "End profiling";

    client_stop = true;

    size_t client_bytes = 0;
    for (size_t i = 0; i < NCLIENT; ++i) {
        client_bytes += cm[i]->bytes;
    }
    MELON_LOG(INFO) << "client_tp=" << (client_bytes - start_client_bytes) / (double) tm.u_elapsed()
              << "MB/s client_msg="
              << (client_bytes - start_client_bytes) * 1000000L / (MESSAGE_SIZE * tm.u_elapsed())
              << "/s";

    for (size_t i = 0; i < NCLIENT; ++i) {
        pthread_join(cth[i], nullptr);
        printf("joined client %lu\n", i);
    }
    for (size_t i = 0; i < NEPOLL; ++i) {
        messenger[i].StopAccept(0);
    }
    sleep(1);
    MELON_LOG(WARNING) << "begin to exit!!!!";
}