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
#include <vector>

#include "abel/strings/str_split.h"
#include "abel/container/flat_hash_map.h"
#include "abel/log/logging.h"

namespace abel {

    class http_uri_builder;

    class http_uri {
    public:
        typedef abel::flat_hash_map<std::string_view, std::string_view> query_map;
    public:
        http_uri() = default;

        // Accessors.
        std::string_view scheme() const noexcept { return get_component(kScheme); }

        std::string_view userinfo() const noexcept { return get_component(kUserInfo); }

        std::string_view host() const noexcept { return get_component(kHost); }

        std::uint16_t port() const noexcept { return _port; }

        std::string_view path() const noexcept { return get_component(kPath); }

        std::string_view query() const noexcept { return get_component(kQuery); }

        // return empty string_view means not exists
        std::string_view get_query(std::string_view key) const noexcept {
            auto it = _query_map.find(key);
            if (it != _query_map.end()) {
                return it->second;
            }
            return "";
        }

        std::string_view fragment() const noexcept { return get_component(kFragment); }

        // Convert this object to string.
        std::string to_string() const { return _uri; }

    private:
        friend std::optional<http_uri> parse_uri(const std::string_view &s);
        friend class http_uri_builder;

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

        http_uri(std::string uri, uri_components comps, std::uint16_t port)
                : _uri(std::move(uri)), _comps(comps), _port(port) {
            auto q = query();
            if (q.empty()) {
                return;
            }
            std::vector<std::string_view> kvs = abel::string_split(q, abel::by_char('&'));
            for (size_t i = 0; i < kvs.size(); i++) {
                std::vector<std::string_view> kv = abel::string_split(kvs[i], abel::by_char('='));
                DCHECK(kv.size() == 2);
                _query_map[kv[0]] = kv[1];
            }
        }

        std::string_view get_component(uri_component comp) const noexcept {
            DCHECK_NE(comp, kPort);
            DCHECK_NE(comp, kComponentCount);
            return std::string_view(_uri).substr(_comps[comp].first,
                                                 _comps[comp].second);
        }

    private:
        std::string _uri;
        uri_components _comps;  // Into `_uri`.
        std::uint16_t _port;
        query_map _query_map;
    };


    std::optional<http_uri> parse_uri(const std::string_view &s);

    class http_uri_builder {
    public:
        typedef abel::flat_hash_map<std::string, std::string> query_map;
    public:
        http_uri_builder() = default;
        explicit http_uri_builder(std::string_view uri) {
            set_http_url(uri);
        }

        bool set_http_url(std::string_view uri) {
            _old_uri = parse_uri(uri);
            if(!_old_uri) {
                return false;
            }
            for(auto it = _old_uri->_query_map.begin(); it != _old_uri->_query_map.end(); ++it) {
                _query_map[it->first] = it->second;
            }
            _scheme = _old_uri->scheme();
            _userinfo = _old_uri->userinfo();
            _host = _old_uri->host();
            _port = _old_uri->port();
            _path = _old_uri->path();
            _query = _old_uri->query();
            _fragment = _old_uri->fragment();
            return true;
        }

        void set_scheme(std::string_view scheme) {
            _scheme = scheme;
        }

        std::string_view get_scheme() const {
            return _scheme;
        }

        void set_user_info(std::string_view uif) {
            _userinfo = uif;
        }

        std::string_view get_user_info() const {
            return _userinfo;
        }

        void set_host(std::string_view host) {
            _host = host;
        }
        std::string_view get_host() const {
            return _host;
        }

        void set_port(uint16_t p) {
            _port = p;
        }

        uint16_t get_port() const {
            return _port;
        }

        void set_fragment(std::string_view f) {
            _fragment = f;
        }

        std::string_view get_fragment() const {
            return _fragment;
        }

        void add_query(std::string_view key, std::string_view value) {
            DCHECK(!key.empty());
            DCHECK(!value.empty());
            _query_map[key] = value;
        }

        void remove_query(std::string_view key) {
            DCHECK(!key.empty());
            _query_map.erase(key);
        }

        std::string to_string(bool with_user_info = false) const;

        std::optional<http_uri> build(bool with_user_info = false) const;

        std::optional<http_uri> parsed_uri() const {
            return _old_uri;
        }

        void set_host_and_port(std::string_view hp);

        void clear() {
           *this = http_uri_builder();
        }

    private:
        std::optional<http_uri> _old_uri{std::nullopt};
        query_map _query_map;
        std::string  _scheme;
        std::string _userinfo;
        std::string _host;
        uint16_t    _port{0};
        std::string _path;
        std::string _query;
        std::string _fragment;

    };
}  // namespace abel

#endif  // ABEL_NET_URI_H_
