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

    //TODO(jeff.li) add famous known header
    class RestfulRequest {
    public:
        static constexpr const char *CONTENT_TYPE = "Content-Type";

        static constexpr const char *AUTHORIZATION = "Authorization";

        using HeaderIterator = HttpHeader::HeaderIterator;
    public:
        explicit RestfulRequest(Controller *controller) : _controller(controller) {}
        ~RestfulRequest() = default;

        /// getters
        HttpMethod method() const;

        bool is_get() const;

        bool is_post() const;

        bool is_http11() const;

        bool is_http2() const;

        const std::string *find_header(const std::string &key) const;

        const HeaderIterator header_begin() const;

        const HeaderIterator header_end() const;

        const std::string *get_content_type() const;

        const std::string *get_authorization() const;

        size_t header_size() const;

        const URI &uri() const;

        const std::string &unresolved_path() const;

        size_t body_size() const;

        const mutil::IOBuf &body() const;

        /// setters
        void set_method(HttpMethod m);

        void set_get();

        void set_post();

        void set_http11();

        void set_http2();

        void set_uri(const std::string &uri);

        void set_uri(const URI &uri);

        void set_header(const std::string &key, const std::string &value);

        void append_header(const std::string &key, const std::string &value);

        void set_content_type(const std::string &value);

        void set_content_type_json();

        void set_content_type_text();

        void set_content_type_proto();

        void set_authorization(const std::string &value);

        void clear_body();

        void set_body(const mutil::IOBuf &buf);

        void set_body(mutil::IOBuf &&buf);

        void set_body(const std::string &buf);

        void append_body(const mutil::IOBuf &buf);

        void append_body(mutil::IOBuf &&buf);

        void append_body(const std::string &buf);

    private:
        Controller *_controller;
    };

    inline HttpMethod RestfulRequest::method() const {
        return _controller->http_request().method();
    }

    inline bool RestfulRequest::is_get() const {
        return _controller->http_request().method() == HttpMethod::HTTP_METHOD_GET;
    }

    inline bool RestfulRequest::is_post() const {
        return _controller->http_request().method() == HttpMethod::HTTP_METHOD_POST;
    }

    inline bool RestfulRequest::is_http11() const {
        return _controller->http_request().major_version() == 1 &&
               _controller->http_request().minor_version() == 1;
    }

    inline bool RestfulRequest::is_http2() const {
        return _controller->http_request().major_version() == 2;
    }

    inline const std::string *RestfulRequest::find_header(const std::string &key) const {
        return _controller->http_request().GetHeader(key);
    }

    inline const RestfulRequest::HeaderIterator RestfulRequest::header_begin() const {
        return _controller->http_request().HeaderBegin();
    }

    inline const RestfulRequest::HeaderIterator RestfulRequest::header_end() const {
        return _controller->http_request().HeaderEnd();
    }

    inline const std::string *RestfulRequest::get_content_type() const {
        return find_header(CONTENT_TYPE);
    }

    inline const std::string *RestfulRequest::get_authorization() const {
        return find_header(AUTHORIZATION);
    }


    inline size_t RestfulRequest::header_size() const {
        return _controller->http_request().HeaderCount();
    }

    inline const URI &RestfulRequest::uri() const {
        return _controller->http_request().uri();
    }

    // Get the HTTP method of the request.
    inline const std::string &RestfulRequest::unresolved_path() const {
        return _controller->http_request().unresolved_path();
    }

    inline  size_t RestfulRequest::body_size() const {
        return _controller->request_attachment().size();
    }

    inline const mutil::IOBuf &RestfulRequest::body() const {
        return _controller->request_attachment();
    }

    inline void RestfulRequest::set_method(HttpMethod m) {
        _controller->http_request().set_method(m);
    }

    inline void RestfulRequest::set_get() {
        _controller->http_request().set_method(HttpMethod::HTTP_METHOD_GET);
    }

    inline void RestfulRequest::set_post() {
        _controller->http_request().set_method(HttpMethod::HTTP_METHOD_POST);
    }

    inline void RestfulRequest::set_http11() {
        _controller->http_request().set_version(1, 1);
    }

    inline void RestfulRequest::set_http2() {
        _controller->http_request().set_version(2, 0);
    }

    inline void RestfulRequest::set_header(const std::string &key, const std::string &value) {
        _controller->http_request().SetHeader(key, value);
    }

    inline void RestfulRequest::append_header(const std::string &key, const std::string &value) {
        _controller->http_request().AppendHeader(key, value);
    }

    inline void RestfulRequest::set_content_type(const std::string &value) {
        _controller->http_request().set_content_type(value);
    }

    inline void RestfulRequest::set_content_type_json() {
        _controller->http_request().set_content_type("application/json");
    }

    inline void RestfulRequest::set_content_type_text() {
        _controller->http_request().set_content_type("text/plain");
    }

    inline void RestfulRequest::set_content_type_proto() {
        _controller->http_request().set_content_type("application/proto");
    }

    inline void RestfulRequest::set_authorization(const std::string &value) {
        _controller->http_request().SetHeader(AUTHORIZATION, value);
    }

    inline void RestfulRequest::clear_body() {
        _controller->request_attachment().clear();
    }

    inline void RestfulRequest::set_body(const mutil::IOBuf &buf) {
        _controller->request_attachment() = buf;
    }

    inline void RestfulRequest::set_body(mutil::IOBuf &&buf) {
        _controller->request_attachment() = std::move(buf);
    }

    inline void RestfulRequest::set_body(const std::string &buf) {
        _controller->request_attachment().clear();
        _controller->request_attachment().append(buf);
    }

    inline void RestfulRequest::append_body(const mutil::IOBuf &buf) {
        _controller->request_attachment().append(buf);
    }

    inline void RestfulRequest::append_body(mutil::IOBuf &&buf) {
        _controller->request_attachment().append(std::move(buf));
    }

    inline void RestfulRequest::append_body(const std::string &buf) {
        _controller->request_attachment().append(buf);
    }

    inline void RestfulRequest::set_uri(const std::string &uri) {
        _controller->http_request().uri() = uri;
    }

    inline void RestfulRequest::set_uri(const URI &uri) {
        _controller->http_request().uri() = uri;
    }

}  // namespace melon
