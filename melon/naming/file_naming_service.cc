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



#include <stdio.h>                                      // getline
#include <string>                                       // std::string
#include <set>                                          // std::set
#include <melon/utility/files/file_watcher.h>                    // FileWatcher
#include <melon/utility/files/scoped_file.h>                     // ScopedFILE
#include <melon/fiber/fiber.h>                            // fiber_usleep
#include <melon/rpc/log.h>
#include <melon/naming/file_naming_service.h>


namespace melon::naming {

    bool SplitIntoServerAndTag(const mutil::StringPiece &line,
                               mutil::StringPiece *server_addr,
                               mutil::StringPiece *tag) {
        size_t i = 0;
        for (; i < line.size() && isspace(line[i]); ++i) {}
        if (i == line.size() || line[i] == '#') {  // blank line or comments
            return false;
        }
        const char *const addr_start = line.data() + i;
        const char *tag_start = NULL;
        ssize_t tag_size = 0;
        for (; i < line.size() && !isspace(line[i]); ++i) {}
        if (server_addr) {
            server_addr->set(addr_start, line.data() + i - addr_start);
        }
        if (i != line.size()) {
            for (++i; i < line.size() && isspace(line[i]); ++i) {}
            if (i < line.size()) {
                tag_start = line.data() + i;
                tag_size = 1;
                // find start of comments.
                for (++i; i < line.size() && line[i] != '#'; ++i, ++tag_size) {}
                // trim ending blanks
                for (; tag_size > 0 && isspace(tag_start[tag_size - 1]);
                       --tag_size) {}
            }
            if (tag) {
                if (tag_size) {
                    tag->set(tag_start, tag_size);
                } else {
                    tag->clear();
                }
            }
        }
        return true;
    }

    int FileNamingService::GetServers(const char *service_name,
                                      std::vector<ServerNode> *servers) {
        servers->clear();
        char *line = NULL;
        size_t line_len = 0;
        ssize_t nr = 0;
        // Sort/unique the inserted vector is faster, but may have a different order
        // of addresses from the file. To make assertions in tests easier, we use
        // set to de-duplicate and keep the order.
        std::set<ServerNode> presence;

        mutil::ScopedFILE fp(fopen(service_name, "r"));
        if (!fp) {
            PLOG(ERROR) << "Fail to open `" << service_name << "'";
            return errno;
        }
        while ((nr = getline(&line, &line_len, fp.get())) != -1) {
            if (line[nr - 1] == '\n') { // remove ending newline
                --nr;
            }
            mutil::StringPiece addr;
            mutil::StringPiece tag;
            if (!SplitIntoServerAndTag(mutil::StringPiece(line, nr),
                                       &addr, &tag)) {
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
                 << (servers->size() > 1 ? " servers" : " server");
        free(line);
        return 0;
    }

    int FileNamingService::RunNamingService(const char *service_name,
                                            NamingServiceActions *actions) {
        std::vector<ServerNode> servers;
        mutil::FileWatcher fw;
        if (fw.init(service_name) < 0) {
            LOG(ERROR) << "Fail to init FileWatcher on `" << service_name << "'";
            return -1;
        }
        for (;;) {
            const int rc = GetServers(service_name, &servers);
            if (rc != 0) {
                return rc;
            }
            actions->ResetServers(servers);

            for (;;) {
                mutil::FileWatcher::Change change = fw.check_and_consume();
                if (change > 0) {
                    break;
                }
                if (change < 0) {
                    LOG(ERROR) << "`" << service_name << "' was deleted";
                }
                if (fiber_usleep(100000L/*100ms*/) < 0) {
                    if (errno == ESTOP) {
                        return 0;
                    }
                    PLOG(ERROR) << "Fail to sleep";
                    return -1;
                }
            }
        }
        CHECK(false);
        return -1;
    }

    void FileNamingService::Describe(std::ostream &os,
                                     const DescribeOptions &) const {
        os << "file";
        return;
    }

    NamingService *FileNamingService::New() const {
        return new FileNamingService;
    }

    void FileNamingService::Destroy() {
        delete this;
    }

} // namespace melon::naming
