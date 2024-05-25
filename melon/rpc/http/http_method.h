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



#ifndef  MELON_RPC_HTTP_HTTP_METHOD_H_
#define  MELON_RPC_HTTP_HTTP_METHOD_H_

namespace melon {

    enum HttpMethod {
        HTTP_METHOD_DELETE = 0,
        HTTP_METHOD_GET = 1,
        HTTP_METHOD_HEAD = 2,
        HTTP_METHOD_POST = 3,
        HTTP_METHOD_PUT = 4,
        HTTP_METHOD_CONNECT = 5,
        HTTP_METHOD_OPTIONS = 6,
        HTTP_METHOD_TRACE = 7,
        HTTP_METHOD_COPY = 8,
        HTTP_METHOD_LOCK = 9,
        HTTP_METHOD_MKCOL = 10,
        HTTP_METHOD_MOVE = 11,
        HTTP_METHOD_PROPFIND = 12,
        HTTP_METHOD_PROPPATCH = 13,
        HTTP_METHOD_SEARCH = 14,
        HTTP_METHOD_UNLOCK = 15,
        HTTP_METHOD_REPORT = 16,
        HTTP_METHOD_MKACTIVITY = 17,
        HTTP_METHOD_CHECKOUT = 18,
        HTTP_METHOD_MERGE = 19,
        HTTP_METHOD_MSEARCH = 20,  // M-SEARCH
        HTTP_METHOD_NOTIFY = 21,
        HTTP_METHOD_SUBSCRIBE = 22,
        HTTP_METHOD_UNSUBSCRIBE = 23,
        HTTP_METHOD_PATCH = 24,
        HTTP_METHOD_PURGE = 25,
        HTTP_METHOD_MKCALENDAR = 26
    };

    // Returns literal description of `http_method'. "UNKNOWN" on not found.
    const char *HttpMethod2Str(HttpMethod http_method);

    // Convert case-insensitive `method_str' to enum HttpMethod.
    // Returns true on success.
    bool Str2HttpMethod(const char *method_str, HttpMethod *method);

} // namespace melon

#endif  // MELON_RPC_HTTP_HTTP_METHOD_H_
