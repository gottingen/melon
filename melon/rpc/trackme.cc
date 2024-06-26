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



#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <melon/utility/fast_rand.h>
#include <melon/rpc/log.h>
#include <melon/rpc/channel.h>
#include <melon/proto/rpc/trackme.pb.h>
#include <melon/rpc/policy/hasher.h>
#include <melon/utility/files/scoped_file.h>

namespace melon {

    DEFINE_string(trackme_server, "", "Where the TrackMe requests are sent to");

    static const int32_t TRACKME_MIN_INTERVAL = 30;
    static const int32_t TRACKME_MAX_INTERVAL = 600;
    static int32_t s_trackme_interval = TRACKME_MIN_INTERVAL;
    // Protecting global vars on trackme
    static pthread_mutex_t s_trackme_mutex = PTHREAD_MUTEX_INITIALIZER;
    // For contacting with trackme_server.
    static Channel *s_trackme_chan = NULL;
    // Any server address in this process.
    static std::string *s_trackme_addr = NULL;

    // Information of bugs.
    // Notice that this structure may be a combination of all affected bugs.
    // Namely `severity' is severity of the worst bug and `error_text' is
    // combination of descriptions of all bugs. Check tools/trackme_server
    // for impl. details.
    struct BugInfo {
        TrackMeSeverity severity;
        std::string error_text;

        bool operator==(const BugInfo &bug) const {
            return severity == bug.severity && error_text == bug.error_text;
        }
    };

    // If a bug was shown, its info was stored in this var as well so that we
    // can avoid showing the same bug repeatly.
    static BugInfo *g_bug_info = NULL;
    // The timestamp(microseconds) that we sent TrackMeRequest.
    static int64_t s_trackme_last_time = 0;

// version of RPC.
// Since the code for getting MELON_REVISION often fails,
// MELON_REVISION must be defined to string and be converted to number
// within our code.
// The code running before main() may see g_rpc_version=0, should be OK.
#if defined(MELON_REVISION)
    const int64_t g_rpc_version = atoll(MELON_REVISION);
#else
    const int64_t g_rpc_version = 0;
#endif

    int ReadJPaasHostPort(int container_port) {
        const uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);
        if (pw == NULL) {
            RPC_VLOG << "Fail to get password file entry of uid=" << uid;
            return -1;
        }
        char JPAAS_LOG_PATH[64];
        snprintf(JPAAS_LOG_PATH, sizeof(JPAAS_LOG_PATH),
                 "%s/jpaas_run/logs/env.log", pw->pw_dir);
        char *line = NULL;
        size_t line_len = 0;
        ssize_t nr = 0;
        mutil::ScopedFILE fp(fopen(JPAAS_LOG_PATH, "r"));
        if (!fp) {
            RPC_VLOG << "Fail to open `" << JPAAS_LOG_PATH << '\'';
            return -1;
        }
        int host_port = -1;
        char prefix[32];
        const int prefix_len =
                snprintf(prefix, sizeof(prefix), "JPAAS_HOST_PORT_%d=", container_port);
        while ((nr = getline(&line, &line_len, fp.get())) != -1) {
            if (line[nr - 1] == '\n') { // remove ending newline
                --nr;
            }
            if (nr > prefix_len && memcmp(line, prefix, prefix_len) == 0) {
                host_port = strtol(line + prefix_len, NULL, 10);
                break;
            }
        }
        free(line);
        RPC_VLOG_IF(host_port < 0) << "No entry starting with `" << prefix << "' found";
        return host_port;
    }

