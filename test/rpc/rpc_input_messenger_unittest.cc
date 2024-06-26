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
#include <netdb.h>                   //
#include <gtest/gtest.h>
#include <melon/utility/gperftools_profiler.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/utility/fd_utility.h>
#include <melon/utility/fd_guard.h>
#include "melon/utility/unix_socket.h"
#include <melon/rpc/acceptor.h>
#include <melon/rpc/policy/hulu_pbrpc_protocol.h>

void EmptyProcessHuluRequest(melon::InputMessageBase* msg_base) {
    melon::DestroyingPtr<melon::InputMessageBase> a(msg_base);
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    melon::Protocol dummy_protocol =
                             { melon::policy::ParseHuluMessage,
                               melon::SerializeRequestDefault,
                               melon::policy::PackHuluRequest,
                               EmptyProcessHuluRequest, EmptyProcessHuluRequest,
                               NULL, NULL, NULL,
                               melon::CONNECTION_TYPE_ALL, "dummy_hulu" };
    EXPECT_EQ(0,  RegisterProtocol((melon::ProtocolType)30, dummy_protocol));
    return RUN_ALL_TESTS();
}

class MessengerTest : public ::testing::Test{
protected:
    MessengerTest(){
    };
    virtual ~MessengerTest(){};
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

inline uint32_t fmix32 ( uint32_t h ) {
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

mutil::atomic<size_t> client_index(0);

void* client_thread(void* arg) {
    ClientMeta* m = (ClientMeta*)arg;
    size_t offset = 0;
    m->times = 0;
    m->bytes = 0;
    const size_t buf_cap = NMESSAGE * MESSAGE_SIZE;
    char* buf = (char*)malloc(buf_cap);
    for (size_t i = 0; i < NMESSAGE; ++i) {
        memcpy(buf + i * MESSAGE_SIZE, "HULU", 4);
        // HULU use host byte order directly...
        *(uint32_t*)(buf + i * MESSAGE_SIZE + 4) = MESSAGE_SIZE - 12;
        *(uint32_t*)(buf + i * MESSAGE_SIZE + 8) = 4;
    }
#ifdef USE_UNIX_DOMAIN_SOCKET
    const size_t id = client_index.fetch_add(1);
    char socket_name[64];
    snprintf(socket_name, sizeof(socket_name), "input_messenger.socket%lu",
             (id % NEPOLL));
    mutil::fd_guard fd(mutil::unix_socket_connect(socket_name));
    if (fd < 0) {
        PLOG(FATAL) << "Fail to connect to " << socket_name;
        return NULL;
    }
#else
    mutil::EndPoint point(mutil::IP_ANY, 7878);
    mutil::fd_guard fd(mutil::tcp_connect(point, NULL));
    if (fd < 0) {
        PLOG(FATAL) << "Fail to connect to " << point;
        return NULL;
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
                PLOG(FATAL) << "Fail to write fd=" << fd;
                return NULL;
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
    return NULL;
}

TEST_F(MessengerTest, dispatch_tasks) {
    client_stop = false;
    
    melon::Acceptor messenger[NEPOLL];
    pthread_t cth[NCLIENT];
    ClientMeta* cm[NCLIENT];

    const melon::InputMessageHandler pairs[] = {
        { melon::policy::ParseHuluMessage,
          EmptyProcessHuluRequest, NULL, NULL, "dummy_hulu" }
    };

    for (size_t i = 0; i < NEPOLL; ++i) {        
#ifdef USE_UNIX_DOMAIN_SOCKET
        char buf[64];
        snprintf(buf, sizeof(buf), "input_messenger.socket%lu", i);
        int listening_fd = mutil::unix_socket_listen(buf);
#else
        int listening_fd = tcp_listen(mutil::EndPoint(mutil::IP_ANY, 7878));
#endif
        ASSERT_TRUE(listening_fd > 0);
        mutil::make_non_blocking(listening_fd);
        ASSERT_EQ(0, messenger[i].AddHandler(pairs[0]));
        ASSERT_EQ(0, messenger[i].StartAccept(listening_fd, -1, NULL, false));
    }
    
    for (size_t i = 0; i < NCLIENT; ++i) {
        cm[i] = new ClientMeta;
        cm[i]->times = 0;
        cm[i]->bytes = 0;
        ASSERT_EQ(0, pthread_create(&cth[i], NULL, client_thread, cm[i]));
    }

    sleep(1);
    
    LOG(INFO) << "Begin to profile... (5 seconds)";
    ProfilerStart("input_messenger.prof");

    size_t start_client_bytes = 0;
    for (size_t i = 0; i < NCLIENT; ++i) {
        start_client_bytes += cm[i]->bytes;
    }
    mutil::Timer tm;
    tm.start();
    
    sleep(5);
    
    tm.stop();
    ProfilerStop();
    LOG(INFO) << "End profiling";

    client_stop = true;

    size_t client_bytes = 0;
    for (size_t i = 0; i < NCLIENT; ++i) {
        client_bytes += cm[i]->bytes;
    }
    LOG(INFO) << "client_tp=" << (client_bytes - start_client_bytes) / (double)tm.u_elapsed()
              << "MB/s client_msg="
              << (client_bytes - start_client_bytes) * 1000000L / (MESSAGE_SIZE * tm.u_elapsed())
              << "/s";

    for (size_t i = 0; i < NCLIENT; ++i) {
        pthread_join(cth[i], NULL);
        printf("joined client %lu\n", i);
    }
    for (size_t i = 0; i < NEPOLL; ++i) {
        messenger[i].StopAccept(0);
    }
    sleep(1);
    LOG(WARNING) << "begin to exit!!!!";
}
