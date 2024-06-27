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
// Created by jeff on 24-6-27.
//

#include <melon/rpc/restful_response.h>
namespace melon {

    const std::string RestfulResponse::CONTENT_TYPE = "Content-Type";
    const std::string RestfulResponse::AUTHORIZATION = "Authorization";
    const std::string RestfulResponse::APPLICATION_JSON = "application/json";

    void RestfulResponse::set_access_control_all_allow() {
        set_header("Access-Control-Allow-Origin", "*");
        set_header("Access-Control-Allow-Method", "*");
        set_header("Access-Control-Allow-Headers", "*");
        set_header("Access-Control-Allow-Credentials", "true");
        set_header("Access-Control-Expose-Headers", "*");
    }
}