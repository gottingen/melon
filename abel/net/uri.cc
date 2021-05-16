//
// Created by liyinbin on 2021/5/1.
//

#include "abel/net/uri.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "abel/strings/byte_set.h"
#include "abel/strings/numbers.h"

namespace abel {

    struct uri_parser_byte_sets {
        const abel::byte_set &alpha;
        const abel::byte_set &upper;
        const abel::byte_set &lower;
        const abel::byte_set &digit;
        const abel::byte_set &alphanum;
        const abel::byte_set &print;
        const abel::byte_set &hex;

        const abel::byte_set gen_delims;
        const abel::byte_set sub_delims;
        const abel::byte_set reserved;
        const abel::byte_set mark;
        const abel::byte_set unreserved;
        const abel::byte_set userinfo;
        const abel::byte_set uric_no_slash;
        const abel::byte_set rel_segment;
        const abel::byte_set scheme;
        const abel::byte_set reg_name;
        const abel::byte_set expansion;
        const abel::byte_set pchar;
        const abel::byte_set query;
        const abel::byte_set fragment;

    private:
        uri_parser_byte_sets()
                : alpha(abel::byte_set::alphas()),
                  upper(abel::byte_set::uppercase()),
                  lower(abel::byte_set::lowercase()),
                  digit(abel::byte_set::digits()),
                  alphanum(abel::byte_set::alpha_nums()),
                  print(abel::byte_set::printables()),
                  hex(abel::byte_set::hex()),
                  gen_delims(":/?#[]@"),
                  sub_delims("!$&'()*+,;="),
                  reserved(gen_delims | sub_delims),
                  mark("-_.!~*'()"),
                  unreserved(alphanum | "-._~"),
                  userinfo(unreserved | ",:&=+$,"),
                  uric_no_slash(unreserved | ",?:@&=+$,"),
                  rel_segment(unreserved | ",@&=+$,"),
                  scheme(abel::byte_set::alpha_nums() | "+-."),
                  reg_name(unreserved | sub_delims),
                // `expansion` was used to hold non-conformant chars (per RFC3986) that
                // was allowed in `common/uri`. Here we strip that behavior by default.
                  expansion("|{}[]^\""),
                  pchar(unreserved | sub_delims | ":@" | expansion),
                  query(pchar | "/?"),
                // We allowed `#` in `common/uri`, this is not conformant per RFC3986,
                // so we strip that behavior here.
                  fragment(pchar | "/?" | "#") {}

    public:
        static const uri_parser_byte_sets &instance() {
            static uri_parser_byte_sets sinstance;
            return sinstance;
        }
    };

    struct uri_parse_result_receiver {
        explicit uri_parse_result_receiver(const char *base) : base_(base) {}

        void SetScheme(const char *value, size_t length) {
            scheme = {value - base_, length};
        }

        void SetUserInfo(const char *value, size_t length) {
            userinfo = {value - base_, length};
        }

        void SetHost(const char *value, size_t length) {
            host = {value - base_, length};
        }

        void SetPort(const char *value, size_t length) {
            port = {value - base_, length};
        }

        void SetPath(const char *value, size_t length) {
            path = {value - base_, length};
        }

        void SetQuery(const char *value, size_t length) {
            query = {value - base_, length};
        }

        void SetFragment(const char *value, size_t length) {
            fragment = {value - base_, length};
        }

        std::pair<std::uint16_t, std::uint16_t> scheme, userinfo, host, port, path,
                query, fragment;

    private:
        const char *base_;
    };

    class uri_parser {
        friend class parse_result;

        // Helper class to auto rollback when parse failure
        class parse_result {
        public:
            // remember current reading pointer
            explicit parse_result(uri_parser *parser)
                    : m_parser(parser), m_begin(parser->m_current), m_result(false) {}

            ~parse_result() {
                // auto rollback if not success
                if (!m_result) m_parser->m_current = m_begin;
            }

            operator bool() const { return m_result; }

            parse_result &operator=(bool value) {
                m_result = value;
                if (!value) m_parser->m_current = m_begin;
                return *this;
            }

