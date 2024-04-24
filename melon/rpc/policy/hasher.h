// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#pragma once

#include <stddef.h>
#include <stdint.h>
#include "melon/utility/strings/string_piece.h"


namespace melon {
namespace policy {

using HashFunc = uint32_t(*)(const void*, size_t);

void MD5HashSignature(const void* key, size_t len, unsigned char* results);
uint32_t MD5Hash32(const void* key, size_t len);
uint32_t MD5Hash32V(const mutil::StringPiece* keys, size_t num_keys);

uint32_t MurmurHash32(const void* key, size_t len);
uint32_t MurmurHash32V(const mutil::StringPiece* keys, size_t num_keys);

}  // namespace policy
} // namespace melon
