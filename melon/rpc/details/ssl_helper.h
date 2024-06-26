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



#pragma once

#include <string.h>
#include <openssl/ssl.h>
// For some versions of openssl, SSL_* are defined inside this header
#include <openssl/ossl_typ.h>
#include <openssl/opensslv.h>
#include <melon/rpc/socket_id.h>                 // SocketId
#include <melon/rpc/ssl_options.h>               // ServerSSLOptions
#include <melon/rpc/adaptive_protocol_type.h>    // AdaptiveProtocolType

namespace melon {

// The calculation method is the same as OPENSSL_VERSION_NUMBER in the openssl/crypto.h file.
// SSL_VERSION_NUMBER can pass parameter calculation instead of using fixed macro.
#define SSL_VERSION_NUMBER(major, minor, patch) \
    ( (major << 28) | (minor << 20) | (patch << 4) )

enum SSLState {
    SSL_UNKNOWN = 0,
    SSL_OFF = 1,                // Not an SSL connection
    SSL_CONNECTING = 2,         // During SSL handshake
    SSL_CONNECTED = 3,          // SSL handshake completed
};

enum SSLProtocol {
    SSLv3 = 1 << 0,
    TLSv1 = 1 << 1,
    TLSv1_1 = 1 << 2,
    TLSv1_2 = 1 << 3,
};

struct FreeSSLCTX {
    inline void operator()(SSL_CTX* ctx) const {
        if (ctx != NULL) {
            SSL_CTX_free(ctx);
        }
    }
};

struct SSLError {
    explicit SSLError(unsigned long e) : error(e) { }
    unsigned long error;
};
std::ostream& operator<<(std::ostream& os, const SSLError&);
std::ostream& operator<<(std::ostream& os, const CertInfo&);

const char* SSLStateToString(SSLState s);

// Initialize locks and callbacks to make SSL work under multi-threaded
// environment. Return 0 on success, -1 otherwise
int SSLThreadInit();

// Initialize global Diffie-Hellman parameters used for DH key exchanges
// Return 0 on success, -1 otherwise
int SSLDHInit();

// Create a new SSL_CTX in client mode and
// set the right options according `options'
SSL_CTX* CreateClientSSLContext(const ChannelSSLOptions& options);

// Create a new SSL_CTX in server mode using `certificate_file'
// and `private_key_file' and then set the right options and alpn
// onto it according `options'.Finally, extract hostnames from CN/subject
// fields into `hostnames'
// Attention: ensure that the life cycle of function return is greater than alpns param.
SSL_CTX* CreateServerSSLContext(const std::string& certificate_file,
                                const std::string& private_key_file,
                                const ServerSSLOptions& options,
                                const std::string* alpns,
                                std::vector<std::string>* hostnames);

// Create a new SSL (per connection object) using configurations in `ctx'.
// Set the required `fd' and mode. `id' will be set into SSL as app data.
SSL* CreateSSLSession(SSL_CTX* ctx, SocketId id, int fd, bool server_mode);

// Add a buffer layer of BIO in front of the socket fd layer,
// which can reduce the total number of calls to system read/write
void AddBIOBuffer(SSL* ssl, int fd, int bufsize);

// Judge whether the underlying channel of `fd' is using SSL
// If the return value is SSL_UNKNOWN, `error_code' will be
// set to indicate the reason (0 for EOF)
SSLState DetectSSLState(int fd, int* error_code);

void Print(std::ostream& os, SSL* ssl, const char* sep);
void Print(std::ostream& os, X509* cert, const char* sep);

std::string ALPNProtocolToString(const AdaptiveProtocolType& protocol);

// Build a binary formatted ALPN protocol list that OpenSSL's
// `SSL_CTX_set_alpn_protos` accepts from a C++ string vector.
bool BuildALPNProtocolList(
    const std::vector<std::string>& alpn_protocols,
    std::vector<unsigned char>& result
);

} // namespace melon