            const char *begin() const { return m_begin; }

            const char *end() const { return m_parser->m_current; }

            size_t length() const { return m_parser->m_current - m_begin; }

            void Reset() {
                m_begin = m_parser->m_current;
                m_result = false;
            }

        private:
            parse_result(const parse_result &);

            parse_result &operator=(const parse_result &);

        private:
            uri_parser *m_parser;
            const char *m_begin;
            bool m_result;
        };

    public:
        uri_parser()
                : m_byte_sets(uri_parser_byte_sets::instance()),
                  m_begin(nullptr),
                  m_end(nullptr),
                  m_current(nullptr),
                  m_result(nullptr) {}

        size_t Parse(const char *uri, size_t uri_length,
                     uri_parse_result_receiver *result) {
            m_begin = uri;
            m_current = uri;
            m_end = uri + uri_length;
            m_result = result;
            match_URI_reference();
            return m_current - m_begin;
        }

    private:  // in RFC 3986 Appendix A.  Collected ABNF for URI order
        // URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
        bool match_URI() {
            parse_result r(this);
            r = match_scheme_and_colon() && match_hier_part();
            if (r) {
                {
                    parse_result r1(this);
                    r1 = match_literal('?') && match_query();
                    if (r1) m_result->SetQuery(r1.begin() + 1, r1.length() - 1);
                }
                {
                    parse_result r1(this);
                    r1 = match_literal('#') && match_fragment();
                    if (r1) m_result->SetFragment(r1.begin() + 1, r1.length() - 1);
                }
            }
            return r;
        }

        // hier-part     = "//" authority path-abempty
        //               / path-absolute
        //               / path-rootless
        //               / path-empty
        bool match_hier_part() {
            parse_result r(this);
            r = match_literal("//") && match_authority() && match_path_abempty();
            if (r) return true;
            return match_path_absolute() || match_path_rootless() || match_path_empty();
        }

        // URI-reference = URI / relative-ref
        bool match_URI_reference() { return match_URI() || match_relative_ref(); }

        // absolute-URI  = scheme ":" hier-part [ "?" query ]
        bool match_absoluteURI() {
            parse_result r(this);
            if (match_scheme_and_colon()) {
                r = match_hier_part();
                if (r) {
                    parse_result r1(this);
                    r1 = match_literal('?') && match_query();
                    if (r1) m_result->SetQuery(r1.begin() + 1, r1.length() - 1);
                }
            }
            return r;
        }

        // relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
        bool match_relative_ref() {
            parse_result r(this);
            r = match_relative_part();
            if (r) {
                parse_result r1(this);
                r1 = match_literal('?') && match_query();
                if (r1) m_result->SetQuery(r1.begin() + 1, r1.length() - 1);
                return true;
            }
            return false;
        }

        // relative-part = "//" authority path-abempty
        //               / path-absolute
        //               / path-noscheme
        //               / path-empty
        bool match_relative_part() {
            parse_result r(this);
            r = match_literal("//") && match_authority() && match_path_abempty();
            if (!r) {
                r = match_path_absolute() || match_path_noscheme() || match_path_empty();
            }
            return r;
        }

        // scheme        = alpha *( alpha | digit | "+" | "-" | "." )
        // colon         = :
        bool match_scheme_and_colon() {
            parse_result r(this);
            r = match_alpha();
            if (r) {
                while (match_byteset(m_byte_sets.scheme)) {
                }
                if (match_literal(':'))
                    m_result->SetScheme(r.begin(), r.length() - 1);
                else
                    r = false;
            }
            return r;
        }

        bool maybe_contains_userinfo() const {
            for (const char *p = m_current; p < m_end; ++p) {
                switch (*p) {
                    case '@':
                        return true;
                    case '/':
                        return false;
                }
            }
            return false;
        }

        // authority     = [ userinfo "@" ] host [ ":" port ]
        bool match_authority() {
            if (maybe_contains_userinfo()) {
                parse_result r(this);
                r = match_userinfo() && match_literal('@');
                if (r) m_result->SetUserInfo(r.begin(), r.length() - 1);
            }

            if (!match_host()) return false;

            parse_result r(this);
            r = match_literal(':') && match_port();
            if (r) m_result->SetPort(r.begin() + 1, r.length() - 1);

            return true;
        }

