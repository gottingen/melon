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


#pragma once

#include <stdio.h>
#include <deque>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <melon/var/var.h>
#include <melon/rpc/channel.h>
#include "info_thread.h"
#include "pb_util.h"

namespace pbrpcframework {
class JsonUtil;

struct PressOptions {
    std::string service;         //service name (packet.rpcservice)
    std::string method;          //method name (rpc service method)
    int server_type;        // server type: 0 = hulu server, 1 = old pbrpc server, 2 = sofa server
    double test_req_rate;      // 0 = no limit
    int test_thread_num;
    std::string input;
    std::string output;
    std::string host;            // server's ip:port, used by hulu server and sofa server
    std::string channel;         // server's channel, used by old pbrpc server
    //comcfg::Configure conf;
    std::string conf_dir;
    std::string conf_file;
    std::string connection_type; // connection type  0:SINGLE 1:POOLED 2:SHORT
    int connect_timeout_ms; // connection timeout in milliseconds
    int timeout_ms; // RPC timeout in milliseconds
    int max_retry; // Maximum retry times by RPC framework
    std::string protocol;
    int request_compress_type; // Snappy:1 Gzip:2 Zlib:3 LZ4:4 None:0
    int response_compress_type; // Snappy:1 Gzip:2 Zlib:3 LZ4:4 None:0
    int attachment_size; // Snappy:1 Gzip:2 Zlib:3 LZ4:4 None:0
    bool auth;// Enable Giano authentication
    std::string auth_group;
    std::string lb_policy; // "rr", "Policy of load balance rr ||random"
    std::string proto_file;
    std::string proto_includes;
    
    PressOptions() :
        server_type(0),
        test_req_rate(0),
        test_thread_num(1),
        connect_timeout_ms(1000),
        timeout_ms(1000),
        max_retry(3),
        request_compress_type(0),
        response_compress_type(0),
        attachment_size(0),
        auth(false)
    {}
};

class PressClient {
public:
    PressClient(const PressOptions* options,
                     google::protobuf::compiler::Importer* importer,
                     google::protobuf::DynamicMessageFactory* factory) { 
        _method_descriptor = NULL;
        _response_prototype = NULL;
        _options = options;
        _importer = importer;
        _factory = factory;

    }

    google::protobuf::Message* get_output_message() {
        return _response_prototype->New();
    }

    int init();
    void call_method(melon::Controller* cntl,
                     google::protobuf::Message* request, 
                     google::protobuf::Message* response, 
                     google::protobuf::Closure* done);
public:
    melon::Channel _rpc_client;
    std::string _attachment;
    const PressOptions* _options;
    const google::protobuf::MethodDescriptor* _method_descriptor;
    const google::protobuf::Message* _response_prototype;
    google::protobuf::compiler::Importer* _importer;
    google::protobuf::DynamicMessageFactory* _factory;
};

class RpcPress {
public:
    RpcPress();
    ~RpcPress();
    int init(const PressOptions* options);
    int start();
    int stop();
    const PressOptions* options() { return &_options; }
    
private:
    DISALLOW_COPY_AND_ASSIGN(RpcPress);
    
    bool new_pbrpc_press_client_by_client_type(int client_type);
    void sync_client();
    void handle_response(melon::Controller* cntl,
                         google::protobuf::Message* request,
                         google::protobuf::Message* response,
                         int64_t start_time_ns);
    static void* sync_call_thread(void* arg);

    melon::var::LatencyRecorder _latency_recorder;
    melon::var::Adder<int64_t> _error_count;
    melon::var::Adder<int64_t> _sent_count;
    std::deque<google::protobuf::Message*> _msgs;
    PressClient* _pbrpc_client;
    PressOptions _options;
    bool _started;
    bool _stop;
    FILE* _output_json;
    google::protobuf::compiler::Importer* _importer;
    google::protobuf::DynamicMessageFactory _factory;
    std::vector<pthread_t> _ttid;
    melon::InfoThread _info_thr;
};
}
