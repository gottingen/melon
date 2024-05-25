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



#include <melon/rpc/http/http_status_code.h>     // HTTP_STATUS_*
#include <melon/rpc/http/http_header.h>


namespace melon {

    HttpHeader::HttpHeader()
            : _status_code(HTTP_STATUS_OK), _method(HTTP_METHOD_GET), _version(1, 1) {
        // NOTE: don't forget to clear the field in Clear() as well.
    }

    void HttpHeader::RemoveHeader(const char *key) {
        if (IsContentType(key)) {
            _content_type.clear();
        } else {
            _headers.erase(key);
        }
    }

    void HttpHeader::AppendHeader(const std::string &key,
                                  const mutil::StringPiece &value) {
        std::string &slot = GetOrAddHeader(key);
        if (slot.empty()) {
            slot.assign(value.data(), value.size());
        } else {
            slot.reserve(slot.size() + 1 + value.size());
            slot.push_back(',');
            slot.append(value.data(), value.size());
        }
    }

    void HttpHeader::Swap(HttpHeader &rhs) {
        _headers.swap(rhs._headers);
        _uri.Swap(rhs._uri);
        std::swap(_status_code, rhs._status_code);
        std::swap(_method, rhs._method);
        _content_type.swap(rhs._content_type);
        _unresolved_path.swap(rhs._unresolved_path);
        std::swap(_version, rhs._version);
    }

    void HttpHeader::Clear() {
        _headers.clear();
        _uri.Clear();
        _status_code = HTTP_STATUS_OK;
        _method = HTTP_METHOD_GET;
        _content_type.clear();
        _unresolved_path.clear();
        _version = std::make_pair(1, 1);
    }

    const char *HttpHeader::reason_phrase() const {
        return HttpReasonPhrase(_status_code);
    }

    void HttpHeader::set_status_code(int status_code) {
        _status_code = status_code;
    }

    std::string &HttpHeader::GetOrAddHeader(const std::string &key) {
        if (IsContentType(key)) {
            return _content_type;
        }

        if (!_headers.initialized()) {
            _headers.init(29);
        }
        return _headers[key];
    }

    const HttpHeader &DefaultHttpHeader() {
        static HttpHeader h;
        return h;
    }

} // namespace melon
