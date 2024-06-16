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



#ifndef MELON_RPC_HTTP_HTTP_MESSAGE_H_
#define MELON_RPC_HTTP_HTTP_MESSAGE_H_

#include <memory>                      // std::unique_ptr
#include <string>                      // std::string
#include <melon/base/macros.h>
#include <melon/base/iobuf.h>               // mutil::IOBuf
#include <melon/base/scoped_lock.h>         // mutil::unique_lock
#include <melon/base/endpoint.h>
#include <melon/rpc/http/http_parser.h>  // http_parser
#include <melon/rpc/http/http_header.h>          // HttpHeader
#include <melon/rpc/progressive_reader.h>   // ProgressiveReader


namespace melon {

    enum HttpParserStage {
        HTTP_ON_MESSAGE_BEGIN,
        HTTP_ON_URL,
        HTTP_ON_STATUS,
        HTTP_ON_HEADER_FIELD,
        HTTP_ON_HEADER_VALUE,
        HTTP_ON_HEADERS_COMPLETE,
        HTTP_ON_BODY,
        HTTP_ON_MESSAGE_COMPLETE
    };

    class HttpMessage {
    public:
        // If read_body_progressively is true, the body will be read progressively
        // by using SetBodyReader().
        explicit HttpMessage(bool read_body_progressively = false,
                             HttpMethod request_method = HTTP_METHOD_GET);

        ~HttpMessage();

        const mutil::IOBuf &body() const { return _body; }

        mutil::IOBuf &body() { return _body; }

        // Parse from array, length=0 is treated as EOF.
        // Returns bytes parsed, -1 on failure.
        ssize_t ParseFromArray(const char *data, const size_t length);

        // Parse from mutil::IOBuf.
        // Emtpy `buf' is sliently ignored, which is different from ParseFromArray.
        // Returns bytes parsed, -1 on failure.
        ssize_t ParseFromIOBuf(const mutil::IOBuf &buf);

        bool Completed() const { return _stage == HTTP_ON_MESSAGE_COMPLETE; }

        HttpParserStage stage() const { return _stage; }

        HttpMethod request_method() const { return _request_method; }

        HttpHeader &header() { return _header; }

        const HttpHeader &header() const { return _header; }

        size_t parsed_length() const { return _parsed_length; }

        // Http parser callback functions
        static int on_message_begin(http_parser *);

        static int on_url(http_parser *, const char *, const size_t);

        static int on_status(http_parser *, const char *, const size_t);

        static int on_header_field(http_parser *, const char *, const size_t);

        static int on_header_value(http_parser *, const char *, const size_t);

        // Returns -1 on error, 0 on success, 1 on success and skip body.
        static int on_headers_complete(http_parser *);

        static int on_body_cb(http_parser *, const char *, const size_t);

        static int on_message_complete_cb(http_parser *);

        const http_parser &parser() const { return _parser; }

        bool read_body_progressively() const { return _read_body_progressively; }

        void set_read_body_progressively(bool read_body_progressively) {
            this->_read_body_progressively = read_body_progressively;
        }

        // Send new parts of the body to the reader. If the body already has some
        // data, feed them to the reader immediately.
        // Any error during the setting will destroy the reader.
        void SetBodyReader(ProgressiveReader *r);

    protected:
        int OnBody(const char *data, size_t size);

        int OnMessageComplete();

        size_t _parsed_length;

    private:
        DISALLOW_COPY_AND_ASSIGN(HttpMessage);

        int UnlockAndFlushToBodyReader(std::unique_lock<mutil::Mutex> &locked);

        HttpParserStage _stage;
        std::string _url;
        HttpMethod _request_method;
        HttpHeader _header;
        bool _read_body_progressively;
        // For mutual exclusion between on_body and SetBodyReader.
        mutil::Mutex _body_mutex;
        // Read body progressively
        ProgressiveReader *_body_reader;
        mutil::IOBuf _body;

        // Parser related members
        struct http_parser _parser;
        std::string _cur_header;
        std::string *_cur_value;

    protected:
        // Only valid when -http_verbose is on
        std::unique_ptr<mutil::IOBufBuilder> _vmsgbuilder;
        size_t _vbodylen;
    };

    std::ostream &operator<<(std::ostream &os, const http_parser &parser);

    // Serialize a http request.
    // header: may be modified in some cases
    // remote_side: used when "Host" is absent
    // content: could be NULL.
    void MakeRawHttpRequest(mutil::IOBuf *request,
                            HttpHeader *header,
                            const mutil::EndPoint &remote_side,
                            const mutil::IOBuf *content);

    // Serialize a http response.
    // header: may be modified in some cases
    // content: cleared after usage. could be NULL.
    void MakeRawHttpResponse(mutil::IOBuf *response,
                             HttpHeader *header,
                             mutil::IOBuf *content);

} // namespace melon

#endif  // MELON_RPC_HTTP_HTTP_MESSAGE_H_
