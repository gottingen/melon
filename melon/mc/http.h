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

#ifndef MELON_MC_HTTP_H_
#define MELON_MC_HTTP_H_

#include <string>
#include <string_view>
#include <vector>
#include "melon/rpc/controller.h"
#include "melon/rpc/channel.h"
#include "melon/butil/iobuf.h"

namespace melon {

    class HttpChannel {
    public:
        HttpChannel() = default;

        ~HttpChannel() = default;

        int initialize(const std::string &host_port, const std::string &lb, bool is_https = false,
                       ChannelOptions *options = nullptr);

        Channel channel;
    };

    class HttpChannelPool {
    public:
        HttpChannelPool() = default;

        ~HttpChannelPool() = default;

        int initialize(const std::string &host_port, size_t max_pool, const std::string &lb = "", bool is_https = false,
                       ChannelOptions *options = nullptr);

        Channel *get_channel();

    private:
        std::vector<std::unique_ptr<Channel>> channels;
    };


    class Http {
    public:
        Http() = default;

        ~Http();

        Http &set_channel(Channel *channel, bool own_channel = false);

        Http &set_uri(std::string &uri);

        Http &set_timeout(int64_t timeout_ms);

        Http &set_max_retry(int max_retry);

        Http &set_header(const std::string &key, const std::string &value);

        const std::string *get_header(const std::string &key) const;

        Http &set_query(const std::string &key, const std::string &value);

        const std::string *get_query(const std::string &key) const;

        Http &set_content_type(const std::string &content_type);

        const std::string &get_content_type() const;

        Http &enable_request_gzip(bool enable = true);

        Http &enable_request_zlib(bool enable = true);

        Http &enable_request_snappy(bool enable = true);

        Http &enable_response_gzip(bool enable = true);

        Http &enable_response_zlib(bool enable = true);

        Http &enable_response_snappy(bool enable = true);

        Http &enable_keep_alive(bool enable = true);

        bool get();

        bool post(const std::string_view &body);

        bool post(butil::IOBufBuilder &body);

        bool ok() const;

        int response_code() const;

        int64_t latency_us() const;

        std::string error_message() const;

        const std::string *get_response_header(const std::string &key) const;

        std::string get_response_status() const;

        void get_raw_response_body(std::string &body) const;

        void get_raw_response_body(butil::IOBufBuilder &body) const;

        int get_response_body(std::string &body) const;

        int get_response_body(butil::IOBuf &body) const;

    private:

        bool _own_channel{false};
        std::string _uri;
        melon::Channel *_channel{nullptr};
        melon::Controller _controller;
    };

    /// Set query string

    inline Http &Http::set_channel(Channel *channel, bool own_channel) {
        _channel = channel;
        _own_channel = own_channel;
        return *this;
    }

    inline Http &Http::set_uri(std::string &uri) {
        _uri = uri;
        return *this;
    }

    inline Http &Http::set_timeout(int64_t timeout_ms) {
        _controller.set_timeout_ms(timeout_ms);
        return *this;
    }

    inline Http &Http::set_max_retry(int max_retry) {
        _controller.set_max_retry(max_retry);
        return *this;
    }

    inline Http &Http::set_header(const std::string &key, const std::string &value) {
        _controller.http_request().SetHeader(key, value);
        return *this;
    }

    inline Http &Http::set_query(const std::string &key, const std::string &value) {
        _controller.http_request().uri().SetQuery(key, value);
        return *this;
    }

    inline Http &Http::set_content_type(const std::string &content_type) {
        _controller.http_request().set_content_type(content_type);
        return *this;
    }

    inline const std::string *Http::get_header(const std::string &key) const {
        return _controller.http_request().GetHeader(key);
    }

    inline const std::string *Http::get_query(const std::string &key) const {
        return _controller.http_request().uri().GetQuery(key);
    }

    inline const std::string &Http::get_content_type() const {
        return _controller.http_request().content_type();
    }

    inline Http &Http::enable_request_gzip(bool enable) {
        if (enable) {
            _controller.set_request_compress_type(COMPRESS_TYPE_GZIP);
        } else {
            _controller.set_request_compress_type(COMPRESS_TYPE_NONE);
        }
        return *this;
    }

    inline Http &Http::enable_request_zlib(bool enable) {
        if (enable) {
            _controller.set_request_compress_type(COMPRESS_TYPE_ZLIB);
        } else {
            _controller.set_request_compress_type(COMPRESS_TYPE_NONE);
        }
        return *this;
    }

    inline Http &Http::enable_request_snappy(bool enable) {
        if (enable) {
            _controller.set_request_compress_type(COMPRESS_TYPE_SNAPPY);
        } else {
            _controller.set_request_compress_type(COMPRESS_TYPE_NONE);
        }
        return *this;
    }

    inline Http &Http::enable_response_gzip(bool enable) {
        if (enable) {
            _controller.set_response_compress_type(COMPRESS_TYPE_GZIP);
        } else {
            _controller.set_response_compress_type(COMPRESS_TYPE_NONE);
        }
        return *this;
    }

    inline Http &Http::enable_response_zlib(bool enable) {
        if (enable) {
            _controller.set_response_compress_type(COMPRESS_TYPE_ZLIB);
        } else {
            _controller.set_response_compress_type(COMPRESS_TYPE_NONE);
        }
        return *this;
    }

    inline Http &Http::enable_response_snappy(bool enable) {
        if (enable) {
            _controller.set_response_compress_type(COMPRESS_TYPE_SNAPPY);
        } else {
            _controller.set_response_compress_type(COMPRESS_TYPE_NONE);
        }
        return *this;
    }

    inline Http &Http::enable_keep_alive(bool enable) {
        if (enable) {
            set_header("Connection", "keep-alive");
        } else {
            set_header("Connection", "close");
        }
        return *this;
    }

    inline bool Http::ok() const {
        return _controller.Failed() == false;
    }

    inline int Http::response_code() const {
        return _controller.http_response().status_code();
    }

    inline const std::string *Http::get_response_header(const std::string &key) const {
        return _controller.http_response().GetHeader(key);
    }

    inline std::string Http::get_response_status() const {
        return _controller.http_response().reason_phrase();
    }

    inline int64_t Http::latency_us() const {
        return _controller.latency_us();
    }

    inline std::string Http::error_message() const {
        return _controller.ErrorText();
    }
}  // namespace melon
#endif // MELON_MC_HTTP_H_