// Called in server.cpp
    void SetTrackMeAddress(mutil::EndPoint pt) {
        MELON_SCOPED_LOCK(s_trackme_mutex);
        if (s_trackme_addr == NULL) {
            // JPAAS has NAT capabilities, read its log to figure out the open port
            // accessible from outside.
            const int jpaas_port = ReadJPaasHostPort(pt.port);
            if (jpaas_port > 0) {
                RPC_VLOG << "Use jpaas_host_port=" << jpaas_port
                          << " instead of jpaas_container_port=" << pt.port;
                pt.port = jpaas_port;
            }
            s_trackme_addr = new std::string(mutil::endpoint2str(pt).c_str());
        }
    }

    static void HandleTrackMeResponse(Controller *cntl, TrackMeResponse *res) {
        if (cntl->Failed()) {
            RPC_VLOG << "Fail to access " << FLAGS_trackme_server << ", " << cntl->ErrorText();
        } else {
            BugInfo cur_info;
            cur_info.severity = res->severity();
            cur_info.error_text = res->error_text();
            bool already_reported = false;
            {
                MELON_SCOPED_LOCK(s_trackme_mutex);
                if (g_bug_info != NULL && *g_bug_info == cur_info) {
                    // we've shown the bug.
                    already_reported = true;
                } else {
                    // save the bug.
                    if (g_bug_info == NULL) {
                        g_bug_info = new BugInfo(cur_info);
                    } else {
                        *g_bug_info = cur_info;
                    }
                }
            }
            if (!already_reported) {
                switch (res->severity()) {
                    case TrackMeOK:
                        break;
                    case TrackMeFatal:
                        LOG(ERROR) << "Your melon (r" << g_rpc_version
                                    << ") is affected by: " << res->error_text();
                        break;
                    case TrackMeWarning:
                        LOG(WARNING) << "Your melon (r" << g_rpc_version
                                      << ") is affected by: " << res->error_text();
                        break;
                    default:
                        LOG(WARNING) << "Unknown severity=" << res->severity();
                        break;
                }
            }
            if (res->has_new_interval()) {
                // We can't fully trust the result from trackme_server which may
                // have bugs. Make sure the reporting interval is not too short or
                // too long
                int32_t new_interval = res->new_interval();
                new_interval = std::max(new_interval, TRACKME_MIN_INTERVAL);
                new_interval = std::min(new_interval, TRACKME_MAX_INTERVAL);
                if (new_interval != s_trackme_interval) {
                    s_trackme_interval = new_interval;
                    RPC_VLOG << "Update s_trackme_interval to " << new_interval;
                }
            }
        }
        delete cntl;
        delete res;
    }

    static void TrackMeNow(std::unique_lock<pthread_mutex_t> &mu) {
        if (s_trackme_addr == NULL) {
            return;
        }
        if (s_trackme_chan == NULL) {
            Channel *chan = new(std::nothrow) Channel;
            if (chan == NULL) {
                LOG(FATAL) << "Fail to new trackme channel";
                return;
            }
            ChannelOptions opt;
            // keep #connections on server-side low
            opt.connection_type = CONNECTION_TYPE_SHORT;
            if (chan->Init(FLAGS_trackme_server.c_str(), "c_murmurhash", &opt) != 0) {
                LOG(WARNING) << "Fail to connect to " << FLAGS_trackme_server;
                delete chan;
                return;
            }
            s_trackme_chan = chan;
        }
        mu.unlock();
        TrackMeService_Stub stub(s_trackme_chan);
        TrackMeRequest req;
        req.set_rpc_version(g_rpc_version);
        req.set_server_addr(*s_trackme_addr);
        TrackMeResponse *res = new TrackMeResponse;
        Controller *cntl = new Controller;
        cntl->set_request_code(policy::MurmurHash32(s_trackme_addr->data(), s_trackme_addr->size()));
        google::protobuf::Closure *done =
                ::melon::NewCallback(&HandleTrackMeResponse, cntl, res);
        stub.TrackMe(cntl, &req, res, done);
    }

// Called in global.cpp
// [Thread-safe] supposed to be called in low frequency.
    void TrackMe() {
        if (FLAGS_trackme_server.empty()) {
            return;
        }
        int64_t now = mutil::gettimeofday_us();
        std::unique_lock<pthread_mutex_t> mu(s_trackme_mutex);
        if (s_trackme_last_time == 0) {
            // Delay the first ping randomly within s_trackme_interval. This
            // protects trackme_server from ping storms.
            s_trackme_last_time =
                    now + mutil::fast_rand_less_than(s_trackme_interval) * 1000000L;
        }
        if (now > s_trackme_last_time + 1000000L * s_trackme_interval) {
            s_trackme_last_time = now;
            return TrackMeNow(mu);
        }
    }

} // namespace melon
