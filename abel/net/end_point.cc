//
// Created by liyinbin on 2021/4/18.
//

#include "abel/net/end_point.h"


#include <arpa/inet.h>                         // inet_pton, inet_ntop
#include <netdb.h>                             // gethostbyname_r
#include <unistd.h>                            // gethostname
#include <errno.h>                             // errno
#include <string.h>                            // strcpy
#include <stdio.h>                             // snprintf
#include <stdlib.h>                            // strtol
#include "abel/log/logging.h"
#include <string_view>
#include <sys/socket.h>                        // SO_REUSEADDR SO_REUSEPORT
#include "abel/base/profile.h"
#include "abel/memory/leaky_singleton.h"
#include "abel/strings/numbers.h"


namespace abel {

    std::optional<end_point> end_point::get_local_side(int fd) {
        end_point_builder eb;
        const int rc = getsockname(fd, eb.addr(), eb.length());
        if (rc != 0) {
            return {};
        }
        return eb.build();
    }

    std::optional<end_point> end_point::get_remote_side(int fd) {
        end_point_builder eb;
        const int rc = getpeername(fd, eb.addr(), eb.length());
        if (rc != 0) {
            return {};
        }
        return eb.build();
    }

    ////NEW

    sockaddr *end_point_builder::addr() {
        return reinterpret_cast<sockaddr *>(&_storage);
    }

    socklen_t *end_point_builder::length() { return &_length; }

    end_point end_point_builder::build() {
        return end_point(addr(), _length);
    }

#define INTERNAL_PTR() (*reinterpret_cast<sockaddr_storage**>(&_storage))
#define INTERNAL_CPTR() (*reinterpret_cast<sockaddr_storage* const*>(&_storage))

    end_point::end_point(const sockaddr *addr, socklen_t len) : _length(len) {
        if (_length <= kOptimizedSize) {
            memcpy(&_storage, addr, _length);
        } else {
            INTERNAL_PTR() = new sockaddr_storage;
            memcpy(INTERNAL_PTR(), addr, _length);
        }
    }

    void end_point::slow_destroy() { delete INTERNAL_PTR(); }

    void end_point::slow_copy_uninitialized(const end_point &ep) {
        _length = ep.length();
        INTERNAL_PTR() = new sockaddr_storage;
        memcpy(INTERNAL_PTR(), ep.get(), _length);
    }

    void end_point::slow_copy(const end_point &ep) {
        if (!is_trivially_copyable()) {
            slow_destroy();
        }
        if (ep.is_trivially_copyable()) {
            _length = ep._length;
            memcpy(&_storage, &ep._storage, _length);
        } else {
            _length = ep._length;
            INTERNAL_PTR() = new sockaddr_storage;
            memcpy(INTERNAL_PTR(), ep.get(), _length);
        }
    }

    const sockaddr *end_point::slow_get() const {
        return reinterpret_cast<const sockaddr *>(INTERNAL_CPTR());
    }


    namespace {

        std::string sock_addr_to_string(const sockaddr *addr) {
            static constexpr auto kPortChars = 6;  // ":12345"
            auto af = addr->sa_family;
            switch (af) {
                case AF_INET: {
                    std::string result;
                    auto p = reinterpret_cast<const sockaddr_in *>(addr);
                    result.resize(INET_ADDRSTRLEN + kPortChars + 1 /* '\x00' */);
                    DCHECK(inet_ntop(af, &p->sin_addr, result.data(), result.size()));
                    auto ptr = result.data() + result.find('\x00');
                    *ptr++ = ':';
                    snprintf(ptr, kPortChars, "%d", ntohs(p->sin_port));
                    result.resize(result.find('\x00'));
                    return result;
                }
                case AF_INET6: {
                    std::string result;
                    auto p = reinterpret_cast<const sockaddr_in6 *>(addr);
                    result.resize(INET6_ADDRSTRLEN + 2 /* "[]" */ + kPortChars +
                                  1 /* '\x00' */);
                    result[0] = '[';
                    DCHECK(
                            inet_ntop(af, &p->sin6_addr, result.data() + 1, result.size()));
                    auto ptr = result.data() + result.find('\x00');
                    *ptr++ = ']';
                    *ptr++ = ':';
                    snprintf(ptr, kPortChars, "%d", ntohs(p->sin6_port));
                    result.resize(result.find('\x00'));
                    return result;
                }
                    /*
                    case AF_UNIX: {
                        auto p = reinterpret_cast<const sockaddr_un*>(addr);
                        if (p->sun_path[0] == '\0' && p->sun_path[1] != '\0') {
                            return "@"s + &p->sun_path[1];
                        } else {
                            return p->sun_path;
                        }
                    }
                     */
                default: {
                    return abel::format("TODO: Endpoint::ToString() for AF #{}.", af);
                }
            }
        }

    }  // namespace

