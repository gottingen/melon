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

#include <stddef.h>
#include <stdint.h>
#include <melon/utility/strings/string_piece.h>


namespace melon {
namespace policy {

using HashFunc = uint32_t(*)(const void*, size_t);

void MD5HashSignature(const void* key, size_t len, unsigned char* results);
uint32_t MD5Hash32(const void* key, size_t len);
uint32_t MD5Hash32V(const std::string_view* keys, size_t num_keys);

uint32_t MurmurHash32(const void* key, size_t len);
uint32_t MurmurHash32V(const std::string_view* keys, size_t num_keys);

}  // namespace policy
} // namespace melon
