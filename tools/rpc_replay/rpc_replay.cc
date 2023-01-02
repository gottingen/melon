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


#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include "melon/times/time.h"
#include "melon/files/filesystem.h"
#include <melon/metrics/all.h>
#include <melon/fiber/internal/fiber.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/server.h>
#include <melon/rpc/rpc_dump.h>
#include <melon/rpc/serialized_request.h>
#include <melon/rpc/details/http_message.h>
#include "info_thread.h"

DEFINE_string(dir, "", "The directory of dumped requests");
DEFINE_int32(times, 1, "Repeat replaying for so many times");
DEFINE_int32(qps, 0, "Limit QPS if this flag is positive");
DEFINE_int32(thread_num, 0, "Number of threads for replaying");
DEFINE_bool(use_fiber, true, "Use fiber to replay");
DEFINE_string(connection_type, "", "Connection type, choose automatically "
              "according to protocol by default");
DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Maximum retry times");
DEFINE_int32(dummy_port, 8899, "Port of dummy server(to monitor replaying)");
DEFINE_string(http_host, "", "Host field for http protocol");

melon::LatencyRecorder g_latency_recorder("rpc_replay");
melon::counter<int64_t> g_error_count("rpc_replay_error_count");
melon::counter<int64_t> g_sent_count;

// Include channels for all protocols that support both client and server.
class ChannelGroup {
public:
    int Init();

    ~ChannelGroup();

    // Get channel by protocol type.
    melon::rpc::Channel* channel(melon::rpc::ProtocolType type) {
        if ((size_t)type < _chans.size()) {
            return _chans[(size_t)type];
        }
        return nullptr;
    }
    
private:
    std::vector<melon::rpc::Channel*> _chans;
};

int ChannelGroup::Init() {
    {
        // force global initialization of rpc.
        melon::rpc::Channel dummy_channel;
    }
    std::vector<std::pair<melon::rpc::ProtocolType, melon::rpc::Protocol> > protocols;
    melon::rpc::ListProtocols(&protocols);
    size_t max_protocol_size = 0;
    for (size_t i = 0; i < protocols.size(); ++i) {
        max_protocol_size = std::max(max_protocol_size,
                                     (size_t)protocols[i].first);
    }
    _chans.resize(max_protocol_size + 1);
    for (size_t i = 0; i < protocols.size(); ++i) {
        if (protocols[i].second.support_client() &&
            protocols[i].second.support_server()) {
            const melon::rpc::ProtocolType prot = protocols[i].first;
            melon::rpc::Channel* chan = new melon::rpc::Channel;
            melon::rpc::ChannelOptions options;
            options.protocol = prot;
            options.connection_type = FLAGS_connection_type;
            options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
            options.max_retry = FLAGS_max_retry;
            if (chan->Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(),
                        &options) != 0) {
                MELON_LOG(ERROR) << "Fail to initialize channel";
                return -1;
            }
            _chans[prot] = chan;
        }
    }
    return 0;
}

ChannelGroup::~ChannelGroup() {
    for (size_t i = 0; i < _chans.size(); ++i) {
        delete _chans[i];
    }
    _chans.clear();
}

static void handle_response(melon::rpc::Controller* cntl, int64_t start_time,
                            bool sleep_on_error/*note*/) {
    // TODO(jeff): some fibers are starved when new fibers are created
    // continuously, which happens when server is down and RPC keeps failing.
    // Sleep a while on error to avoid that now.
    const int64_t end_time = melon::get_current_time_micros();
    const int64_t elp = end_time - start_time;
    if (!cntl->Failed()) {
        g_latency_recorder << elp;
    } else {
        g_error_count << 1;
        if (sleep_on_error) {
            melon::fiber_sleep_for(10000);
        }
    }
    delete cntl;
}

std::atomic<int> g_thread_offset(0);

