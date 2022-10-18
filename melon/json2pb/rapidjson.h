// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef  MELON_RPC_JSON2PB_RAPIDJSON_H_
#define  MELON_RPC_JSON2PB_RAPIDJSON_H_


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#endif

#include "melon/rapidjson/allocators.h"
#include "melon/rapidjson/document.h"
#include "melon/rapidjson/encodedstream.h"
#include "melon/rapidjson/encodings.h"
#include "melon/rapidjson/filereadstream.h"
#include "melon/rapidjson/filewritestream.h"
#include "melon/rapidjson/prettywriter.h"
#include "melon/rapidjson/rapidjson.h"
#include "melon/rapidjson/reader.h"
#include "melon/rapidjson/stringbuffer.h"
#include "melon/rapidjson/writer.h"
#include "melon/rapidjson/optimized_writer.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#pragma GCC diagnostic pop
#endif

#endif  // MELON_RPC_JSON2PB_RAPIDJSON_H_
