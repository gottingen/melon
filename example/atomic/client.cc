// Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gflags/gflags.h>
#include <melon/fiber/fiber.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/controller.h>
#include <melon/raft/raft.h>
#include <melon/raft/util.h>
#include <melon/raft/route_table.h>
#include "atomic.pb.h"

DEFINE_bool(log_each_request, false, "Print log for each request");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_int32(add_percentage, 100, "Percentage of fetch_add");
DEFINE_int64(added_by, 1, "Num added to each peer");
DEFINE_int32(thread_num, 1, "Number of threads sending requests");
DEFINE_int32(timeout_ms, 1000, "Timeout for each request");
DEFINE_string(conf, "", "Configuration of the raft group");
DEFINE_string(group, "Atomic", "Id of the replication group");

melon::var::LatencyRecorder g_latency_recorder("atomic_client");
std::atomic<int> g_nthreads(0);

struct SendArg {
    int64_t id;
};

static void* sender(void* arg) {
    SendArg* sa = (SendArg*)arg;
    int64_t value = 0;
    while (!melon::IsAskedToQuit()) {
        melon::raft::PeerId leader;
        // Select leader of the target group from RouteTable
        if (melon::raft::rtb::select_leader(FLAGS_group, &leader) != 0) {
            // Leader is unknown in RouteTable. Ask RouteTable to refresh leader
            // by sending RPCs.
            mutil::Status st = melon::raft::rtb::refresh_leader(
                        FLAGS_group, FLAGS_timeout_ms);
            if (!st.ok()) {
                // Not sure about the leader, sleep for a while and the ask again.
                LOG(WARNING) << "Fail to refresh_leader : " << st;
                fiber_usleep(FLAGS_timeout_ms * 1000L);
            }
            continue;
        }

        // Now we known who is the leader, construct Stub and then sending
        // rpc
        melon::Channel channel;
        if (channel.Init(leader.addr, NULL) != 0) {
            LOG(ERROR) << "Fail to init channel to " << leader;
            fiber_usleep(FLAGS_timeout_ms * 1000L);
            continue;
        }
        example::AtomicService_Stub stub(&channel);

        melon::Controller cntl;
        cntl.set_timeout_ms(FLAGS_timeout_ms);
        example::CompareExchangeRequest request;
        example::AtomicResponse response;
        request.set_id(sa->id);
        request.set_expected_value(value);
        request.set_new_value(value + 1);

        stub.compare_exchange(&cntl, &request, &response, NULL);

        if (cntl.Failed()) {
            LOG(WARNING) << "Fail to send request to " << leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            melon::raft::rtb::update_leader(FLAGS_group, melon::raft::PeerId());
            fiber_usleep(FLAGS_timeout_ms * 1000L);
            continue;
        }

        if (!response.success()) {
            // A redirect response
            if (!response.has_old_value()) {
                LOG(WARNING) << "Fail to send request to " << leader
                             << ", redirecting to "
                             << (response.has_redirect() 
                                    ? response.redirect() : "nowhere");
                // Update route table since we have redirect information
                melon::raft::rtb::update_leader(FLAGS_group, response.redirect());
                continue;
            }
            // old_value unmatches expected value check if this is the initial
            // request
            if (value == 0 || response.old_value() == value + 1) {
                //   ^^^                          ^^^
                // It's initalized value          ^^^
                //                          There was false negative
                value = response.old_value();
            } else {
                CHECK_EQ(value, response.old_value());
                exit(-1);
            }
        } else {
            value = response.new_value();
        }
        g_latency_recorder << cntl.latency_us();
        if (FLAGS_log_each_request) {
            LOG(INFO) << "Received response from " << leader
                      << " old_value=" << response.old_value()
                      << " new_value=" << response.new_value()
                      << " latency=" << cntl.latency_us();
            fiber_usleep(1000L * 1000L);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    mutil::AtExitManager exit_manager;

    // Register configuration of target group to RouteTable
    if (melon::raft::rtb::update_configuration(FLAGS_group, FLAGS_conf) != 0) {
        LOG(ERROR) << "Fail to register configuration " << FLAGS_conf
                   << " of group " << FLAGS_group;
        return -1;
    }

    std::vector<fiber_t> tids;
    std::vector<pthread_t> pids;
    std::vector<SendArg> args;
    for (int i = 1; i <= FLAGS_thread_num; ++i) {
        SendArg arg = { i };
        args.push_back(arg);
    }

    if (!FLAGS_use_fiber) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(
                        &pids[i], NULL, sender, &args[i]) != 0) {
                LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        tids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (fiber_start_background(
                        &tids[i], NULL, sender, &args[i]) != 0) {
                LOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }

    while (!melon::IsAskedToQuit()) {
        sleep(1);
        LOG_IF(INFO, !FLAGS_log_each_request)
                << "Sending Request to " << FLAGS_group
                << " (" << FLAGS_conf << ')'
                << " at qps=" << g_latency_recorder.qps(1)
                << " latency=" << g_latency_recorder.latency(1);
    }

    LOG(INFO) << "Counter client is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(tids[i], NULL);
        }
    }
    return 0;
}
