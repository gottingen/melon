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
// Created by jeff on 24-6-27.
//

#include <melon/br/connections.h>
#include <melon/rpc/server.h>
#include <melon/rpc/acceptor.h>
#include <collie/nlohmann/json.hpp>
#include <turbo/flags/flag.h>
#include <turbo/log/logging.h>
#include <turbo/flags/reflection.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/match.h>
#include <turbo/times/time.h>
#include <ostream>
#include <iomanip>
#include <netinet/tcp.h>
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/socket_map.h>           // SocketMapList
#include <melon/rpc/acceptor.h>             // Acceptor
#include <melon/rpc/server.h>
#include <melon/utility/class_name.h>
#include <turbo/flags/flag.h>

TURBO_FLAG(bool, show_hostname_instead_of_ip, false,
           "/connections shows hostname instead of ip").on_validate(turbo::AllPassValidator<bool>::validate);


TURBO_FLAG(int32_t, max_shown_connections, 1024,
           "Print stats of at most so many connections (soft limit)");

namespace melon {

    std::string endpoint_name(const mutil::EndPoint &nop) {
        char buf[128];
        if (turbo::get_flag(FLAGS_show_hostname_instead_of_ip) &&
            mutil::endpoint2hostname(nop, buf, sizeof(buf)) == 0) {
            return buf;
        } else {
            return mutil::endpoint2str(nop).c_str();
        }
    }

    inline const char *SSLStateToYesNo(SSLState s) {
        switch (s) {
            case SSL_UNKNOWN:
                return (" - ");
            case SSL_CONNECTING:
            case SSL_CONNECTED:
                return "Yes";
            case SSL_OFF:
                return "No ";
        }
        return "Bad";
    }

    void ListConnectionProcessor::print_connections(nlohmann::json &results, const std::vector<SocketId> &conns, bool is_channel_conn) {
        if (conns.empty()) {
            return;
        }

        SocketStat stat;
        std::vector<SocketId> first_id;
        for (size_t i = 0; i < conns.size(); ++i) {
            nlohmann::json result;
            const SocketId socket_id = conns[i];
            SocketUniquePtr ptr;
            bool failed = false;
            if (Socket::Address(socket_id, &ptr) != 0) {
                int ret = Socket::AddressFailedAsWell(socket_id, &ptr);
                if (ret < 0) {
                    continue;
                } else if (ret > 0) {
                    if (!ptr->HCEnabled()) {
                        // Sockets without HC will soon be destroyed
                        continue;
                    }
                    failed = true;
                }
            }

            if (failed) {
                result["state"] = "broken";
                result["created_time"] = "-";
                result["remote_side"] = endpoint_name(ptr->remote_side());
                if (is_channel_conn) {
                    result["local_side"] = ptr->local_side().port;
                    result["recent_error_count"] = ptr->recent_error_count();
                    result["isolated_times"] = ptr->isolated_times();
                }
                result["ssl_state"] = SSLStateToYesNo(ptr->ssl_state());
                result["protocol"] = "-";
                result["fd"] = ptr->fd();
                result["in_bytes_s"] = 0;
                result["in_num_messages_s"] = 0;
                result["in_size_m"] = 0;
                result["in_num_messages_m"] = 0;
                result["out_size_s"] = 0;
                result["out_num_messages_s"] = 0;
                result["out_size_m"] = 0;
                result["out_num_messages_m"] = 0;
                result["rtt"] = "-";
            } else {
                {
                    SocketUniquePtr agent_sock;
                    if (ptr->PeekAgentSocket(&agent_sock) == 0) {
                        ptr.swap(agent_sock);
                    }
                }
                // Get name of the protocol. In principle we can dynamic_cast the
                // socket user to InputMessenger but I'm not sure if that's a bit
                // slow (because we have many connections here).
                int pref_index = ptr->preferred_index();
                SocketUniquePtr first_sub;
                int pooled_count = -1;
                if (ptr->HasSocketPool()) {
                    int numfree = 0;
                    int numinflight = 0;
                    if (ptr->GetPooledSocketStats(&numfree, &numinflight)) {
                        pooled_count = numfree + numinflight;
                    }
                    // Check preferred_index of any pooled sockets.
                    ptr->ListPooledSockets(&first_id, 1);
                    if (!first_id.empty()) {
                        Socket::Address(first_id[0], &first_sub);
                    }
                }
                const char *pref_prot = "-";
                if (ptr->user() == _server->_am) {
                    pref_prot = _server->_am->NameOfProtocol(pref_index);
                } else if (ptr->CreatedByConnect()) {
                    pref_prot = get_client_side_messenger()->NameOfProtocol(pref_index);
                }
                if (strcmp(pref_prot, "unknown") == 0) {
                    // Show unknown protocol as - to be consistent with other columns.
                    pref_prot = "-";
                } else if (strcmp(pref_prot, "h2") == 0) {
                    if (!ptr->is_ssl()) {
                        pref_prot = "h2c";
                    }
                }
                ptr->GetStat(&stat);
                result["state"] = "connected";
                result["created_time"] = turbo::Time::format(turbo::Time::from_microseconds(ptr->reset_fd_real_us()));
                int rttfd = ptr->fd();
                if (rttfd < 0 && first_sub != NULL) {
                    rttfd = first_sub->fd();
                }

                bool got_rtt = false;
                uint32_t srtt = 0;
                uint32_t rtt_var = 0;
                // get rtt
#if defined(OS_LINUX)
                struct tcp_info ti;
                socklen_t len = sizeof(ti);
                if (0 == getsockopt(rttfd, SOL_TCP, TCP_INFO, &ti, &len)) {
                    got_rtt = true;
                    srtt = ti.tcpi_rtt;
                    rtt_var = ti.tcpi_rttvar;
                }
#elif defined(OS_MACOSX)
                struct tcp_connection_info ti;
                socklen_t len = sizeof(ti);
                if (0 == getsockopt(rttfd, IPPROTO_TCP, TCP_CONNECTION_INFO, &ti, &len)) {
                    got_rtt = true;
                    srtt = ti.tcpi_srtt;
                    rtt_var = ti.tcpi_rttvar;
                }
#endif
                char rtt_display[32];
                if (got_rtt) {
                    snprintf(rtt_display, sizeof(rtt_display), "%.1f/%.1f",
                             srtt / 1000.0, rtt_var / 1000.0);
                } else {
                    strcpy(rtt_display, "-");
                }
                result["remote_side"] = endpoint_name(ptr->remote_side());
                if (is_channel_conn) {
                    if (ptr->local_side().port > 0) {
                        result["local_side"] = ptr->local_side().port;
                    } else {
                        result["local_side"] = "-";
                    }
                    result["recent_error_count"] = ptr->recent_error_count();
                    result["isolated_times"] = ptr->isolated_times();
                }
                result["ssl_state"] = SSLStateToYesNo(ptr->ssl_state());
                char protname[32];
                if (pooled_count < 0) {
                    snprintf(protname, sizeof(protname), "%s", pref_prot);
                } else {
                    snprintf(protname, sizeof(protname), "%s*%d", pref_prot,
                             pooled_count);
                }
                result["protocol"] = protname;
                if (ptr->fd() >= 0) {
                    result["fd"] = ptr->fd();
                } else {
                    result["fd"] = "-";
                }
                result["in_bytes_s"] = stat.in_size_s;
                result["in_num_messages_s"] = stat.in_num_messages_s;
                result["in_size_m"] = stat.in_size_m;
                result["in_num_messages_m"] = stat.in_num_messages_m;
                result["out_size_s"] = stat.out_size_s;
                result["out_num_messages_s"] = stat.out_num_messages_s;
                result["out_size_m"] = stat.out_size_m;
                result["out_num_messages_m"] = stat.out_num_messages_m;
                result["rtt"] = rtt_display;
            }
            result["socket_id"] = socket_id;
            results["connections"].push_back(result);
        }
    }

