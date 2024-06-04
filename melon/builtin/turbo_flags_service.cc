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



#include <ostream>
#include <vector>                           // std::vector
#include <set>
// CommandLineFlagInfo
#include <melon/utility/string_printf.h>
#include <melon/utility/string_splitter.h>

#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/controller.h>           // Controller
#include <melon/proto/rpc/errno.pb.h>
#include <melon/rpc/server.h>
#include <melon/builtin/common.h>
#include <melon/builtin/turbo_flags_service.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <turbo/container/flat_hash_set.h>

TURBO_FLAG(std::string, server_name, "melon", "server name");
TURBO_FLAG(int32_t, server_port, 8080, "server port");
namespace melon {

    DEFINE_bool(immutable_turbo_flags, false, "turbo flags on /flags page can't be modified");

    // Replace some characters with html replacements. If the input string does
    // not need to be changed, return its const reference directly, otherwise put
    // the replaced string in backing string and return its const reference.
    // TODO(gejun): Make this function more general.
    static std::string HtmlReplace(const std::string &s) {
        std::string b;
        size_t last_pos = 0;
        while (1) {
            size_t new_pos = s.find_first_of("<>&", last_pos);
            if (new_pos == std::string::npos) {
                if (b.empty()) {  // no special characters.
                    return s;
                }
                b.append(s.data() + last_pos, s.size() - last_pos);
                return b;
            }
            b.append(s.data() + last_pos, new_pos - last_pos);
            switch (s[new_pos]) {
                case '<' :
                    b.append("&lt;");
                    break;
                case '>' :
                    b.append("&gt;");
                    break;
                case '&' :
                    b.append("&amp;");
                    break;
                default:
                    b.push_back(s[new_pos]);
                    break;
            }
            last_pos = new_pos + 1;
        }
    }

    static void PrintFlag(std::ostream &os, const turbo::CommandLineFlag *flag,
                          bool use_html) {
        if (use_html) {
            os << "<tr><td>";
        }
        os << flag->Name();
        /*
        if (flag.has_validator_fn) {
            if (use_html) {
                os << " (<a href='/flags/" << flag->Name()
                   << "?setvalue&withform'>R</a>)";
            } else {
                os << " (R)";
            }
        }*/
        auto defualt_value = flag->DefaultValue();
        auto current_value = flag->CurrentValue();
        bool is_default = (defualt_value == current_value);
        os << (use_html ? "</td><td>" : " | ");
        if (!is_default && use_html) {
            os << "<span style='color:#FF0000'>";
        }
        if (!current_value.empty()) {
            os << (use_html ? HtmlReplace(current_value)
                            : current_value);
        } else {
            os << (use_html ? "&nbsp;" : " ");
        }
        if (!is_default) {
            if (defualt_value != current_value) {
                os << " (default:" << (use_html ?
                                       HtmlReplace(defualt_value) :
                                       defualt_value) << ')';
            }
            if (use_html) {
                os << "</span>";
            }
        }
        os << (use_html ? "</td><td>" : " | ") << flag->Help()
           << (use_html ? "</td><td>" : " | ") << flag->Filename();
        if (use_html) {
            os << "</td></tr>";
        }
    }

    void TurboFlagsService::set_value_page(Controller *cntl,
                                      ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        const std::string &name = cntl->http_request().unresolved_path();
        auto fflag = turbo::FindCommandLineFlag(name);
        if (!fflag) {
            cntl->SetFailed(ENOMETHOD, "No such gflag");
            return;
        }
        mutil::IOBufBuilder os;
        const bool is_string = fflag->IsOfType<std::string>();
        os << "<!DOCTYPE html><html><body>"
              "<form action='' method='get'>"
              " Set `" << name << "' from ";
        if (is_string) {
            os << '"';
        }
        os << fflag->CurrentValue();
        if (is_string) {
            os << '"';
        }
        os << " to <input name='setvalue' value=''>"
              "  <button>go</button>"
              "</form>"
              "</body></html>";
        os.move_to(cntl->response_attachment());
    }

