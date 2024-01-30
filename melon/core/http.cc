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

#include "melon/core/http.h"
#include "melon/compress/gzip_compress.h"
#include "melon/compress/snappy_compress.h"
#include "melon/butil/fast_rand.h"

namespace melon {

    int HttpChannel::initialize(const std::string &host_port, const std::string &lb, bool is_https,
                                ChannelOptions *options) {
        ChannelOptions option;
        if (options != nullptr) {
            option = *options;
        }
        if (is_https) {
            option.protocol = "https";
        }
        return channel.Init(host_port.c_str(), lb.c_str(), &option);

    }

    int HttpChannelPool::initialize(const std::string&host_port, size_t max_pool, const std::string &lb, bool is_https, ChannelOptions *options) {
        channels.reserve(max_pool);
        for (size_t i = 0; i < max_pool; ++i) {
            std::unique_ptr<Channel> channel(new Channel);
            if (channel->Init(host_port.c_str(), lb.c_str(), options) != 0) {
                LOG(ERROR) << "Fail to initialize channel";
                return -1;
            }
            channels.push_back(std::move(channel));
        }
        return 0;
    }

    Channel *HttpChannelPool::get_channel() {
        if(channels.empty()) {
            return nullptr;
        }

        if(channels.size() == 1) {
            return channels[0].get();
        }

        size_t index = butil::fast_rand_less_than(channels.size());
        return channels[index].get();
    }

    Http::~Http() {
        if (_own_channel) {
            delete _channel;
        }
    }

    bool Http::get() {
        _controller.http_request().set_method(HTTP_METHOD_GET);
        _channel->CallMethod(nullptr, &_controller, nullptr, nullptr, nullptr);
        return _controller.Failed() ? false : true;
    }

    bool Http::post(const std::string_view &body) {
        butil::IOBufBuilder os;
        os << "....";
        os.move_to(_controller.request_attachment());
        _channel->CallMethod(nullptr, &_controller, nullptr, nullptr, nullptr);
        return _controller.Failed() ? false : true;
    }

    bool Http::post(butil::IOBufBuilder &body) {
        body.move_to(_controller.request_attachment());
        _channel->CallMethod(nullptr, &_controller, nullptr, nullptr, nullptr);
        return _controller.Failed() ? false : true;
    }

    void Http::get_raw_response_body(std::string &body) const {
        body = _controller.response_attachment().to_string();
    }

    void Http::get_raw_response_body(butil::IOBufBuilder &body) const {
        body << _controller.response_attachment();
    }

    int Http::get_response_body(std::string &body) const {
        auto content_type = _controller.response_compress_type();
        if (content_type == COMPRESS_TYPE_NONE) {
            body = _controller.response_attachment().to_string();
            return 0;
        }
        butil::IOBuf uncompressed;
        if (content_type == COMPRESS_TYPE_GZIP) {
            if (!melon::compress::GzipDecompress(_controller.response_attachment(), &uncompressed)) {
                LOG(ERROR) << "Fail to un-gzip response body";
                return -1;
            }
        } else if (content_type == COMPRESS_TYPE_SNAPPY) {
            if (!melon::compress::SnappyDecompress(_controller.response_attachment(), &uncompressed)) {
                LOG(ERROR) << "Fail to un-snappy response body";
                return -1;
            }
        } else if (content_type == COMPRESS_TYPE_ZLIB) {
            if (!melon::compress::ZlibDecompress(_controller.response_attachment(), &uncompressed)) {
                LOG(ERROR) << "Fail to un-zlib response body";
                return -1;
            }
        }
        body = uncompressed.to_string();
        return 0;
    }

    int Http::get_response_body(butil::IOBuf &body) const {
        body.clear();
        auto content_type = _controller.response_compress_type();
        if (content_type == COMPRESS_TYPE_NONE) {
            body = _controller.response_attachment();
            return 0;
        }
        butil::IOBuf uncompressed;
        if (content_type == COMPRESS_TYPE_GZIP) {
            if (!melon::compress::GzipDecompress(_controller.response_attachment(), &uncompressed)) {
                LOG(ERROR) << "Fail to un-gzip response body";
                return -1;
            }
        } else if (content_type == COMPRESS_TYPE_SNAPPY) {
            if (!melon::compress::SnappyDecompress(_controller.response_attachment(), &uncompressed)) {
                LOG(ERROR) << "Fail to un-snappy response body";
                return -1;
            }
        } else if (content_type == COMPRESS_TYPE_ZLIB) {
            if (!melon::compress::ZlibDecompress(_controller.response_attachment(), &uncompressed)) {
                LOG(ERROR) << "Fail to un-zlib response body";
                return -1;
            }
        }
        uncompressed.swap(body);
        return 0;
    }
}  // namespace melon
