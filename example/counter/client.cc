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
#include "counter.pb.h"

DEFINE_bool(log_each_request, false, "Print log for each request");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_int32(add_percentage, 100, "Percentage of fetch_add");
DEFINE_int64(added_by, 1, "Num added to each peer");
DEFINE_int32(thread_num, 1, "Number of threads sending requests");
DEFINE_int32(timeout_ms, 1000, "Timeout for each request");
DEFINE_string(conf, "", "Configuration of the raft group");
DEFINE_string(group, "Counter", "Id of the replication group");

melon::var::LatencyRecorder g_latency_recorder("counter_client");

static void* sender(void* arg) {
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
        example::CounterService_Stub stub(&channel);

        melon::Controller cntl;
        cntl.set_timeout_ms(FLAGS_timeout_ms);
        // Randomly select which request we want send;
        example::CounterResponse response;

        if (mutil::fast_rand_less_than(100) < (size_t)FLAGS_add_percentage) {
            example::FetchAddRequest request;
            request.set_value(FLAGS_added_by);
            stub.fetch_add(&cntl, &request, &response, NULL);
        } else {
            example::GetRequest request;
            stub.get(&cntl, &request, &response, NULL);
        }
        if (cntl.Failed()) {
            LOG(WARNING) << "Fail to send request to " << leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            melon::raft::rtb::update_leader(FLAGS_group, melon::raft::PeerId());
            fiber_usleep(FLAGS_timeout_ms * 1000L);
            continue;
        }
        if (!response.success()) {
            LOG(WARNING) << "Fail to send request to " << leader
                         << ", redirecting to "
                         << (response.has_redirect() 
                                ? response.redirect() : "nowhere");
            // Update route table since we have redirect information
            melon::raft::rtb::update_leader(FLAGS_group, response.redirect());
            continue;
        }
        g_latency_recorder << cntl.latency_us();
        if (FLAGS_log_each_request) {
            LOG(INFO) << "Received response from " << leader
                      << " value=" << response.value()
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
    if (!FLAGS_use_fiber) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], NULL, sender, NULL) != 0) {
                LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        tids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (fiber_start_background(&tids[i], NULL, sender, NULL) != 0) {
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
