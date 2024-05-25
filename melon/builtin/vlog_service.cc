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



#if !MELON_WITH_GLOG

#include <melon/rpc/log.h>
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/builtin/vlog_service.h>
#include <melon/builtin/common.h>

namespace melon {

    class VLogPrinter : public VLogSitePrinter {
    public:
        VLogPrinter(bool use_html, std::ostream &os)
                : _use_html(use_html), _os(&os) {}

        void print(const VLogSitePrinter::Site &site) {
            const char *const bar = (_use_html ? "</td><td>" : " | ");
            if (_use_html) {
                *_os << "<tr><td>";
            }
            *_os << site.full_module << ":" << site.line_no << bar
                 << site.current_verbose_level << bar << site.required_verbose_level
                 << bar;
            if (site.current_verbose_level >= site.required_verbose_level) {
                if (_use_html) {
                    *_os << "<span style='font-weight:bold;color:#00A000'>"
                         << "enabled</span>";
                } else {
                    *_os << "enabled";
                }
            } else {
                *_os << "disabled";
            }
            if (_use_html) {
                *_os << "</td></tr>";
            }
            *_os << '\n';
        }

    private:
        bool _use_html;
        std::ostream *_os;

    };

    void VLogService::default_method(::google::protobuf::RpcController *cntl_base,
                                     const ::melon::VLogRequest *,
                                     ::melon::VLogResponse *,
                                     ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        const bool use_html = UseHTML(cntl->http_request());
        mutil::IOBufBuilder os;

        cntl->http_response().set_content_type(
                use_html ? "text/html" : "text/plain");
        if (use_html) {
            os << "<!DOCTYPE html><html><head>" << gridtable_style()
               << "<script src=\"/js/sorttable\"></script></head><body>"
                  "<table class=\"gridtable\" border=\"1\"><tr>"
                  "<th>Module</th><th>Current</th><th>Required</th>"
                  "<th>Status</th></tr>\n";
        } else {
            os << "Module | Current | Required | Status\n";
        }
        VLogPrinter printer(use_html, os);
        print_vlog_sites(&printer);
        if (use_html) {
            os << "</table>\n";
        }

        if (use_html) {
            os << "</body></html>\n";
        }
        os.move_to(cntl->response_attachment());
    }

} // namespace melon

#endif

