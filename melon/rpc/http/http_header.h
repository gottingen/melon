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



#ifndef  MELON_RPC_HTTP_HTTP_HEADER_H_
#define  MELON_RPC_HTTP_HTTP_HEADER_H_

#include <melon/utility/strings/string_piece.h>  // StringPiece
#include <melon/utility/containers/case_ignored_flat_map.h>
#include <melon/rpc/uri.h>              // URI
#include <melon/rpc/http/http_method.h>      // HttpMethod
#include <melon/rpc/http/http_status_code.h>
#include <melon/rpc/http/http2.h>


namespace melon {
    class InputMessageBase;
    namespace policy {
        void ProcessHttpRequest(InputMessageBase *msg);

        class H2StreamContext;
    }

    // Non-body part of a HTTP message.
    class HttpHeader {
    public:
        typedef mutil::CaseIgnoredFlatMap<std::string> HeaderMap;
        typedef HeaderMap::const_iterator HeaderIterator;
        typedef HeaderMap::key_equal HeaderKeyEqual;

        HttpHeader();

        // Exchange internal fields with another HttpHeader.
        void Swap(HttpHeader &rhs);

        // Reset internal fields as if they're just default-constructed.
        void Clear();

        // Get http version, 1.1 by default.
        int major_version() const { return _version.first; }

        int minor_version() const { return _version.second; }

        // Change the http version
        void set_version(int http_major, int http_minor) { _version = std::make_pair(http_major, http_minor); }

        // True if version of http is earlier than 1.1
        bool before_http_1_1() const { return (major_version() * 10000 + minor_version()) <= 10000; }

        // True if the message is from HTTP2.
        bool is_http2() const { return major_version() == 2; }

        // Get/set "Content-Type".
        // possible values: "text/plain", "application/json" ...
        // NOTE: Equal to `GetHeader("Content-Type")', ·SetHeader("Content-Type")‘ (case-insensitive).
        const std::string &content_type() const { return _content_type; }

        void set_content_type(const std::string &type) { _content_type = type; }

        void set_content_type(const char *type) { _content_type = type; }

        std::string &mutable_content_type() { return _content_type; }

        // Get value of a header which is case-insensitive according to:
        //   https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
        // Namely, GetHeader("log-id"), GetHeader("Log-Id"), GetHeader("LOG-ID")
        // point to the same value.
        // Return pointer to the value, NULL on not found.
        // NOTE: If the key is "Content-Type", `GetHeader("Content-Type")'
        // (case-insensitive) is equal to `content_type()'.
        const std::string *GetHeader(const char *key) const { return _headers.seek(key); }

        const std::string *GetHeader(const std::string &key) const { return _headers.seek(key); }

        // Set value of a header.
        // NOTE: If the key is "Content-Type", `SetHeader("Content-Type", ...)'
        // (case-insensitive) is equal to `set_content_type(...)'.
        void SetHeader(const std::string &key, const std::string &value) { GetOrAddHeader(key) = value; }

        // Remove a header.
        void RemoveHeader(const char *key);

        void RemoveHeader(const std::string &key) { RemoveHeader(key.c_str()); }

        // Append value to a header. If the header already exists, separate
        // old value and new value with comma(,) according to:
        //   https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
        void AppendHeader(const std::string &key, const mutil::StringPiece &value);

        // Get header iterators which are invalidated after calling AppendHeader()
        HeaderIterator HeaderBegin() const { return _headers.begin(); }

        HeaderIterator HeaderEnd() const { return _headers.end(); }

        // #headers
        size_t HeaderCount() const { return _headers.size(); }

        // Get the URI object, check src/melon/uri.h for details.
        const URI &uri() const { return _uri; }

        URI &uri() { return _uri; }

        // Get/set http method.
        HttpMethod method() const { return _method; }

        void set_method(const HttpMethod method) { _method = method; }

        // Get/set status-code and reason-phrase. Notice that the const char*
        // returned by reason_phrase() will be invalidated after next call to
        // set_status_code().
        int status_code() const { return _status_code; }

        const char *reason_phrase() const;

        void set_status_code(int status_code);

        // The URL path removed with matched prefix.
        // NOTE: always normalized and NOT started with /.
        //
        // Accessing HttpService.Echo
        //   [URL]                               [unresolved_path]
        //   "/HttpService/Echo"                 ""
        //   "/HttpService/Echo/Foo"             "Foo"
        //   "/HttpService/Echo/Foo/Bar"         "Foo/Bar"
        //   "/HttpService//Echo///Foo//"        "Foo"
        //
        // Accessing FileService.default_method:
        //   [URL]                               [unresolved_path]
        //   "/FileService"                      ""
        //   "/FileService/123.txt"              "123.txt"
        //   "/FileService/mydir/123.txt"        "mydir/123.txt"
        //   "/FileService//mydir///123.txt//"   "mydir/123.txt"
        const std::string &unresolved_path() const { return _unresolved_path; }

    private:
        friend class HttpMessage;

        friend class HttpMessageSerializer;

        friend class policy::H2StreamContext;

        friend void policy::ProcessHttpRequest(InputMessageBase *msg);

        std::string &GetOrAddHeader(const std::string &key);

        static bool IsContentType(const std::string &key) {
            return HeaderKeyEqual()(key, "content-type");
        }

        HeaderMap _headers;
        URI _uri;
        int _status_code;
        HttpMethod _method;
        std::string _content_type;
        std::string _unresolved_path;
        std::pair<int, int> _version;
    };

    const HttpHeader &DefaultHttpHeader();

} // namespace melon


#endif  // MELON_RPC_HTTP_HTTP_HEADER_H_
