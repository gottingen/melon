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


// A server to receive EchoRequest and send back EchoResponse.

#include <gflags/gflags.h>
#include <turbo/log/logging.h>
#include <melon/rpc/server.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <melon/json2pb/json_to_pb.h>
#include <melon/fiber/timer_thread.h>
#include <melon/fiber/fiber.h>

#include <cstdlib>
#include <fstream>
#include "cl_test.pb.h"

DEFINE_int32(server_fiber_concurrency, 4,
             "Configuring the value of fiber_concurrency, For compute max qps, ");
DEFINE_int32(server_sync_sleep_us, 2500, 
             "Usleep time, each request will be executed once, For compute max qps");
// max qps = 1000 / 2.5 * 4 

DEFINE_int32(control_server_port, 9000, "");
DEFINE_int32(echo_port, 9001, "TCP Port of echo server");
DEFINE_int32(cntl_port, 9000, "TCP Port of controller server");
DEFINE_string(case_file, "", "File path for test_cases");
DEFINE_int32(latency_change_interval_us, 50000, "Intervalt for server side changes the latency");
DEFINE_int32(server_max_concurrency, 0, "Echo Server's max_concurrency");
DEFINE_bool(use_usleep, false, 
            "EchoServer uses ::usleep or fiber_usleep to simulate latency "
            "when processing requests");


fiber::TimerThread g_timer_thread;

int cast_func(void* arg) {
    return *(int*)arg;
}

void DisplayStage(const test::Stage& stage) {
    std::string type;
    switch(stage.type()) {
        case test::FLUCTUATE: 
            type = "Fluctuate";
            break;
        case test::SMOOTH:
            type = "Smooth";
            break;
        default:
            type = "Unknown";
    }
    std::stringstream ss;
    ss 
        << "Stage:[" << stage.lower_bound() << ':' 
        << stage.upper_bound() <<  "]"
        << " , Type:" << type;
    LOG(INFO) << ss.str();
}

mutil::atomic<int> cnt(0);
mutil::atomic<int> atomic_sleep_time(0);
melon::var::PassiveStatus<int> atomic_sleep_time_var(cast_func, &atomic_sleep_time);

namespace fiber {
DECLARE_int32(fiber_concurrency);
}

void TimerTask(void* data);

class EchoServiceImpl : public test::EchoService {
public:
    EchoServiceImpl() 
        : _stage_index(0)
        , _running_case(false) {
    };

    virtual ~EchoServiceImpl() {}

    void SetTestCase(const test::TestCase& test_case) {
        _test_case = test_case;
        _next_stage_start = _test_case.latency_stage_list(0).duration_sec() + 
            mutil::gettimeofday_s();
        _stage_index = 0;
        _running_case = false;
        DisplayStage(_test_case.latency_stage_list(_stage_index));
    }

    void StartTestCase() {
        CHECK(!_running_case);
        _running_case = true;
        UpdateLatency();
    }

    void StopTestCase() {
        _running_case = false;
    }
    
    void UpdateLatency() {
        if (!_running_case) {
            return;
        }
        ComputeLatency();
        g_timer_thread.schedule(TimerTask, (void*)this, 
            mutil::microseconds_from_now(FLAGS_latency_change_interval_us));
    }

    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const test::NotifyRequest* request,
                      test::NotifyResponse* response,
                      google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        response->set_message("hello");
        ::usleep(FLAGS_server_sync_sleep_us);
        if (FLAGS_use_usleep) {
            ::usleep(_latency.load(mutil::memory_order_relaxed));
        } else {
            fiber_usleep(_latency.load(mutil::memory_order_relaxed));
        }
    }

    void ComputeLatency() {
        if (_stage_index < _test_case.latency_stage_list_size() &&
            mutil::gettimeofday_s() > _next_stage_start) {
            ++_stage_index;
            if (_stage_index < _test_case.latency_stage_list_size()) {
                _next_stage_start += _test_case.latency_stage_list(_stage_index).duration_sec();
                DisplayStage(_test_case.latency_stage_list(_stage_index));
            }
        }

        if (_stage_index == _test_case.latency_stage_list_size()) {
            const test::Stage& latency_stage = 
                _test_case.latency_stage_list(_stage_index - 1);
            if (latency_stage.type() == test::ChangeType::FLUCTUATE) {
                _latency.store((latency_stage.lower_bound() + latency_stage.upper_bound()) / 2,
                               mutil::memory_order_relaxed);
            } else if (latency_stage.type() == test::ChangeType::SMOOTH) {
                _latency.store(latency_stage.upper_bound(), mutil::memory_order_relaxed);
            }
            return;
        }
        
        const test::Stage& latency_stage = _test_case.latency_stage_list(_stage_index);
        const int lower_bound = latency_stage.lower_bound();
        const int upper_bound = latency_stage.upper_bound();
        if (latency_stage.type() == test::FLUCTUATE) {
            _latency.store(mutil::fast_rand_less_than(upper_bound - lower_bound) + lower_bound,
                           mutil::memory_order_relaxed);
        } else if (latency_stage.type() == test::SMOOTH) {
            int latency = lower_bound + (upper_bound - lower_bound) / 
                double(latency_stage.duration_sec()) * 
                (latency_stage.duration_sec() - _next_stage_start + 
                mutil::gettimeofday_s());
            _latency.store(latency, mutil::memory_order_relaxed);
        } else {
            LOG(FATAL) << "Wrong Type:" << latency_stage.type();
        }
    }

