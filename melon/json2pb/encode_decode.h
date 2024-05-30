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

#include <string>

namespace json2pb {

    //pattern: _Zxxx_
    //rules: keep original lower-case characters, upper-case characters,
    //digital charactors and '_' in the original position,
    //change other special characters to '_Zxxx_',
    //xxx is the character's decimal digit
    //fg: 'abc123_ABC-' convert to 'abc123_ABC_Z045_'

    //params: content: content need to encode
    //params: encoded_content: content encoded
    //return value: false: no need to encode, true: need to encode.
    //note: when return value is false, no change in encoded_content.
    bool encode_name(const std::string &content, std::string &encoded_content);

    //params: content: content need to decode
    //params: decoded_content: content decoded
    //return value: false: no need to decode, true: need to decode.
    //note: when return value is false, no change in decoded_content.
    bool decode_name(const std::string &content, std::string &decoded_content);

}
