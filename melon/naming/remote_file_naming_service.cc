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



#include <turbo/flags/flag.h>
#include <cstdio>                                      // getline
#include <string>                                       // std::string
#include <set>                                          // std::set
#include <melon/fiber/fiber.h>                            // fiber_usleep
#include <melon/utility/iobuf.h>
#include <melon/rpc/log.h>
#include <melon/rpc/channel.h>
#include <melon/naming/remote_file_naming_service.h>

TURBO_FLAG(int, remote_file_connect_timeout_ms, -1,
           "Timeout for creating connections to fetch remote server lists,"
           " set to remote_file_timeout_ms/3 by default (-1)");
TURBO_FLAG(int, remote_file_timeout_ms, 1000,
           "Timeout for fetching remote server lists");

namespace melon::naming {

    // Defined in file_naming_service.cpp
    bool SplitIntoServerAndTag(const mutil::StringPiece &line,
                               mutil::StringPiece *server_addr,
                               mutil::StringPiece *tag);

    static bool CutLineFromIOBuf(mutil::IOBuf *source, std::string *line_out) {
        if (source->empty()) {
            return false;
        }
        mutil::IOBuf line_data;
        if (source->cut_until(&line_data, "\n") != 0) {
            source->cutn(line_out, source->size());
            return true;
        }
        line_data.copy_to(line_out);
        if (!line_out->empty() && mutil::back_char(*line_out) == '\r') {
            line_out->resize(line_out->size() - 1);
        }
        return true;
    }

    int RemoteFileNamingService::GetServers(const char *service_name_cstr,
                                            std::vector<ServerNode> *servers) {
        servers->clear();

        if (_channel == nullptr) {
            mutil::StringPiece tmpname(service_name_cstr);
            size_t pos = tmpname.find("://");
            mutil::StringPiece proto;
            if (pos != mutil::StringPiece::npos) {
                proto = tmpname.substr(0, pos);
                for (pos += 3; tmpname[pos] == '/'; ++pos) {}
                tmpname.remove_prefix(pos);
            } else {
                proto = "http";
            }
            if (proto != "bns" && proto != "http") {
                LOG(ERROR) << "Invalid protocol=`" << proto
                           << "\' in service_name=" << service_name_cstr;
                return -1;
            }
            size_t slash_pos = tmpname.find('/');
            mutil::StringPiece server_addr_piece;
            if (slash_pos == mutil::StringPiece::npos) {
                server_addr_piece = tmpname;
                _path = "/";
            } else {
                server_addr_piece = tmpname.substr(0, slash_pos);
                _path = tmpname.substr(slash_pos).as_string();
            }
            _server_addr.reserve(proto.size() + 3 + server_addr_piece.size());
            _server_addr.append(proto.data(), proto.size());
            _server_addr.append("://");
            _server_addr.append(server_addr_piece.data(), server_addr_piece.size());
            ChannelOptions opt;
            opt.protocol = PROTOCOL_HTTP;
            opt.connect_timeout_ms = turbo::get_flag(FLAGS_remote_file_connect_timeout_ms) > 0 ?
                                     turbo::get_flag(FLAGS_remote_file_connect_timeout_ms) :
                                     turbo::get_flag(FLAGS_remote_file_timeout_ms) / 3;
            opt.timeout_ms = turbo::get_flag(FLAGS_remote_file_timeout_ms);
            std::unique_ptr<Channel> chan(new Channel);
            if (chan->Init(_server_addr.c_str(), "rr", &opt) != 0) {
                LOG(ERROR) << "Fail to init channel to " << _server_addr;
                return -1;
            }
            _channel.swap(chan);
        }

        Controller cntl;
        cntl.http_request().uri() = _path;
        _channel->CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        if (cntl.Failed()) {
            LOG(WARNING) << "Fail to access " << _server_addr << _path << ": "
                         << cntl.ErrorText();
            return -1;
        }
        std::string line;
        // Sort/unique the inserted vector is faster, but may have a different order
        // of addresses from the file. To make assertions in tests easier, we use
        // set to de-duplicate and keep the order.
        std::set<ServerNode> presence;

        while (CutLineFromIOBuf(&cntl.response_attachment(), &line)) {
            mutil::StringPiece addr;
            mutil::StringPiece tag;
            if (!SplitIntoServerAndTag(line, &addr, &tag)) {
                continue;
            }
            const_cast<char *>(addr.data())[addr.size()] = '\0'; // safe
            mutil::EndPoint point;
            if (str2endpoint(addr.data(), &point) != 0 &&
                hostname2endpoint(addr.data(), &point) != 0) {
                LOG(ERROR) << "Invalid address=`" << addr << '\'';
                continue;
            }
            ServerNode node;
            node.addr = point;
            tag.CopyToString(&node.tag);
            if (presence.insert(node).second) {
                servers->push_back(node);
            } else {
                RPC_VLOG << "Duplicated server=" << node;
            }
        }
        RPC_VLOG << "Got " << servers->size()
                 << (servers->size() > 1 ? " servers" : " server")
                 << " from " << service_name_cstr;
        return 0;
    }

    void RemoteFileNamingService::Describe(std::ostream &os,
                                           const DescribeOptions &) const {
        os << "remotefile";
        return;
    }

    NamingService *RemoteFileNamingService::New() const {
        return new RemoteFileNamingService;
    }

    void RemoteFileNamingService::Destroy() {
        delete this;
    }

} // namespace melon::naming
