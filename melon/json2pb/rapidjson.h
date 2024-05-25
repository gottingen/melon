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

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#endif

#include <melon/utility/third_party/rapidjson/allocators.h>
#include <melon/utility/third_party/rapidjson/document.h>
#include <melon/utility/third_party/rapidjson/encodedstream.h>
#include <melon/utility/third_party/rapidjson/encodings.h>
#include <melon/utility/third_party/rapidjson/filereadstream.h>
#include <melon/utility/third_party/rapidjson/filewritestream.h>
#include <melon/utility/third_party/rapidjson/prettywriter.h>
#include <melon/utility/third_party/rapidjson/rapidjson.h>
#include <melon/utility/third_party/rapidjson/reader.h>
#include <melon/utility/third_party/rapidjson/stringbuffer.h>
#include <melon/utility/third_party/rapidjson/writer.h>
#include <melon/utility/third_party/rapidjson/optimized_writer.h>
#include <melon/utility/third_party/rapidjson/error/en.h>  // GetErrorCode_En

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#pragma GCC diagnostic pop
#endif

