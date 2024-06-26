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


#include <melon/utility/macros.h>                           // ARRAY_SIZE
#include <melon/utility/iobuf.h>                            // mutil::IOBuf
#include <melon/rpc/controller.h>                  // Controller
#include <melon/builtin/sorttable_js.h>
#include <melon/builtin/jquery_min_js.h>
#include <melon/builtin/flot_min_js.h>
#include <melon/builtin/viz_min_js.h>
#include <melon/builtin/vue_js.h>
#include <melon/builtin/get_js_service.h>
#include <melon/builtin/common.h>


namespace melon {

    static const char *g_last_modified = "Wed, 16 Sep 2015 01:25:30 GMT";

    static void SetExpires(HttpHeader *header, time_t seconds) {
        char buf[256];
        time_t now = time(0);
        Time2GMT(now, buf, sizeof(buf));
        header->SetHeader("Date", buf);
        Time2GMT(now + seconds, buf, sizeof(buf));
        header->SetHeader("Expires", buf);
    }

    void GetJsService::sorttable(
            ::google::protobuf::RpcController *controller,
            const GetJsRequest * /*request*/,
            GetJsResponse * /*response*/,
            ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("application/javascript");
        SetExpires(&cntl->http_response(), 80000);
        cntl->response_attachment().append(sorttable_js_iobuf());
    }

    void GetJsService::jquery_min(
            ::google::protobuf::RpcController *controller,
            const GetJsRequest * /*request*/,
            GetJsResponse * /*response*/,
            ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("application/javascript");
        SetExpires(&cntl->http_response(), 600);

        const std::string *ims =
                cntl->http_request().GetHeader("If-Modified-Since");
        if (ims != NULL && *ims == g_last_modified) {
            cntl->http_response().set_status_code(HTTP_STATUS_NOT_MODIFIED);
            return;
        }
        cntl->http_response().SetHeader("Last-Modified", g_last_modified);

        if (SupportGzip(cntl)) {
            cntl->http_response().SetHeader("Content-Encoding", "gzip");
            cntl->response_attachment().append(jquery_min_js_iobuf_gzip());
        } else {
            cntl->response_attachment().append(jquery_min_js_iobuf());
        }
    }

    void GetJsService::flot_min(
            ::google::protobuf::RpcController *controller,
            const GetJsRequest * /*request*/,
            GetJsResponse * /*response*/,
            ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("application/javascript");
        SetExpires(&cntl->http_response(), 80000);

        const std::string *ims =
                cntl->http_request().GetHeader("If-Modified-Since");
        if (ims != NULL && *ims == g_last_modified) {
            cntl->http_response().set_status_code(HTTP_STATUS_NOT_MODIFIED);
            return;
        }
        cntl->http_response().SetHeader("Last-Modified", g_last_modified);

        if (SupportGzip(cntl)) {
            cntl->http_response().SetHeader("Content-Encoding", "gzip");
            cntl->response_attachment().append(flot_min_js_iobuf_gzip());
        } else {
            cntl->response_attachment().append(flot_min_js_iobuf());
        }
    }

    void GetJsService::viz_min(
            ::google::protobuf::RpcController *controller,
            const GetJsRequest * /*request*/,
            GetJsResponse * /*response*/,
            ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("application/javascript");
        SetExpires(&cntl->http_response(), 80000);

        const std::string *ims =
                cntl->http_request().GetHeader("If-Modified-Since");
        if (ims != NULL && *ims == g_last_modified) {
            cntl->http_response().set_status_code(HTTP_STATUS_NOT_MODIFIED);
            return;
        }
        cntl->http_response().SetHeader("Last-Modified", g_last_modified);

        if (SupportGzip(cntl)) {
            cntl->http_response().SetHeader("Content-Encoding", "gzip");
            cntl->response_attachment().append(viz_min_js_iobuf_gzip());
        } else {
            cntl->response_attachment().append(viz_min_js_iobuf());
        }
    }

    void GetJsService::vue(
            ::google::protobuf::RpcController *controller,
            const GetJsRequest * /*request*/,
            GetJsResponse * /*response*/,
            ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("application/javascript");
        SetExpires(&cntl->http_response(), 80000);

        const std::string *ims =
                cntl->http_request().GetHeader("If-Modified-Since");
        if (ims != NULL && *ims == g_last_modified) {
            cntl->http_response().set_status_code(HTTP_STATUS_NOT_MODIFIED);
            return;
        }
        cntl->http_response().SetHeader("Last-Modified", g_last_modified);

        if (SupportGzip(cntl)) {
            cntl->http_response().SetHeader("Content-Encoding", "gzip");
            cntl->response_attachment().append(vue_js_iobuf_gzip());
        } else {
            cntl->response_attachment().append(vue_js_iobuf());
        }
    }

} // namespace melon