    void TurboFlagsService::default_method(::google::protobuf::RpcController *cntl_base,
                                      const ::melon::TurboFlagsRequest *,
                                      ::melon::TurboFlagsResponse *,
                                      ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        const std::string *value_str =
                cntl->http_request().uri().GetQuery(SETVALUE_STR);
        const std::string &constraint = cntl->http_request().unresolved_path();

        const bool use_html = UseHTML(cntl->http_request());
        cntl->http_response().set_content_type(
                use_html ? "text/html" : "text/plain");

        if (value_str != nullptr) {
            // reload value if ?setvalue=VALUE is present.
            if (constraint.empty()) {
                cntl->SetFailed(ENOMETHOD, "Require turbo flag name");
                return;
            }
            if (use_html && cntl->http_request().uri().GetQuery("withform")) {
                return set_value_page(cntl, done_guard.release());
            }
            auto fflag = turbo::FindCommandLineFlag(constraint);
            if (!fflag) {
                cntl->SetFailed(ENOMETHOD, "No such turbo flag");
                return;
            }
            /*
            if (!info.has_validator_fn) {
                cntl->SetFailed(EPERM, "A reloadable turbo flag must have validator");
                return;
            }*/
            if (FLAGS_immutable_turbo_flags) {
                cntl->SetFailed(EPERM, "Cannot modify `%s' because -immutable_turbo_flags is on",
                                constraint.c_str());
                return;
            }
            auto r = fflag->ParseFrom(*value_str, nullptr);
            if (!r) {
                cntl->SetFailed(EPERM, "Fail to set `%s' to %s",
                                constraint.c_str(),
                                (value_str->empty() ? "empty string" : value_str->c_str()));
                return;
            }
            mutil::IOBufBuilder os;
            os << "Set `" << constraint << "' to " << *value_str;
            if (use_html) {
                os << "<br><a href='/flags'>[back to flags]</a>";
            }
            os.move_to(cntl->response_attachment());
            return;
        }

        // Parse query-string which is comma-separated flagnames/wildcards.
        std::vector<std::string> wildcards;
        turbo::flat_hash_set<std::string> exact;
        if (!constraint.empty()) {
            for (mutil::StringMultiSplitter sp(constraint.c_str(), ",;"); sp != nullptr; ++sp) {
                std::string name(sp.field(), sp.length());
                if (name.find_first_of("$*") != std::string::npos) {
                    wildcards.push_back(name);
                } else {
                    exact.insert(name);
                }
            }
        }

        // Print header of the table
        mutil::IOBufBuilder os;
        if (use_html) {
            os << "<!DOCTYPE html><html><head>\n" << gridtable_style()
               << "<script language=\"javascript\" type=\"text/javascript\" src=\"/js/jquery_min\"></script>\n"
               << TabsHead()
               << "</head><body>";
            cntl->server()->PrintTabsBody(os, "flags");
            os << "<table class=\"gridtable\" border=\"1\"><tr><th>Name</th><th>Value</th>"
                  "<th>Description</th><th>Defined At</th></tr>\n";
        } else {
            os << "Name | Value | Description | Defined At\n"
                  "---------------------------------------\n";
        }

        if (!constraint.empty() && wildcards.empty()) {
            // Only exact names. We don't have to iterate all flags in this case.
            for (auto it = exact.begin();
                 it != exact.end(); ++it) {
                auto finfo = turbo::FindCommandLineFlag(*it);
                if (finfo != nullptr) {
                    PrintFlag(os, finfo, use_html);
                    os << '\n';
                }
            }

        } else {
            // Iterate all flags and filter.
            auto flag_list = turbo::GetAllFlags();
            for (auto it = flag_list.begin(); it != flag_list.end(); ++it) {
                if (!constraint.empty() &&
                    exact.find(it->first) == exact.end() &&
                    !MatchAnyWildcard(std::string(it->first), wildcards)) {
                    continue;
                }
                PrintFlag(os, it->second, use_html);
                os << '\n';
            }
        }
        if (use_html) {
            os << "</table></body></html>\n";
        }
        os.move_to(cntl->response_attachment());
        cntl->set_response_compress_type(COMPRESS_TYPE_GZIP);
    }

    void TurboFlagsService::GetTabInfo(TabInfoList *info_list) const {
        TabInfo *info = info_list->add();
        info->path = "/tflags";
        info->tab_name = "tflags";
    }

} // namespace melon
