// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <stdio.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <melon/fiber/fiber.h>
#include <melon/utility/file_util.h>                     // mutil::FilePath
#include <melon/utility/time.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/controller.h>
#include <melon/utility/logging.h>
#include <melon/json2pb/pb_to_json.h>
#include "json_loader.h"
#include "rpc_press_impl.h"

using google::protobuf::Message;
using google::protobuf::Closure;

namespace pbrpcframework {

class ImportErrorPrinter
    : public google::protobuf::compiler::MultiFileErrorCollector {
public:
    // Line and column numbers are zero-based.  A line number of -1 indicates
    // an error with the entire file (e.g. "not found").
    virtual void AddError(const std::string& filename, int line,
                          int /*column*/, const std::string& message) {
        LOG_AT(ERROR, filename.c_str(), line) << message;
    }
};

int PressClient::init() {
    melon::ChannelOptions rpc_options;
    rpc_options.connect_timeout_ms = _options->connect_timeout_ms;
    rpc_options.timeout_ms = _options->timeout_ms;
    rpc_options.max_retry = _options->max_retry;
    rpc_options.protocol = _options->protocol;
    rpc_options.connection_type = _options->connection_type;
    if (_options->attachment_size > 0) {
        _attachment.clear();
        _attachment.assign(_options->attachment_size, 'a');
    }
    if (_rpc_client.Init(_options->host.c_str(), _options->lb_policy.c_str(),
                         &rpc_options) != 0){
        MLOG(ERROR) << "Fail to initialize channel";
        return -1;
    }
    _method_descriptor = find_method_by_name(
        _options->service, _options->method, _importer);
    if (NULL == _method_descriptor) {
        MLOG(ERROR) << "Fail to find method=" << _options->service << '.'
                   << _options->method;
        return -1;
    }
    _response_prototype = get_prototype_by_method_descriptor(
        _method_descriptor, false, _factory);
    return 0;
}

void PressClient::call_method(melon::Controller* cntl, Message* request,
                              Message* response, Closure* done) {
    if (!_attachment.empty()) {
        cntl->request_attachment().append(_attachment);
    }
    _rpc_client.CallMethod(_method_descriptor, cntl, request, response, done);
}

RpcPress::RpcPress()
    : _pbrpc_client(NULL)
    , _started(false)
    , _stop(false)
    , _output_json(NULL) {
}

RpcPress::~RpcPress() {
    if (_output_json) {
        fclose(_output_json);
        _output_json = NULL;
    }
    delete _importer;
}

int RpcPress::init(const PressOptions* options) {
    if (NULL == options) {
        MLOG(ERROR) << "Param[options] is NULL" ;
        return -1;
    }
    _options = *options;

    // Import protos.
    if (_options.proto_file.empty()) {
        MLOG(ERROR) << "-proto is required";
        return -1;
    }
    int pos = _options.proto_file.find_last_of('/');
    std::string proto_file(_options.proto_file.substr(pos + 1));
    std::string proto_path(_options.proto_file.substr(0, pos));
    google::protobuf::compiler::DiskSourceTree sourceTree;
    // look up .proto file in the same directory
    sourceTree.MapPath("", proto_path.c_str());
    // Add paths in -inc
    if (!_options.proto_includes.empty()) {
        mutil::StringSplitter sp(_options.proto_includes.c_str(), ';');
        for (; sp; ++sp) {
            sourceTree.MapPath("", std::string(sp.field(), sp.length()));
        }
    }
    ImportErrorPrinter error_printer;
    _importer = new google::protobuf::compiler::Importer(&sourceTree, &error_printer);
    if (_importer->Import(proto_file.c_str()) == NULL) {
        MLOG(ERROR) << "Fail to import " << proto_file;
        return -1;
    }
    
    _pbrpc_client = new PressClient(&_options, _importer, &_factory);

    if (!_options.output.empty()) {
        mutil::File::Error error;
        mutil::FilePath path(_options.output);
        mutil::FilePath dir = path.DirName();
        if (!mutil::CreateDirectoryAndGetError(dir, &error)) {
            MLOG(ERROR) << "Fail to create directory=`" << dir.value()
                       << "', " << error;
            return -1;
        }
        _output_json = fopen(_options.output.c_str(), "w");
        LOG_IF(ERROR, !_output_json) << "Fail to open " << _options.output;
    }

    int ret = _pbrpc_client->init();
    if (0 != ret) {
        MLOG(ERROR) << "Fail to initialize rpc client";
        return ret;
    }

    if (_options.input.empty()) {
        MLOG(ERROR) << "-input is empty";
        return -1;
    }
    melon::JsonLoader json_util(_importer, &_factory,
                                     _options.service, _options.method);
    if (mutil::PathExists(mutil::FilePath(_options.input))) {
        int fd = open(_options.input.c_str(), O_RDONLY);
        if (fd < 0) {
            PMLOG(ERROR) << "Fail to open " << _options.input;
            return -1;
        }
        json_util.load_messages(fd, &_msgs);
    } else {
        json_util.load_messages(_options.input, &_msgs);
    }
    if (_msgs.empty()) {
        MLOG(ERROR) << "Fail to load requests";
        return -1;
    }
    MLOG(INFO) << "Loaded " << _msgs.size() << " requests";
    _latency_recorder.expose("rpc_press");
    _error_count.expose("rpc_press_error_count");
    return 0;
}

void* RpcPress::sync_call_thread(void* arg) {
    ((RpcPress*)arg)->sync_client();
    return NULL;
}

void RpcPress::handle_response(melon::Controller* cntl,
                               Message* request,
                               Message* response, 
                               int64_t start_time){
    if (!cntl->Failed()){
        int64_t rpc_call_time_us = mutil::gettimeofday_us() - start_time;
        _latency_recorder << rpc_call_time_us;

        if (_output_json) {
            std::string response_json;
            std::string error;
            if (!json2pb::ProtoMessageToJson(*response, &response_json, &error)) {
                MLOG(WARNING) << "Fail to convert to json: " << error;
            }
            fprintf(_output_json, "%s\n", response_json.c_str());
        }
    } else {
        MLOG(WARNING) << "error_code=" <<  cntl->ErrorCode() << ", "
                   << cntl->ErrorText();
        _error_count << 1;
    }
    delete response;
    delete cntl;
}

static mutil::atomic<int> g_thread_count(0);

void RpcPress::sync_client() {
    double req_rate = _options.test_req_rate / _options.test_thread_num;
    //max make up time is 5 s
    if (_msgs.empty()) {
        MLOG(ERROR) << "nothing to send!";
        return;
    }
    const int thread_index = g_thread_count.fetch_add(1, mutil::memory_order_relaxed);
    int msg_index = thread_index;
    int64_t last_expected_time = mutil::monotonic_time_ns();
    const int64_t interval = (int64_t) (1000000000L / req_rate);
    // the max tolerant delay between end_time and expected_time. 10ms or 10 intervals
    int64_t max_tolerant_delay = std::max((int64_t) 10000000L, 10 * interval);    
    while (!_stop) {
        melon::Controller* cntl = new melon::Controller;
        msg_index = (msg_index + _options.test_thread_num) % _msgs.size();
        Message* request = _msgs[msg_index];
        Message* response = _pbrpc_client->get_output_message();
        const int64_t start_time = mutil::gettimeofday_us();
        google::protobuf::Closure* done = melon::NewCallback<
            RpcPress, 
            RpcPress*, 
            melon::Controller*,
            Message*, 
            Message*, int64_t>
            (this, &RpcPress::handle_response, cntl, request, response, start_time);
        const melon::CallId cid1 = cntl->call_id();
        _pbrpc_client->call_method(cntl, request, response, done);
        _sent_count << 1;

        if (_options.test_req_rate <= 0) { 
            melon::Join(cid1);
        } else {
            int64_t end_time = mutil::monotonic_time_ns();
            int64_t expected_time = last_expected_time + interval;
            if (end_time < expected_time) {
                usleep((expected_time - end_time)/1000);
            }
            if (end_time - expected_time > max_tolerant_delay) {
                expected_time = end_time;
            }            
            last_expected_time = expected_time;
        }
    }
}

int RpcPress::start() {
    _ttid.resize(_options.test_thread_num);
    int ret = 0;
    for (int i = 0; i < _options.test_thread_num; i++) {
        if ((ret = pthread_create(&_ttid[i], NULL, sync_call_thread, this)) != 0) {
            MLOG(ERROR) << "Fail to create sending threads";
            return -1;
        }
    }
    melon::InfoThreadOptions info_thr_opt;
    info_thr_opt.latency_recorder = &_latency_recorder;
    info_thr_opt.error_count = &_error_count;
    info_thr_opt.sent_count = &_sent_count;
    if (!_info_thr.start(info_thr_opt)) {
        MLOG(ERROR) << "Fail to create stats thread";
        return -1;
    }
    _started = true;
    return 0;
}
int RpcPress::stop() {
    if (!_started) {
        return -1;
    }
    _stop = true;
    for (size_t i = 0; i < _ttid.size(); i++) {
        pthread_join(_ttid[i], NULL);
    }
    _info_thr.stop();
    return 0;
}
} //namespace
