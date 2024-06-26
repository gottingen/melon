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



#ifndef MELON_RPC_SSL_OPTION_H_
#define MELON_RPC_SSL_OPTION_H_

#include <string>
#include <vector>

namespace melon {

struct CertInfo {
    // Certificate in PEM format.
    // Note that CN and alt subjects will be extracted from the certificate,
    // and will be used as hostnames. Requests to this hostname (provided SNI
    // extension supported) will be encrypted using this certificate.
    // Supported both file path and raw string
    std::string certificate;

    // Private key in PEM format.
    // Supported both file path and raw string based on prefix:
    std::string private_key;

    // Additional hostnames besides those inside the certificate. Wildcards
    // are supported but it can only appear once at the beginning (i.e. *.xxx.com).
    std::vector<std::string> sni_filters;
};

struct VerifyOptions {
    // Constructed with default options
    VerifyOptions();

    // Set the maximum depth of the certificate chain for verification
    // If 0, turn off the verification
    // Default: 0
    int verify_depth;

    // Set the trusted CA file to verify the peer's certificate
    // If empty, use the system default CA files
    // Default: ""
    std::string ca_file_path;
};

// SSL options at client side
struct ChannelSSLOptions {
    // Constructed with default options
    ChannelSSLOptions();

    // Cipher suites used for SSL handshake.
    // The format of this string should follow that in `man 1 ciphers'.
    // Default: "DEFAULT"
    std::string ciphers;

    // SSL protocols used for SSL handshake, separated by comma.
    // Available protocols: SSLv3, TLSv1, TLSv1.1, TLSv1.2
    // Default: TLSv1, TLSv1.1, TLSv1.2
    std::string protocols;

    // When set, fill this into the SNI extension field during handshake,
    // which can be used by the server to locate the right certificate.
    // Default: empty
    std::string sni_name;

    // Certificate used for client authentication
    // Default: empty
    CertInfo client_cert;

    // Options used to verify the server's certificate
    // Default: see above
    VerifyOptions verify;

    // Set the protocol preference of ALPN (Application-Layer Protocol Negotiation)
    // Default: unset
    std::vector<std::string> alpn_protocols;

    // TODO: Support CRL
};

// SSL options at server side
struct ServerSSLOptions {
    // Constructed with default options
    ServerSSLOptions();

    // Default certificate which will be loaded into server. Requests
    // without hostname or whose hostname doesn't have a corresponding
    // certificate will use this certificate. MUST be set to enable SSL.
    CertInfo default_cert;

    // Additional certificates which will be loaded into server. These
    // provide extra bindings between hostnames and certificates so that
    // we can choose different certificates according to different hostnames.
    // See `CertInfo' for detail.
    std::vector<CertInfo> certs;

    // When set, requests without hostname or whose hostname can't be found in
    // any of the cerficates above will be dropped. Otherwise, `default_cert'
    // will be used.
    // Default: false
    bool strict_sni;

    // When set, SSLv3 requests will be dropped. Strongly recommended since
    // SSLv3 has been found suffering from severe security problems. Note that
    // some old versions of browsers may use SSLv3 by default such as IE6.0
    // Default: true
    bool disable_ssl3;

    // Flag for SSL_MODE_RELEASE_BUFFERS. When set, release read/write buffers
    // when SSL connection is idle, which saves 34KB memory per connection.
    // On the other hand, it introduces additional latency and reduces throughput
    // Default: false
    bool release_buffer;

    // Maximum lifetime for a session to be cached inside OpenSSL in seconds.
    // A session can be reused (initiated by client) to save handshake before
    // it reaches this timeout.
    // Default: 300
    int session_lifetime_s;

    // Maximum number of cached sessions. When cache is full, no more new
    // session will be added into the cache until SSL_CTX_flush_sessions is
    // called (automatically by SSL_read/write). A special value is 0, which
    // means no limit.
    // Default: 20480
    int session_cache_size;

    // Cipher suites allowed for each SSL handshake. The format of this string
    // should follow that in `man 1 ciphers'. If empty, OpenSSL will choose
    // a default cipher based on the certificate information
    // Default: ""
    std::string ciphers;

    // Name of the elliptic curve used to generate ECDH ephemeral keys
    // Default: prime256v1
    std::string ecdhe_curve_name;

    // Options used to verify the client's certificate
    // Default: see above
    VerifyOptions verify;

    // Options used to choose the most suitable application protocol, separated by comma.
    // The NPN protocol is not commonly used, so only ALPN is supported.
    // Available protocols: http, h2, melon_std etc.
    // Default: empty
    std::string alpns;

    // TODO: Support OSCP stapling
};

// Legacy name defined in server.h
typedef ServerSSLOptions SSLOptions;

} // namespace melon

#endif // MELON_RPC_SSL_OPTION_H_
