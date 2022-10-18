// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef MELON_RPC_HTTP_MESSAGE_H_
#define MELON_RPC_HTTP_MESSAGE_H_

#include <string>                      // std::string
#include "melon/base/profile.h"
#include "melon/io/cord_buf.h"               // melon::cord_buf
#include "melon/base/scoped_lock.h"
#include "melon/base/endpoint.h"
#include "melon/rpc/details/http_parser.h"  // http_parser
#include "melon/rpc/http_header.h"          // HttpHeader
#include "melon/rpc/progressive_reader.h"   // ProgressiveReader


namespace melon::rpc {

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
        HttpMessage(bool read_body_progressively = false);

        ~HttpMessage();

        const melon::cord_buf &body() const { return _body; }

        melon::cord_buf &body() { return _body; }

        // Parse from array, length=0 is treated as EOF.
        // Returns bytes parsed, -1 on failure.
        ssize_t ParseFromArray(const char *data, const size_t length);

        // Parse from melon::cord_buf.
        // Emtpy `buf' is sliently ignored, which is different from ParseFromArray.
        // Returns bytes parsed, -1 on failure.
        ssize_t ParseFromCordBuf(const melon::cord_buf &buf);

        bool Completed() const { return _stage == HTTP_ON_MESSAGE_COMPLETE; }

        HttpParserStage stage() const { return _stage; }

        HttpHeader &header() { return _header; }

        const HttpHeader &header() const { return _header; }

        size_t parsed_length() const { return _parsed_length; }

        // Http parser callback functions
        static int on_message_begin(http_parser *);

        static int on_url(http_parser *, const char *, const size_t);

        static int on_status(http_parser *, const char *, const size_t);

        static int on_header_field(http_parser *, const char *, const size_t);

        static int on_header_value(http_parser *, const char *, const size_t);

        static int on_headers_complete(http_parser *);

        static int on_body_cb(http_parser *, const char *, const size_t);

        static int on_message_complete_cb(http_parser *);

        const http_parser &parser() const { return _parser; }

        bool read_body_progressively() const { return _read_body_progressively; }

        // Send new parts of the body to the reader. If the body already has some
        // data, feed them to the reader immediately.
        // Any error during the setting will destroy the reader.
        void SetBodyReader(ProgressiveReader *r);

    protected:
        int OnBody(const char *data, size_t size);

        int OnMessageComplete();

        size_t _parsed_length;

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(HttpMessage);

        int UnlockAndFlushToBodyReader(std::unique_lock<std::mutex> &locked);

        HttpParserStage _stage;
        std::string _url;
        HttpHeader _header;
        bool _read_body_progressively;
        // For mutual exclusion between on_body and SetBodyReader.
        std::mutex _body_mutex;
        // Read body progressively
        ProgressiveReader *_body_reader;
        melon::cord_buf _body;

        // Parser related members
        struct http_parser _parser;
        std::string _cur_header;
        std::string *_cur_value;

    protected:
        // Only valid when -http_verbose is on
        melon::cord_buf_builder *_vmsgbuilder;
        size_t _vbodylen;
    };

    std::ostream &operator<<(std::ostream &os, const http_parser &parser);

// Serialize a http request.
// header: may be modified in some cases
// remote_side: used when "Host" is absent
// content: could be NULL.
    void MakeRawHttpRequest(melon::cord_buf *request,
                            HttpHeader *header,
                            const melon::base::end_point &remote_side,
                            const melon::cord_buf *content);

    // Serialize a http response.
    // header: may be modified in some cases
    // content: cleared after usage. could be NULL.
    void MakeRawHttpResponse(melon::cord_buf *response,
                             HttpHeader *header,
                             melon::cord_buf *content);

} // namespace melon::rpc

#endif  // MELON_RPC_HTTP_MESSAGE_H_
