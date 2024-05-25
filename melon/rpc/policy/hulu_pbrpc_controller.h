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

#include <stdint.h>                             // int64_t
#include <string>                               // std::string
#include <melon/rpc/controller.h>                    // Controller

namespace melon {
namespace policy {

// Special Controller that can be filled with hulu-pbrpc specific meta fields.
class HuluController : public Controller {
public:
    HuluController()
        : _request_source_addr(0)
        , _response_source_addr(0)
    {}

    void Reset() {
        _request_source_addr = 0;
        _response_source_addr = 0;
        _request_user_data.clear();
        _response_user_data.clear();
        Controller::Reset();
    }


    // ------------------------------------------------------------------
    //                      Client-side methods
    // These calls shall be made from the client side only.  Their results
    // are undefined on the server side (may crash).
    // ------------------------------------------------------------------
   
    // Send the address that the client listens as a server to the remote
    // side.
    int64_t request_source_addr() const { return _request_source_addr; }
    void set_request_source_addr(int64_t request_source_addr)
    { _request_source_addr = request_source_addr; }

    // Send a raw data along with the Hulu rpc meta instead of carrying it with
    // the request.
    const std::string& request_user_data() const { return _request_user_data; }
    void set_request_user_data(const std::string& request_user_data)
    { _request_user_data = request_user_data; }

    // ------------------------------------------------------------------------
    //                      Server-side methods.
    // These calls shall be made from the server side only. Their results are
    // undefined on the client side (may crash).
    // ------------------------------------------------------------------------
    
    // Send the address that the server listens to the remote side.
    int64_t response_source_addr() const { return _response_source_addr; }
    void set_response_source_addr(int64_t response_source_addr)
    { _response_source_addr = response_source_addr; }

    // Send a raw data along with the Hulu rpc meta instead of carrying it with
    // the response.
    const std::string& response_user_data() const { return _response_user_data; }
    void set_response_user_data(const std::string& response_user_data)
    { _response_user_data = response_user_data; }
    

private:
    int64_t _request_source_addr;
    int64_t _response_source_addr;
    std::string _request_user_data;
    std::string _response_user_data;
};

}  // namespace policy
} // namespace melon
