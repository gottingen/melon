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

#include <melon/rpc/authenticator.h>

namespace melon {
namespace policy {

// Request to couchbase for authentication.
// Notice that authentication for couchbase in special SASLAuthProtocol.
// Couchbase Server 2.2 provide CRAM-MD5 support for SASL authentication,
// but Couchbase Server prior to 2.2 using PLAIN SASL authentication.
class CouchbaseAuthenticator : public Authenticator {
 public:
  CouchbaseAuthenticator(const std::string& bucket_name,
                         const std::string& bucket_password)
      : bucket_name_(bucket_name), bucket_password_(bucket_password) {}

  int GenerateCredential(std::string* auth_str) const;

  int VerifyCredential(const std::string&, const mutil::EndPoint&,
                       melon::AuthContext*) const {
    return 0;
  }

 private:
  const std::string bucket_name_;
  const std::string bucket_password_;
};

}  // namespace policy
}  // namespace melon
