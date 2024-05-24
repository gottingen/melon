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

#include <melon/naming/sns_naming_service.h>
#include <gflags/gflags.h>
#include <thread>
#include <melon/fiber/fiber.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/controller.h>
#include <melon/utility/third_party/rapidjson/document.h>
#include <melon/utility/third_party/rapidjson/memorybuffer.h>
#include <melon/utility/third_party/rapidjson/writer.h>
#include <melon/utility/string_splitter.h>

namespace melon::naming {

    DEFINE_string(sns_server, "", "The address of sns api");
    DEFINE_int32(sns_timeout_ms, 3000, "Timeout for discovery requests");
    DEFINE_string(sns_env, "prod", "Environment of services");
    DEFINE_string(sns_status, "1", "Status of services. 1 for ready, 2 for not ready, 3 for all");
    DEFINE_string(sns_zone, "", "Zone of services");
    DEFINE_string(sns_coloe, "", "Zone of services");
    DEFINE_int32(sns_renew_interval_s, 30, "The interval between two consecutive renews");
    DEFINE_int32(sns_reregister_threshold, 3, "The renew error threshold beyond"
                                              " which Register would be called again");

    static std::once_flag g_init_sns_channel_once;
    static Channel *g_sns_channel{nullptr};

    static inline melon::PeerStatus to_peer_status(const std::string &status) {
        if (status == "1") {
            return melon::PeerStatus::NORMAL;
        } else if (status == "2") {
            return melon::PeerStatus::SLOW;
        } else if (status == "3"){
            return melon::PeerStatus::FULL;
        } else {
            return melon::PeerStatus::DEAD;
        }
    }

    static void NewSnsChannel() {
        ChannelOptions channel_options;
        channel_options.protocol = PROTOCOL_HTTP;
        channel_options.timeout_ms = FLAGS_sns_timeout_ms;
        channel_options.connect_timeout_ms = FLAGS_sns_timeout_ms / 3;
        g_sns_channel = new Channel;
        if (g_sns_channel->Init(FLAGS_sns_server.c_str(), "rr", &channel_options) != 0) {
            MLOG(ERROR) << "Fail to init channel to " << FLAGS_sns_server;
            return;
        }
    }

    static bool is_valid(const melon::SnsPeer &peer) {
        return peer.has_app_name() && !peer.app_name().empty() &&
               peer.has_zone() && !peer.zone().empty() &&
               peer.has_servlet_name() && !peer.servlet_name().empty() &&
               peer.has_env() && !peer.env().empty() &&
               peer.has_color() && !peer.color().empty() &&
               peer.has_address() && !peer.address().empty();
    }

    inline Channel *GetOrNewSnsChannel() {
        std::call_once(g_init_sns_channel_once, NewSnsChannel);
        return g_sns_channel;
    }

    SnsNamingClient::SnsNamingClient()
            : _th(INVALID_FIBER), _registered(false) {
    }

    SnsNamingClient::~SnsNamingClient() {
        if (_registered.load(mutil::memory_order_acquire)) {
            fiber_stop(_th);
            fiber_join(_th, NULL);
            do_cancel();
        }
    }

    int SnsNamingClient::register_peer(const melon::SnsPeer &params) {
        if (_registered.load(mutil::memory_order_relaxed) ||
            _registered.exchange(true, mutil::memory_order_release)) {
            return 0;
        }
        if (!is_valid(params)) {
            return -1;
        }
        _params = params;

        if (do_register() != 0) {
            return -1;
        }
        if (fiber_start_background(&_th, NULL, periodic_renew, this) != 0) {
            MLOG(ERROR) << "Fail to start background PeriodicRenew";
            return -1;
        }
        return 0;
    }

    int SnsNamingClient::do_register() {
        Channel *chan = GetOrNewSnsChannel();
        if (NULL == chan) {
            MLOG(ERROR) << "Fail to create discovery channel";
            return -1;
        }
        Controller cntl;
        melon::SnsService_Stub stub(chan);
        melon::SnsResponse response;
        stub.registry(&cntl, &_params, &response, NULL);
        if (cntl.Failed()) {
            MLOG(ERROR) << "Fail to register peer: " << cntl.ErrorText();
            return -1;
        }
        if (response.errcode() != melon::Errno::OK && response.errcode() != melon::Errno::AlreadyExists) {
            MLOG(ERROR) << "Fail to register peer: " << response.errmsg();
            return -1;
        }
        _current_discovery_server = cntl.remote_side();
        return 0;
    }

    int SnsNamingClient::do_renew() const {
        Channel *chan = GetOrNewSnsChannel();
        if (NULL == chan) {
            MLOG(ERROR) << "Fail to create discovery channel";
            return -1;
        }
        Controller cntl;
        melon::SnsService_Stub stub(chan);
        melon::SnsResponse response;
        auto request = _params;
        request.set_status(to_peer_status(FLAGS_sns_status));
        stub.update(&cntl, &request, &response, NULL);
        if (cntl.Failed()) {
            MLOG(ERROR) << "Fail to register peer: " << cntl.ErrorText();
            return -1;
        }
        if (response.errcode() != melon::Errno::OK) {
            MLOG(ERROR) << "Fail to register peer: " << response.errmsg();
            return -1;
        }
        return 0;
    }

