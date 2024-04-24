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



// Access many http servers in parallel, much faster than curl (even called in batch)

#include <gflags/gflags.h>
#include <deque>
#include <melon/fiber/fiber.h>
#include <melon/utility/logging.h>
#include <melon/utility/files/scoped_file.h>
#include <melon/rpc/channel.h>

DEFINE_string(url_file, "", "The file containing urls to fetch. If this flag is"
              " empty, read urls from stdin");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_int32(thread_num, 8, "Number of threads to access urls");
DEFINE_int32(concurrency, 1000, "Max number of http calls in parallel");
DEFINE_bool(one_line_mode, false, "Output as `URL HTTP-RESPONSE' on true");
DEFINE_bool(only_show_host, false, "Print host name only");

struct AccessThreadArgs {
    const std::deque<std::string>* url_list;
    size_t offset;
    std::deque<std::pair<std::string, mutil::IOBuf> > output_queue;
    mutil::Mutex output_queue_mutex;
    mutil::atomic<int> current_concurrency;
};

class OnHttpCallEnd : public google::protobuf::Closure {
public:
    void Run();
public:
    melon::Controller cntl;
    AccessThreadArgs* args;
    std::string url;
};

void OnHttpCallEnd::Run() {
    std::unique_ptr<OnHttpCallEnd> delete_self(this);
    {
        MELON_SCOPED_LOCK(args->output_queue_mutex);
        if (cntl.Failed()) {
            args->output_queue.push_back(std::make_pair(url, mutil::IOBuf()));
        } else {
            args->output_queue.push_back(
                std::make_pair(url, cntl.response_attachment()));
        }
    }
    args->current_concurrency.fetch_sub(1, mutil::memory_order_relaxed);
}

void* access_thread(void* void_args) {
    AccessThreadArgs* args = (AccessThreadArgs*)void_args;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    options.connect_timeout_ms = FLAGS_timeout_ms / 2;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    const int concurrency_for_this_thread = FLAGS_concurrency / FLAGS_thread_num;

    for (size_t i = args->offset; i < args->url_list->size(); i += FLAGS_thread_num) {
        std::string const& url = (*args->url_list)[i];
        melon::Channel channel;
        if (channel.Init(url.c_str(), &options) != 0) {
            LOG(ERROR) << "Fail to create channel to url=" << url;
            MELON_SCOPED_LOCK(args->output_queue_mutex);
            args->output_queue.push_back(std::make_pair(url, mutil::IOBuf()));
            continue;
        }
        while (args->current_concurrency.fetch_add(1, mutil::memory_order_relaxed)
               > concurrency_for_this_thread) {
            args->current_concurrency.fetch_sub(1, mutil::memory_order_relaxed);
            fiber_usleep(5000);
        }
        OnHttpCallEnd* done = new OnHttpCallEnd;
        done->cntl.http_request().uri() = url;
        done->args = args;
        done->url = url;
        channel.CallMethod(NULL, &done->cntl, NULL, NULL, done);
    }
    return NULL;
}

int main(int argc, char** argv) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);
    
    // if (FLAGS_path.empty() || FLAGS_path[0] != '/') {
    //     FLAGS_path = "/" + FLAGS_path;
    // }

    mutil::ScopedFILE fp_guard;
    FILE* fp = NULL;
    if (!FLAGS_url_file.empty()) {
        fp_guard.reset(fopen(FLAGS_url_file.c_str(), "r"));
        if (!fp_guard) {
            PLOG(ERROR) << "Fail to open `" << FLAGS_url_file << '\'';
            return -1;
        }
        fp = fp_guard.get();
    } else {
        fp = stdin;
    }
    char* line_buf = NULL;
    size_t line_len = 0;
    ssize_t nr = 0;
    std::deque<std::string> url_list;
    while ((nr = getline(&line_buf, &line_len, fp)) != -1) {
        if (line_buf[nr - 1] == '\n') { // remove ending newline
            line_buf[nr - 1] = '\0';
            --nr;
        }
        mutil::StringPiece line(line_buf, nr);
        line.trim_spaces();
        if (!line.empty()) {
            url_list.push_back(line.as_string());
        }
    }
    if (url_list.empty()) {
        return 0;
    }
    AccessThreadArgs* args = new AccessThreadArgs[FLAGS_thread_num];
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        args[i].url_list = &url_list;
        args[i].offset = i;
        args[i].current_concurrency.store(0, mutil::memory_order_relaxed);
    }
    std::vector<fiber_t> tids;
    tids.resize(FLAGS_thread_num);
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        CHECK_EQ(0, fiber_start_background(&tids[i], NULL, access_thread, &args[i]));
    }
    std::deque<std::pair<std::string, mutil::IOBuf> > output_queue;
    size_t nprinted = 0;
    while (nprinted != url_list.size()) {
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            {
                MELON_SCOPED_LOCK(args[i].output_queue_mutex);
                output_queue.swap(args[i].output_queue);
            }
            for (size_t i = 0; i < output_queue.size(); ++i) {
                mutil::StringPiece url = output_queue[i].first;
                mutil::StringPiece hostname;
                if (url.starts_with("http://")) {
                    url.remove_prefix(7);
                }
                size_t slash_pos = url.find('/');
                if (slash_pos != mutil::StringPiece::npos) {
                    hostname = url.substr(0, slash_pos);
                } else {
                    hostname = url;
                }
                if (FLAGS_one_line_mode) {
                    if (FLAGS_only_show_host) {
                        std::cout << hostname;
                    } else {
                        std::cout << "http://" << url;
                    }
                    if (output_queue[i].second.empty()) {
                        std::cout << " ERROR" << std::endl;
                    } else {
                        std::cout << ' ' << output_queue[i].second << std::endl;
                    }
                } else {
                    // The prefix is unlikely be part of a ordinary http body,
                    // thus the line can be easily removed by shell utilities.
                    std::cout << "#### ";
                    if (FLAGS_only_show_host) {
                        std::cout << hostname;
                    } else {
                        std::cout << "http://" << url;
                    }
                    if (output_queue[i].second.empty()) {
                        std::cout << " ERROR" << std::endl;
                    } else {
                        std::cout << '\n' << output_queue[i].second << std::endl;
                    }
                }
            }
            nprinted += output_queue.size();
            output_queue.clear();
        }
        usleep(10000);
    }

    for (int i = 0; i < FLAGS_thread_num; ++i) {
        fiber_join(tids[i], NULL);
    }
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        while (args[i].current_concurrency.load(mutil::memory_order_relaxed) != 0) {
            usleep(10000);
        }
    }
    return 0;
}