        // userinfo = *( unreserved | escaped | ";" | ":" | "&" | "=" | "+" | "$" |
        //               "," )
        bool match_userinfo() {
            while (match_byteset(m_byte_sets.userinfo) || match_pct_encoded()) {
            }
            return true;
        }

        // host          = IP-literal / IPv4address / reg-name
        bool match_host() {
            const char *begin = m_current;
            if (match_IPv4address() || match_ip_literal() || match_reg_name()) {
                m_result->SetHost(begin, m_current - begin);
                return true;
            }
            return false;
        }

        // port          = *digit
        bool match_port() {
            while (match_byteset(m_byte_sets.digit)) {
            }
            return true;
        }

        // IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
        // We don't accept IPv6 address to simplify the implementation
        bool match_ip_literal() { return false; }

        // IPv4address   = 1*digit "." 1*digit "." 1*digit "." 1*digit
        bool match_IPv4address() {
            parse_result r(this);
            for (int i = 0; i < 3; ++i) {
                if (match_digit()) {
                    while (match_digit()) {
                    }
                    if (!match_literal('.')) return false;
                }
            }

            // last field, no following dot
            if (match_digit()) {
                while (match_digit()) {
                }
                r = true;
            }

            return r;
        }

        // reg-name      = *( unreserved / pct-encoded / sub-delims )
        bool match_reg_name() {
            parse_result r(this);
            int n = 0;
            while (match_byteset(m_byte_sets.reg_name) || match_pct_encoded()) {
                ++n;
            }
            r = n > 0;
            return r;
        }

        // path          = path-abempty    ; begins with "/" or is empty
        //               / path-absolute   ; begins with "/" but not "//"
        //               / path-noscheme   ; begins with a non-colon segment
        //               / path-rootless   ; begins with a segment
        //               / path-empty      ; zero characters
        bool match_path() {
            return match_path_abempty() || match_path_absolute() ||
                   match_path_noscheme() || match_path_rootless() || match_path_empty();
        }

        // path-abempty  = *( "/" segment )
        bool match_path_abempty() {
            const char *begin = m_current;
            for (;;) {
                parse_result r1(this);
                r1 = match_literal('/') && match_segment();
                if (!r1) break;
            }
            m_result->SetPath(begin, m_current - begin);
            return true;
        }

        // path-absolute = "/" [ segment-nz *( "/" segment ) ]
        bool match_path_absolute() {
            const char *begin = m_current;
            if (!match_literal('/')) return false;
            if (match_segment_nz()) {
                for (;;) {
                    parse_result r(this);
                    r = match_literal('/') && match_segment();
                    if (!r) break;
                }
            }
            m_result->SetPath(begin, m_current - begin);
            return true;
        }

        // path-noscheme = segment-nz-nc *( "/" segment )
        bool match_path_noscheme() {
            const char *begin = m_current;
            if (!match_segment_nz_nc()) return false;
            for (;;) {
                parse_result r(this);
                r = match_literal('/') && match_segment();
                if (!r) break;
            }
            m_result->SetPath(begin, m_current - begin);
            return true;
        }

        // path-rootless = segment-nz *( "/" segment )
        bool match_path_rootless() {
            const char *begin = m_current;
            if (!match_segment_nz()) return false;
            for (;;) {
                parse_result r(this);
                r = match_literal('/') && match_segment();
                if (!r) break;
            }
            m_result->SetPath(begin, m_current - begin);
            return true;
        }

        // path-empty    = 0<pchar>
        bool match_path_empty() {
            m_result->SetPath("", 0);
            return true;
        }

        // segment       = *pchar
        bool match_segment() {
            while (match_pchar()) {
            }
            return true;
        }

        // segment-nz    = 1*pchar
        bool match_segment_nz() {
            int count = 0;
            while (match_pchar()) ++count;
            return count > 0;
        }

        // segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
        //               ; non-zero-length segment without any colon ":"
        bool match_segment_nz_nc() {
            int count = 0;
            for (;;) {
                parse_result r(this);
                r = match_unreserved() || match_pct_encoded() ||
                    match_byteset(m_byte_sets.sub_delims) || match_literal('@');
                if (r)
                    ++count;
                else
                    break;
            }
            return count > 0;
        }

        // pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
        bool match_pchar() {
            return match_byteset(m_byte_sets.pchar) || match_pct_encoded();
        }

        // escaped       = "%" hex hex |
        //                 "%u" hex hex hex hex
        bool match_pct_encoded() {
            size_t left = m_end - m_current;
            if (left > 2 && *m_current == '%') {
                if (m_byte_sets.hex.contains(m_current[1]) &&
                    m_byte_sets.hex.contains(m_current[2])) {
                    m_current += 3;
                    return true;
                }

                // "%uXXXX"
                if (left > 5 && m_current[1] == 'u' &&
                    m_byte_sets.hex.contains(m_current[2]) &&
                    m_byte_sets.hex.contains(m_current[3]) &&
                    m_byte_sets.hex.contains(m_current[4]) &&
                    m_byte_sets.hex.contains(m_current[5])) {
                    m_current += 6;
                    return true;
                }
            }
            return false;
        }

        // unreserved    = alphanum | mark
        bool match_unreserved() { return match_byteset(m_byte_sets.unreserved); }

        // reserved      = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ","
        bool match_reserved() { return match_byteset(m_byte_sets.reserved); }

    private:  // helpers
        // hostname = *( domainlabel "." ) toplabel [ "." ]
        bool match_hostname() {
            parse_result r(this);
            for (;;) {
                parse_result r1(this);
                r1 = match_domainlabel() && match_literal('.');
                if (!r1) break;
            }
            r = match_toplabel();
            if (r) match_literal('.');
            return r;
        }

        // domainlabel   = alphanum | alphanum *( alphanum | "-" ) alphanum
        bool match_domainlabel() {
            // alphanum | alphanum *( alphanum | "-" ) alphanum
            parse_result r(this);
            if (match_alphanum()) {
                parse_result r1(this);
                for (;;) {
                    while (match_literal(
                            '-')) {  // || match_literal('_')) // allow _ in domain?
                    }
                    if (match_alphanum())
                        r1 = true;
                    else
                        break;
                }
                r = true;
            }
            return r;
        }

        // toplabel      = alpha | alpha *( alphanum | "-" ) alphanum
        bool match_toplabel() {
            parse_result r(this);
            if (match_alpha()) {
                parse_result r1(this);
                for (;;) {
                    while (match_literal('-')) {
                    }
                    if (match_alphanum())
                        r1 = true;
                    else
                        break;
                }
                r = true;
            }
            return r;
        }

        // RFC 3986
        // query         = *( pchar / "/" / "?" )
        bool match_query() {
            while (match_byteset(m_byte_sets.query) || match_pct_encoded()) {
            }
            return true;
        }

        // fragment      = *( pchar / "/" / "?" / "#" )
        bool match_fragment() {
            while (match_byteset(m_byte_sets.fragment) || match_pct_encoded()) {
            }
            return true;
        }

        // mark          = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
        bool match_mark() { return match_byteset(m_byte_sets.mark); }

        bool match_hex() { return match_byteset(m_byte_sets.hex); }

        // escaped       = "%" hex hex
        bool match_percent_encoded() {
            size_t left = m_end - m_current;
            if (left > 2 && m_current[0] == '%' &&
                m_byte_sets.hex.contains(m_current[1]) &&
                m_byte_sets.hex.contains(m_current[2])) {
                m_current += 3;
                return true;
            }
            return false;
        }

        bool match_alphanum() { return match_byteset(m_byte_sets.alphanum); }

        bool match_alpha() { return match_byteset(m_byte_sets.alpha); }

        bool match_digit() { return match_byteset(m_byte_sets.digit); }