private:
    int _stage_index;
    int _next_stage_start;
    mutil::atomic<int> _latency;
    test::TestCase _test_case;
    bool _running_case;
};

void TimerTask(void* data) {
    EchoServiceImpl* echo_service = (EchoServiceImpl*)data;
    echo_service->UpdateLatency();
}

class ControlServiceImpl : public test::ControlService {
public:
    ControlServiceImpl() 
        : _case_index(0) {
        LoadCaseSet(FLAGS_case_file);
        _echo_service = new EchoServiceImpl;
        if (_server.AddService(_echo_service,
                                melon::SERVER_OWNS_SERVICE) != 0) {
            LOG(FATAL) << "Fail to add service";
        }
        g_timer_thread.start(NULL);
    }
    virtual ~ControlServiceImpl() { 
        _echo_service->StopTestCase();
        g_timer_thread.stop_and_join(); 
    };

    virtual void Notify(google::protobuf::RpcController* cntl_base,
                        const test::NotifyRequest* request,
                        test::NotifyResponse* response,
                        google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        const std::string& message = request->message();
        LOG(INFO) << message;
        if (message == "ResetCaseSet") {
            _server.Stop(0);
            _server.Join();
            _echo_service->StopTestCase();

            LoadCaseSet(FLAGS_case_file);
            _case_index = 0;
            response->set_message("CaseSetReset");
        } else if (message == "StartCase") {
            CHECK(!_server.IsRunning()) << "Continuous StartCase";
            const test::TestCase& test_case = _case_set.test_case(_case_index++);
            _echo_service->SetTestCase(test_case);
            melon::ServerOptions options;
            options.max_concurrency = FLAGS_server_max_concurrency;
            _server.MaxConcurrencyOf("test.EchoService.Echo") = test_case.max_concurrency();

            _server.Start(FLAGS_echo_port, &options);            
            _echo_service->StartTestCase();
            response->set_message("CaseStarted");
        } else if (message == "StopCase") {
            CHECK(_server.IsRunning()) << "Continuous StopCase";
            _server.Stop(0);
            _server.Join();

            _echo_service->StopTestCase();
            response->set_message("CaseStopped");
        } else {
            LOG(FATAL) << "Invalid message:" << message;
            response->set_message("Invalid Cntl Message");
        }
    }

private:
    void LoadCaseSet(const std::string& file_path) {
        std::ifstream ifs(file_path.c_str(), std::ios::in);  
        if (!ifs) {
            LOG(FATAL) << "Fail to open case set file: " << file_path;
        }
        std::string case_set_json((std::istreambuf_iterator<char>(ifs)),  
                                  std::istreambuf_iterator<char>()); 
        test::TestCaseSet case_set;
        std::string err;
        if (!json2pb::JsonToProtoMessage(case_set_json, &case_set, &err)) {
            LOG(FATAL)
                << "Fail to trans case_set from json to protobuf message: "
                << err;
        }
        _case_set = case_set;
        ifs.close();
    }

    melon::Server _server;
    EchoServiceImpl* _echo_service;
    test::TestCaseSet _case_set;
    int _case_index;
};


int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);
    fiber::FLAGS_fiber_concurrency= FLAGS_server_fiber_concurrency;

    melon::Server server;

    ControlServiceImpl control_service_impl;

    if (server.AddService(&control_service_impl, 
                          melon::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    if (server.Start(FLAGS_cntl_port, NULL) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    server.RunUntilAskedToQuit();
    return 0;
}

