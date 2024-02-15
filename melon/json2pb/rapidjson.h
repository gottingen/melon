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

#ifndef  MELON_JSON2PB_RAPIDJSON_H_
#define  MELON_JSON2PB_RAPIDJSON_H_


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#endif

#include "melon/utility/third_party/rapidjson/allocators.h"
#include "melon/utility/third_party/rapidjson/document.h"
#include "melon/utility/third_party/rapidjson/encodedstream.h"
#include "melon/utility/third_party/rapidjson/encodings.h"
#include "melon/utility/third_party/rapidjson/filereadstream.h"
#include "melon/utility/third_party/rapidjson/filewritestream.h"
#include "melon/utility/third_party/rapidjson/prettywriter.h"
#include "melon/utility/third_party/rapidjson/rapidjson.h"
#include "melon/utility/third_party/rapidjson/reader.h"
#include "melon/utility/third_party/rapidjson/stringbuffer.h"
#include "melon/utility/third_party/rapidjson/writer.h"
#include "melon/utility/third_party/rapidjson/optimized_writer.h"
#include "melon/utility/third_party/rapidjson/error/en.h"  // GetErrorCode_En

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#pragma GCC diagnostic pop
#endif

#endif  // MELON_JSON2PB_RAPIDJSON_H_
