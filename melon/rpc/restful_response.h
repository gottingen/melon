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
// Created by jeff on 24-6-14.
//
#pragma once

#include <melon/rpc/controller.h>
#include <melon/rpc/http/http_header.h>

namespace melon {

    class RestfulResponse {
    public:
        static const std::string CONTENT_TYPE;
        static const std::string AUTHORIZATION;
        static const std::string APPLICATION_JSON;
        using HeaderIterator = HttpHeader::HeaderIterator;
    public:

        RestfulResponse(Controller *controller) : _controller(controller) {}

        /// getters
        int status_code() const;

        const char* get_reason_phrase() const;

        const std::string *find_header(const std::string &key) const;

        const std::string *get_authorization() const;

        const std::string *get_content_type() const;

        const HeaderIterator header_begin() const;

        const HeaderIterator header_end() const;

        size_t header_size() const;

        size_t body_size() const;

        const mutil::IOBuf &body() const;

        bool failed() const;

        std::string failed_reason() const;
        /// setters
        void set_status_code(int code);

        void set_header(const std::string &key, const std::string &value);

        void set_content_type(const std::string &value) {
            set_header(CONTENT_TYPE, value);
        }

        void set_content_json() {
            set_header(CONTENT_TYPE, APPLICATION_JSON);
        }

        void set_access_control_all_allow();

        void append_header(const std::string &key, const std::string &value);

        void clear_body();

        void set_body(const mutil::IOBuf &body);

        void set_body(mutil::IOBuf &&body);

        void set_body(const std::string &body);

        void set_body(const char *body, size_t size);

        void append_body(const mutil::IOBuf &body);

        void append_body(mutil::IOBuf &&body);

        void append_body(const std::string &body);

        void append_body(const char *body, size_t size);
    private:
        Controller *_controller;
    };

    inline int RestfulResponse::status_code() const {
        return _controller->http_response().status_code();
    }

    inline const char* RestfulResponse::get_reason_phrase() const {
        return _controller->http_response().reason_phrase();
    }

    inline const std::string *RestfulResponse::find_header(const std::string &key) const {
        return _controller->http_response().GetHeader(key);
    }

    inline const RestfulResponse::HeaderIterator RestfulResponse::header_begin() const {
        return _controller->http_response().HeaderBegin();
    }

    inline const RestfulResponse::HeaderIterator RestfulResponse::header_end() const {
        return _controller->http_response().HeaderEnd();
    }

    inline const std::string *RestfulResponse::get_content_type() const {
        return find_header(CONTENT_TYPE);
    }

    inline const std::string *RestfulResponse::get_authorization() const {
        return find_header(AUTHORIZATION);
    }

    inline size_t RestfulResponse::header_size() const {
        return _controller->http_response().HeaderCount();
    }

    inline size_t RestfulResponse::body_size() const {
        return _controller->response_attachment().size();
    }

    inline const mutil::IOBuf &RestfulResponse::body() const {
        return _controller->response_attachment();
    }

    inline void RestfulResponse::set_status_code(int code) {
        _controller->http_response().set_status_code(code);
    }

    inline void RestfulResponse::set_header(const std::string &key, const std::string &value) {
        _controller->http_response().SetHeader(key, value);
    }

    inline void RestfulResponse::append_header(const std::string &key, const std::string &value) {
        _controller->http_response().AppendHeader(key, value);
    }

    inline void RestfulResponse::clear_body() {
        _controller->response_attachment().clear();
    }

    inline void RestfulResponse::set_body(const mutil::IOBuf &body) {
        _controller->response_attachment() = body;
    }

    inline void RestfulResponse::set_body(mutil::IOBuf &&body) {
        _controller->response_attachment() = std::move(body);
    }

    inline void RestfulResponse::set_body(const std::string &body) {
        clear_body();
        _controller->response_attachment().append(body.data(), body.size());
    }

    inline void RestfulResponse::append_body(const mutil::IOBuf &body) {
        _controller->response_attachment().append(body);
    }

    inline void RestfulResponse::append_body(mutil::IOBuf &&body) {
        _controller->response_attachment().append(std::move(body));
    }

    inline void RestfulResponse::append_body(const std::string &body) {
        _controller->response_attachment().append(body.data(), body.size());
    }

    inline void RestfulResponse::set_body(const char *body, size_t size) {
        clear_body();
        _controller->response_attachment().append(body, size);
    }

    inline void RestfulResponse::append_body(const char *body, size_t size) {
        _controller->response_attachment().append(body, size);
    }

    inline bool RestfulResponse::failed() const {
        return _controller->Failed();
    }

    inline std::string RestfulResponse::failed_reason() const {
        return _controller->ErrorText();
    }

}  // namespace melon