    int SnsNamingClient::do_cancel() const {
        Channel *chan = GetOrNewSnsChannel();
        if (NULL == chan) {
            MLOG(ERROR) << "Fail to create discovery channel";
            return -1;
        }
        Controller cntl;
        melon::SnsService_Stub stub(chan);
        melon::SnsResponse response;
        stub.cancel(&cntl, &_params, &response, NULL);
        if (cntl.Failed()) {
            MLOG(ERROR) << "Fail to register peer: " << cntl.ErrorText();
            return -1;
        }
        if (response.errcode() != melon::Errno::OK) {
            MLOG(ERROR) << "Fail to register peer: " << response.errmsg();
            return -1;
        }
        return 0;
    }

    void *SnsNamingClient::periodic_renew(void *arg) {
        auto *sns = static_cast<SnsNamingClient *>(arg);
        int consecutive_renew_error = 0;
        int64_t init_sleep_s = FLAGS_sns_renew_interval_s / 2 +
                               mutil::fast_rand_less_than(FLAGS_sns_renew_interval_s / 2);
        if (fiber_usleep(init_sleep_s * 1000000) != 0) {
            if (errno == ESTOP) {
                return NULL;
            }
        }

        while (!fiber_stopped(fiber_self())) {
            if (consecutive_renew_error == FLAGS_sns_reregister_threshold) {
                MLOG(WARNING) << "Re-register since discovery renew error threshold reached";
                // Do register until succeed or Cancel is called
                while (!fiber_stopped(fiber_self())) {
                    if (sns->do_register() == 0) {
                        break;
                    }
                    fiber_usleep(FLAGS_sns_renew_interval_s * 1000000);
                }
                consecutive_renew_error = 0;
            }
            if (sns->do_renew() != 0) {
                consecutive_renew_error++;
                continue;
            }
            consecutive_renew_error = 0;
            fiber_usleep(FLAGS_sns_renew_interval_s * 1000000);
        }
        return NULL;
    }

    int SnsNamingService::GetServers(const char *service_name,
                                     std::vector<ServerNode> *servers) {
        if (service_name == NULL || *service_name == '\0' ||
            FLAGS_sns_env.empty() ||
            FLAGS_sns_status.empty() ||
            FLAGS_sns_zone.empty() ||
            FLAGS_sns_coloe.empty()) {
            MLOG_ONCE(ERROR) << "Invalid parameters";
            return -1;
        }

        Channel *chan = GetOrNewSnsChannel();
        if (NULL == chan) {
            MLOG(ERROR) << "Fail to create discovery channel";
            return -1;
        }
        Controller cntl;
        melon::SnsService_Stub stub(chan);
        melon::SnsResponse response;
        melon::SnsRequest request;
        request.set_app_name(service_name);
        auto env_sp = mutil::StringSplitter(FLAGS_sns_env, ',');
        while (env_sp) {
            request.add_env(env_sp.field());
            ++env_sp;
        }
        auto color_sp = mutil::StringSplitter(FLAGS_sns_coloe, ',');
        while (color_sp) {
            request.add_color(color_sp.field());
            ++color_sp;
        }
        auto zone_sp = mutil::StringSplitter(FLAGS_sns_zone, ',');
        while (zone_sp) {
            request.add_zones(zone_sp.field());
            ++zone_sp;
        }

        stub.naming(&cntl, &request, &response, NULL);
        if (cntl.Failed()) {
            MLOG(ERROR) << "Fail to register peer: " << cntl.ErrorText();
            return -1;
        }
        if (response.errcode() != melon::Errno::OK) {
            MLOG(ERROR) << "Fail to register peer: " << response.errmsg();
            return -1;
        }

        for (int i = 0; i < response.servlets_size(); ++i) {
            const auto &peer = response.servlets(i);
            if (!is_valid(peer)) {
                MLOG(ERROR) << "Invalid peer: " << peer.DebugString();
                continue;
            }
            ServerNode node;
            if(mutil::str2endpoint(peer.address().c_str(), &node.addr) != 0) {
                MLOG(ERROR) << "Invalid address: " << peer.address();
                continue;
            }
            node.tag = peer.app_name() + "." + peer.zone() + "." + peer.env() + "." + peer.color();
            servers->push_back(node);
        }
        return 0;
    }

    void SnsNamingService::Describe(std::ostream &os,
                                    const DescribeOptions &) const {
        os << "sns";
        return;
    }

    NamingService *SnsNamingService::New() const {
        return new SnsNamingService;
    }

    void SnsNamingService::Destroy() {
        delete this;
    }


}  // namespace melon::naming