    std::string end_point::to_string() const {
        if (ABEL_UNLIKELY(empty())) {
            return "(null)";
        }
        return sock_addr_to_string(get());
    }

    std::ostream &operator<<(std::ostream &os, const end_point &endpoint) {
        return os << endpoint.to_string();
    }

    bool operator==(const end_point &left, const end_point &right) {
        return memcmp(left.get(), right.get(), left.length()) == 0;
    }

    end_point end_point::from_ipv4(const std::string &ip, std::uint16_t port) {
        end_point_builder er;
        auto addr = er.addr();
        auto p = reinterpret_cast<sockaddr_in *>(addr);
        memset(p, 0, sizeof(sockaddr_in));
        DCHECK_MSG(inet_pton(AF_INET, ip.c_str(), &p->sin_addr),
                   "Cannot parse [{}].", ip);
        p->sin_port = htons(port);
        p->sin_family = AF_INET;
        *er.length() = sizeof(sockaddr_in);
        return er.build();
    }

    end_point end_point::from_ipv6(const std::string &ip, std::uint16_t port) {
        end_point_builder er;
        auto addr = er.addr();
        auto p = reinterpret_cast<sockaddr_in6 *>(addr);
        memset(p, 0, sizeof(sockaddr_in6));
        DCHECK_MSG(inet_pton(AF_INET6, ip.c_str(), &p->sin6_addr),
                   "Cannot parse [{}].", ip);
        p->sin6_port = htons(port);
        p->sin6_family = AF_INET6;
        *er.length() = sizeof(sockaddr_in6);
        return er.build();
    }

    std::string end_point::get_ip() const {
        std::string result;
        if (family() == AF_INET) {
            result.resize(INET_ADDRSTRLEN + 1 /* '\x00' */);
            DCHECK(inet_ntop(family(),
                             &unsafe_get<sockaddr_in>()->sin_addr,
                             result.data(), result.size()));
        } else if (family() == AF_INET6) {
            result.resize(INET6_ADDRSTRLEN + 1 /* '\x00' */);
            DCHECK(inet_ntop(family(),
                             &unsafe_get<sockaddr_in6>()->sin6_addr,
                             result.data(), result.size()));
        } else {
            DCHECK_MSG(
                    0, "Unexpected: Address family #{} is not a valid IP address family.",
                    family());
        }
        return result.substr(0, result.find('\x00'));
    }

    std::uint16_t end_point::get_port() const {
        DCHECK_MSG(family() == AF_INET || family() == AF_INET6,
                   "Unexpected: Address family #{} is not a valid IP address family.",
                   family());
        if (family() == AF_INET) {
            return ntohs(
                    reinterpret_cast<const sockaddr_in *>(get())->sin_port);
        } else if (family() == AF_INET6) {
            return ntohs(
                    reinterpret_cast<const sockaddr_in6 *>(get())->sin6_port);
        }
        return 0;
    }

    std::optional<end_point> end_point::from_ipv6(const std::string_view &src) {
        auto pos = src.find_last_of(':');
        if (pos == std::string_view::npos) {
            return {};
        }
        if (pos < 2) {
            return {};
        }
        auto ip = std::string(src.substr(1, pos - 2));
        uint32_t port;
        auto r = abel::simple_atoi(src.substr(pos + 1), &port);
        if (!r) {
            return {};
        }
        end_point_builder er;
        auto addr = er.addr();
        auto p = reinterpret_cast<sockaddr_in6 *>(addr);
        memset(p, 0, sizeof(sockaddr_in6));
        if (inet_pton(AF_INET6, ip.c_str(), &p->sin6_addr) != 1) {
            return {};
        }
        p->sin6_port = htons(port);
        p->sin6_family = AF_INET6;
        *er.length() = sizeof(sockaddr_in6);
        return er.build();
    }

    std::optional<end_point> end_point::from_ipv4(const std::string_view &src) {
        auto pos = src.find(':');
        if (pos == std::string_view::npos) {
            return {};
        }
        auto ip = std::string(src.substr(0, pos));
        uint32_t port;
        auto r = abel::simple_atoi(src.substr(pos + 1), &port);
        if (!r) {
            return {};
        }
        end_point_builder er;
        auto addr = er.addr();
        auto p = reinterpret_cast<sockaddr_in *>(addr);
        memset(p, 0, sizeof(sockaddr_in));
        if (inet_pton(AF_INET, ip.c_str(), &p->sin_addr) != 1) {
            return {};
        }
        p->sin_port = htons(port);
        p->sin_family = AF_INET;
        *er.length() = sizeof(sockaddr_in);
        return er.build();
    }

    end_point end_point::from_string(const std::string_view &src) {
        if (auto opt4 = from_ipv4(src)) {
            return *opt4;
        } else {
            auto opt6 = from_ipv6(src);
            DCHECK(opt6);
            return *opt6;
        }
    }
}  // namespace abel