static void* replay_thread(void* arg) {
    ChannelGroup* chan_group = static_cast<ChannelGroup*>(arg);
    const int thread_offset = g_thread_offset.fetch_add(1, std::memory_order_relaxed);
    double req_rate = FLAGS_qps / (double)FLAGS_thread_num;
    melon::rpc::SerializedRequest req;
    std::deque<int64_t> timeq;
    size_t MAX_QUEUE_SIZE = (size_t)req_rate;
    if (MAX_QUEUE_SIZE < 100) {
        MAX_QUEUE_SIZE = 100;
    } else if (MAX_QUEUE_SIZE > 2000) {
        MAX_QUEUE_SIZE = 2000;
    }
    timeq.push_back(melon::get_current_time_micros());
    for (int i = 0; !melon::rpc::IsAskedToQuit() && i < FLAGS_times; ++i) {
        melon::rpc::SampleIterator it(FLAGS_dir);
        int j = 0;
        for (melon::rpc::SampledRequest* sample = it.Next();
             !melon::rpc::IsAskedToQuit() && sample != nullptr; sample = it.Next(), ++j) {
            std::unique_ptr<melon::rpc::SampledRequest> sample_guard(sample);
            if ((j % FLAGS_thread_num) != thread_offset) {
                continue;
            }
            melon::rpc::Channel* chan =
                chan_group->channel(sample->meta.protocol_type());
            if (chan == nullptr) {
                MELON_LOG(ERROR) << "No channel on protocol="
                           << sample->meta.protocol_type();
                continue;
            }
            
            melon::rpc::Controller* cntl = new melon::rpc::Controller;
            req.Clear();

            melon::rpc::SerializedRequest* req_ptr = &req;

            cntl->reset_sampled_request(sample_guard.release());

            if (sample->meta.protocol_type() == melon::rpc::PROTOCOL_HTTP) {
                melon::rpc::HttpMessage http_message;
                http_message.ParseFromCordBuf(sample->request);
                cntl->http_request().Swap(http_message.header());
                if (!FLAGS_http_host.empty()) {
                    // reset Host in header
                    cntl->http_request().SetHeader("Host", FLAGS_http_host);
                }
                cntl->request_attachment() = http_message.body().movable();
                req_ptr = NULL;
            } else if (sample->meta.attachment_size() > 0) {
                sample->request.cutn(
                    &req.serialized_data(),
                    sample->request.size() - sample->meta.attachment_size());
                cntl->request_attachment() = sample->request.movable();
            } else {
                req.serialized_data() = sample->request.movable();
            }
            g_sent_count << 1;
            const int64_t start_time = melon::get_current_time_micros();
            if (FLAGS_qps <= 0) {
                chan->CallMethod(nullptr/*use rpc_dump_context in cntl instead*/,
                        cntl, req_ptr, nullptr/*ignore response*/, nullptr);
                handle_response(cntl, start_time, true);
            } else {
                google::protobuf::Closure* done =
                    melon::rpc::NewCallback(handle_response, cntl, start_time, false);
                chan->CallMethod(nullptr/*use rpc_dump_context in cntl instead*/,
                        cntl, req_ptr, nullptr/*ignore response*/, done);
                const int64_t end_time = melon::get_current_time_micros();
                int64_t expected_elp = 0;
                int64_t actual_elp = 0;
                timeq.push_back(end_time);
                if (timeq.size() > MAX_QUEUE_SIZE) {
                    actual_elp = end_time - timeq.front();
                    timeq.pop_front();
                    expected_elp = (size_t)(1000000 * timeq.size() / req_rate);
                } else {
                    actual_elp = end_time - timeq.front();
                    expected_elp = (size_t)(1000000 * (timeq.size() - 1) / req_rate);
                }
                if (actual_elp < expected_elp) {
                    melon::fiber_sleep_for(expected_elp - actual_elp);
                }
            }
        }
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_dir.empty() ||
        !melon::exists(FLAGS_dir)) {
        MELON_LOG(ERROR) << "--dir=<dir-of-dumped-files> is required";
        return -1;
    }

    if (FLAGS_dummy_port >= 0) {
        melon::rpc::StartDummyServerAt(FLAGS_dummy_port);
    }
    
    ChannelGroup chan_group;
    if (chan_group.Init() != 0) {
        MELON_LOG(ERROR) << "Fail to init ChannelGroup";
        return -1;
    }

    if (FLAGS_thread_num <= 0) {
        if (FLAGS_qps <= 0) { // unlimited qps
            FLAGS_thread_num = 50;
        } else {
            FLAGS_thread_num = FLAGS_qps / 10000;
            if (FLAGS_thread_num < 1) {
                FLAGS_thread_num = 1;
            }
            if (FLAGS_thread_num > 50) {
                FLAGS_thread_num = 50;
            }
        }
    }

    std::vector<fiber_id_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_fiber) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], nullptr, replay_thread, &chan_group) != 0) {
                MELON_LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (fiber_start_background(
                    &bids[i], nullptr, replay_thread, &chan_group) != 0) {
                MELON_LOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }
    melon::rpc::InfoThread info_thr;
    melon::rpc::InfoThreadOptions info_thr_opt;
    info_thr_opt.latency_recorder = &g_latency_recorder;
    info_thr_opt.error_count = &g_error_count;
    info_thr_opt.sent_count = &g_sent_count;
    
    if (!info_thr.start(info_thr_opt)) {
        MELON_LOG(ERROR) << "Fail to create info_thread";
        return -1;
    }

    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], nullptr);
        } else {
            fiber_join(bids[i], nullptr);
        }
    }
    info_thr.stop();

    return 0;
}
