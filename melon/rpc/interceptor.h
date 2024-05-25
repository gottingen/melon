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


#ifndef MELON_RPC_INTERCEPTOR_H_
#define MELON_RPC_INTERCEPTOR_H_

#include <melon/rpc/controller.h>


namespace melon {

class Interceptor {
public:
    virtual ~Interceptor() = default;

    // Returns true if accept request, reject request otherwise.
    // When server rejects request, You can fill `error_code'
    // and `error_txt' which will send to client.
    virtual bool Accept(const melon::Controller* controller,
                        int& error_code,
                        std::string& error_txt) const = 0;

};

} // namespace melon


#endif // MELON_RPC_INTERCEPTOR_H_
