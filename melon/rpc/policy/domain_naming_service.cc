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


#include <netdb.h>                                    // gethostbyname_r
#include <cstdlib>                                   // strtol
#include <string>                                     // std::string
#include "melon/fiber/internal/fiber.h"
#include "melon/rpc/log.h"
#include "melon/rpc/policy/domain_naming_service.h"
#include "melon/base/profile.h"


namespace melon::rpc {
    namespace policy {

        DomainNamingService::DomainNamingService(int default_port)
                : _aux_buf_len(0), _default_port(default_port) {}

        int DomainNamingService::GetServers(const char *dns_name,
                                            std::vector<ServerNode> *servers) {
            servers->clear();
            if (!dns_name) {
                MELON_LOG(ERROR) << "dns_name is nullptr";
                return -1;
            }

            // Should be enough to hold host name
            char buf[128];
            size_t i = 0;
            for (; i < sizeof(buf) - 1 && dns_name[i] != '\0'
                   && dns_name[i] != ':' && dns_name[i] != '/'; ++i) {
                buf[i] = dns_name[i];
            }
            if (i == sizeof(buf) - 1) {
                MELON_LOG(ERROR) << "dns_name=`" << dns_name << "' is too long";
                return -1;
            }

            buf[i] = '\0';
            int port = _default_port;
            if (dns_name[i] == ':') {
                ++i;
                char *end = nullptr;
                port = strtol(dns_name + i, &end, 10);
                if (end == dns_name + i) {
                    MELON_LOG(ERROR) << "No port after colon in `" << dns_name << '\'';
                    return -1;
                } else if (*end != '\0') {
                    if (*end != '/') {
                        MELON_LOG(ERROR) << "Invalid content=`" << end << "' after port="
                                         << port << " in `" << dns_name << '\'';
                        return -1;
                    }
                    // Drop path and other stuff.
                    RPC_VLOG << "Drop content=`" << end << "' after port=" << port
                             << " in `" << dns_name << '\'';
                    // NOTE: Don't ever change *end which is const.
                }
            }
            if (port < 0 || port > 65535) {
                MELON_LOG(ERROR) << "Invalid port=" << port << " in `" << dns_name << '\'';
                return -1;
            }

#if defined(MELON_PLATFORM_OSX)
            _aux_buf_len = 0; // suppress unused warning
            // gethostbyname on MAC is thread-safe (with current usage) since the
            // returned hostent is TLS. Check following link for the ref:
            // https://lists.apple.com/archives/darwin-dev/2006/May/msg00008.html
            struct hostent *result = gethostbyname(buf);
            if (result == nullptr) {
                MELON_LOG(WARNING) << "result of gethostbyname is nullptr";
                return -1;
            }
#else
            if (_aux_buf == nullptr) {
                _aux_buf_len = 1024;
                _aux_buf.reset(new char[_aux_buf_len]);
            }
            int ret = 0;
            int error = 0;
            struct hostent ent;
            struct hostent* result = nullptr;
            do {
                result = nullptr;
                error = 0;
                ret = gethostbyname_r(buf, &ent, _aux_buf.get(), _aux_buf_len,
                                      &result, &error);
                if (ret != ERANGE) { // _aux_buf is not long enough
                    break;
                }
                _aux_buf_len *= 2;
                _aux_buf.reset(new char[_aux_buf_len]);
                RPC_VLOG << "Resized _aux_buf to " << _aux_buf_len
                         << ", dns_name=" << dns_name;
            } while (1);
            if (ret != 0) {
                // `hstrerror' is thread safe under linux
                MELON_LOG(WARNING) << "Can't resolve `" << buf << "', return=`" << melon_error(ret)
                             << "' herror=`" << hstrerror(error) << '\'';
                return -1;
            }
            if (result == nullptr) {
                MELON_LOG(WARNING) << "result of gethostbyname_r is nullptr";
                return -1;
            }
#endif

            melon::base::end_point point;
            point.port = port;
            for (int i = 0; result->h_addr_list[i] != nullptr; ++i) {
                if (result->h_addrtype == AF_INET) {
                    // Only fetch IPv4 addresses
                    bcopy(result->h_addr_list[i], &point.ip, result->h_length);
                    servers->push_back(ServerNode(point, std::string()));
                } else {
                    MELON_LOG(WARNING) << "Found address of unsupported protocol="
                                       << result->h_addrtype;
                }
            }
            return 0;
        }

        void DomainNamingService::Describe(
                std::ostream &os, const DescribeOptions &) const {
            os << "http";
            return;
        }

        NamingService *DomainNamingService::New() const {
            return new DomainNamingService(_default_port);
        }

        void DomainNamingService::Destroy() {
            delete this;
        }

    }  // namespace policy
} // namespace melon::rpc