    void ListConnectionProcessor::process(const melon::RestfulRequest *request, melon::RestfulResponse *response) {
        std::string match_str;
        size_t max_shown = (size_t)turbo::get_flag(FLAGS_max_shown_connections);
        auto *ptr = request->uri().GetQuery("all");
        if (ptr) {
            max_shown = std::numeric_limits<size_t>::max();
        }
        Acceptor *am = _server->_am;
        Acceptor *internal_am = _server->_internal_am;
        bool has_uncopied = false;
        std::vector<SocketId> conns;
        // NOTE: not accurate count.
        size_t num_conns = am->ConnectionCount();
        am->ListConnections(&conns, max_shown);
        if (conns.size() == max_shown && num_conns > conns.size()) {
            // OK to have false positive
            has_uncopied = true;
        }
        if (internal_am) {
            size_t num_conns2 = internal_am->ConnectionCount();
            std::vector<SocketId> internal_conns;
            // Connections to internal_port are generally small, thus
            // -max_shown_connections is treated as a soft limit
            internal_am->ListConnections(&internal_conns, max_shown);
            if (internal_conns.size() == max_shown &&
                num_conns2 > internal_conns.size()) {
                // OK to have false positive
                has_uncopied = true;
            }
            conns.insert(conns.end(), internal_conns.begin(), internal_conns.end());
        }

        response->set_content_json();
        response->set_access_control_all_allow();
        response->set_status_code(200);
        nlohmann::json j;
        j["code"] = 0;
        j["message"] = "success";
        j["has_more"] = has_uncopied;
        print_connections(j, conns, false);
        SocketMapList(&conns);
        print_connections(j, conns, true);
        response->set_body(j.dump());
    }

    void SocketInfoProcessor::process(const melon::RestfulRequest *request, melon::RestfulResponse *response) {
        auto flag_list = turbo::get_all_flags();
        std::string input_str = request->body().to_string();
        response->set_content_json();
        response->set_access_control_all_allow();
        response->set_status_code(200);
        nlohmann::json j;
        std::stringstream os;
        auto *ptr = request->uri().GetQuery("id");
        if (!ptr) {
            j["code"] = 1;
            j["message"] = "SocketId is required";
            response->set_body(j.dump());
            return;
        }

        uint64_t socket_id;
        if (!turbo::simple_atoi(*ptr, &socket_id)) {
            j["code"] = 1;
            j["message"] = "SocketId is invalid";
            response->set_body(j.dump());
            return;
        }
        Socket::DebugSocket(os, socket_id);

        j["code"] = 0;
        j["message"] = "ok";
        j["data"] = os.str();
        response->set_body(j.dump());
    }
}  // namespace melon