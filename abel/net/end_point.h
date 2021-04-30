//
// Created by liyinbin on 2021/4/5.
//


#ifndef ABEL_NET_ENDPOINT_H_
#define ABEL_NET_ENDPOINT_H_

#include <netinet/in.h>
#include <iostream>
#include "abel/hash/hash.h"

namespace abel {

    typedef struct in_addr ip_t;

    static const ip_t IP_ANY = {INADDR_ANY};
    static const ip_t IP_NONE = {INADDR_NONE};

    inline in_addr_t ip2int(ip_t ip) { return ip.s_addr; }

    inline ip_t int2ip(in_addr_t ip_value) {
        const ip_t ip = {ip_value};
        return ip;
    }

    class end_point;
    class end_point_builder {
    public:

        sockaddr*  addr();
        socklen_t* length();

        end_point build();

    private:
        sockaddr_storage _storage;
        socklen_t _length = sizeof(sockaddr_storage);
    };

    inline constexpr struct from_ipv4_t {
        constexpr explicit from_ipv4_t() = default;
    } from_ipv4;
    inline constexpr struct from_ipv6_t {
        constexpr explicit from_ipv6_t() = default;
    } from_ipv6;

    class alignas(alignof(sockaddr_storage)) end_point {

    public:
        end_point() : _length(0) {}
        ~end_point();

        end_point(const end_point& ep);
        end_point& operator=(const end_point& ep);
        end_point(end_point&& ep) noexcept;
        end_point& operator=(end_point&& ep) noexcept;

        bool empty() const noexcept { return _length == 0; }

        const sockaddr* get() const noexcept {
            if (ABEL_LIKELY(is_trivially_copyable())) {
                return reinterpret_cast<const sockaddr*>(&_storage);
            }
            return slow_get();
        }

        template <class T>
        const T* unsafe_get() const noexcept {
            return reinterpret_cast<const T*>(get());
        }

        socklen_t length() const noexcept { return _length; }

        sa_family_t family() const noexcept { return get()->sa_family; }

        std::string to_string() const;

        std::string get_ip() const;

        std::uint16_t get_port() const;

        template<typename H>
        friend H qrpc_hash_value(H h, end_point t) {
            return H::combine(std::move(h), ip2int(t.ip), t.port);
        }

        ip_t ip;
        int port;
    public:

        static end_point from_ipv4(const std::string& ip, std::uint16_t port);
        static end_point from_ipv6(const std::string& ip, std::uint16_t port);
        static std::optional<end_point> from_ipv6(const std::string_view &src);
        static std::optional<end_point> from_ipv4(const std::string_view &src);

        static end_point from_string(const std::string_view &src);

        // Get the other end of a socket connection
        static std::optional<end_point> get_remote_side(int fd);

        // Get the local end of a socket connection
        static std::optional<end_point> get_local_side(int fd);

    private:

        friend class end_point_builder;
        end_point(const sockaddr* addr, socklen_t len);

        bool is_trivially_copyable() const { return _length <= kOptimizedSize; }


        void slow_destroy();
        void slow_copy_uninitialized(const end_point& ep);
        void slow_copy(const end_point& ep);
        const sockaddr* slow_get() const;

    private:
        static constexpr auto kOptimizedSize = sizeof(sockaddr_in6);
        using Storage = std::aligned_storage_t<kOptimizedSize, 1>;
        //must be first;
        Storage _storage;
        socklen_t _length;
    };

    std::ostream& operator<<(std::ostream& os, const end_point& endpoint);
    bool operator==(const end_point& left, const end_point& right);

    inline end_point::~end_point() {
        if (ABEL_LIKELY(is_trivially_copyable())) {
            return;  // Nothing to do.
        }
        slow_destroy();
    }

    inline end_point::end_point(const end_point& ep) {
        if (ABEL_LIKELY(ep.is_trivially_copyable())) {
            memcpy(reinterpret_cast<void*>(this), reinterpret_cast<const void*>(&ep),
                   sizeof(end_point));
        } else {
            slow_copy_uninitialized(ep);
        }
    }

    inline end_point& end_point::operator=(const end_point& ep) {
        if (ABEL_LIKELY(is_trivially_copyable() && ep.is_trivially_copyable())) {
            memcpy(reinterpret_cast<void*>(this), reinterpret_cast<const void*>(&ep),
                   sizeof(end_point));
        } else {
            slow_copy(ep);
        }
        return *this;
    }

    inline end_point::end_point(end_point&& ep) noexcept {
        if (ABEL_LIKELY(ep.is_trivially_copyable())) {
            memcpy(reinterpret_cast<void*>(this), reinterpret_cast<const void*>(&ep),
                   sizeof(end_point));
        } else {
            slow_copy_uninitialized(ep);
        }
    }

    inline end_point& end_point::operator=(end_point&& ep) noexcept {
        if (ABEL_LIKELY(is_trivially_copyable() && ep.is_trivially_copyable())) {
            memcpy(reinterpret_cast<void*>(this), reinterpret_cast<const void*>(&ep),
                   sizeof(end_point));
        } else {
            slow_copy(ep);
        }
        return *this;
    }

}  // namespace abel






#endif  // ABEL_NET_ENDPOINT_H_
