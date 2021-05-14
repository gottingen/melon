//
// Created by liyinbin on 2021/5/1.
//

#ifndef ABEL_NET_URI_H_
#define ABEL_NET_URI_H_

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "abel/log/logging.h"

namespace abel {


    class uri {
    public:
        uri() = default;

        // If `from` is malformed, the program crashes.
        //
        // To parse URI from untrusted source, use `pa<uri>(...)` instead.
        explicit uri(const std::string_view& from);

        // Accessors.
        std::string_view scheme() const noexcept { return GetComponent(kScheme); }
        std::string_view userinfo() const noexcept { return GetComponent(kUserInfo); }
        std::string_view host() const noexcept { return GetComponent(kHost); }
        std::uint16_t port() const noexcept { return port_; }
        std::string_view path() const noexcept { return GetComponent(kPath); }
        std::string_view query() const noexcept { return GetComponent(kQuery); }
        std::string_view fragment() const noexcept { return GetComponent(kFragment); }

        // Convert this object to string.
        std::string to_string() const { return uri_; }

    private:
        friend  std::optional<uri> parse_uri(const std::string_view& s);
        // Not declared as `enum class` intentionally. We use enumerators below as
        // indices.
        enum uri_component {
            kScheme = 0,
            kUserInfo = 1,
            kHost = 2,
            kPort = 3,
            kPath = 4,
            kQuery = 5,
            kFragment = 6,
            kComponentCount = 7
        };

        // Using `std::uint16_t` saves memory. I don't expect a URI longer than 64K.
        using uri_component_view = std::pair<std::uint16_t, std::uint16_t>;
        using uri_components = std::array<uri_component_view, kComponentCount>;

        uri(std::string uri, uri_components comps, std::uint16_t port)
                : uri_(std::move(uri)), comps_(comps), port_(port) {}

        std::string_view GetComponent(uri_component comp) const noexcept {
            DCHECK_NE(comp, kPort);
            DCHECK_NE(comp, kComponentCount);
            return std::string_view(uri_).substr(comps_[comp].first,
                                                 comps_[comp].second);
        }

    private:
        std::string uri_;
        uri_components comps_;  // Into `uri_`.
        std::uint16_t port_;
    };


    std::optional<uri> parse_uri(const std::string_view& s);


}  // namespace abel

#endif  // ABEL_NET_URI_H_