        bool match_literal(char c) {
            if (m_current < m_end && *m_current == c) {
                ++m_current;
                return true;
            }
            return false;
        }

        template<size_t N>
        bool match_literal(const char (&l)[N]) {
            const ptrdiff_t w = N - 1;
            if (m_end - m_current >= w && !memcmp(m_current, l, w)) {
                m_current += w;
                return true;
            }
            return false;
        }

        template<typename Pred>
        bool match_byteset(const Pred &cs) {
            if (m_current < m_end && cs.contains(*m_current)) {
                ++m_current;
                return true;
            }
            return false;
        }

    private:
        const uri_parser_byte_sets &m_byte_sets;
        const char *m_begin;
        const char *m_end;
        const char *m_current;
        uri_parse_result_receiver *m_result;
    };

    std::optional<http_uri> parse_uri(const std::string_view &s) {
        if (s.size() > std::numeric_limits<std::uint16_t>::max()) {
            DLOG_ERROR("Unexpected: URI is too long.");
            return std::nullopt;
        }

        std::string uri_copy(s);
        http_uri::uri_components components;
        std::uint16_t port = 0;

        uri_parse_result_receiver receiver(s.data());
        uri_parser parser;

        if (parser.Parse(s.data(), s.size(), &receiver) != s.size()) {
            return std::nullopt;
        }

        components[http_uri::kScheme] = receiver.scheme;
        components[http_uri::kUserInfo] = receiver.userinfo;
        components[http_uri::kHost] = receiver.host;
        components[http_uri::kPort] = receiver.port;
        components[http_uri::kPath] = receiver.path;
        components[http_uri::kQuery] = receiver.query;
        components[http_uri::kFragment] = receiver.fragment;

        std::string_view possible_port(s.data() + receiver.port.first,
                                       receiver.port.second);
        if (!possible_port.empty()) {
            int64_t p;
            if (abel::simple_atoi(possible_port, &p)) {
                port = p;
            } else {
                return std::nullopt;
            }
        }

        // https://tools.ietf.org/html/rfc3986#section-3.1:
        //
        // > Although schemes are case-insensitive, the canonical form is lowercase
        // > and documents that specify schemes must do so with lowercase letters.
        auto scheme_since = uri_copy.begin() + components[http_uri::kScheme].first;
        auto scheme_upto = scheme_since + components[http_uri::kScheme].second;
        std::transform(scheme_since, scheme_upto, scheme_since, ::tolower);
        return http_uri(std::move(uri_copy), components, port);
    }

    std::string http_uri_builder::to_string(bool with_user_info) const {
        std::string ret;
        if (!_host.empty()) {
            if (!_scheme.empty()) {
                ret += _scheme;
                ret += "://";
            } else {
                ret += "http://";
            }
            if(with_user_info && !_userinfo.empty()) {
                ret += _userinfo;
                ret += "@";
            }
            ret += _host;
            if (_port != 0) {
                ret += ":";
                ret += std::to_string(_port);
            }
        }


        if (_path.empty()) {
            ret += "/";
        } else {
            ret += _path;
        }

        if (!_query_map.empty()) {
            bool first = true;
            for (auto it = _query_map.begin(); it != _query_map.end(); ++it) {
                if (!first) {
                    ret += "&";
                } else {
                    ret += "?";
                    first = false;
                }
                ret += it->first;
                ret += "=";
                ret += it->second;
            }
        }

        if (!_fragment.empty()) {
            ret += "#";
            ret += _fragment;
        }
        return ret;
    }

    std::optional<http_uri> http_uri_builder::build(bool with_user_info) const {
        auto str = to_string(with_user_info);
        auto ret = parse_uri(str);
        return ret;
    }

    void http_uri_builder::set_host_and_port(std::string_view hp) {
        std::vector<std::string_view> segment = abel::string_split(hp, abel::by_char(':'));
        if(segment.size() == 1) {
            _host = hp;
        }
        if(segment.size() == 2) {
            _host = segment[0];
            int64_t p;
            auto r = abel::simple_atoi(segment[1], &p);
            if(r) {
                _port = p;
            }
        }
    }

}  // namespace abel
