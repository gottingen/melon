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

// Date: Sun Aug  9 12:26:03 CST 2015

#include <stdlib.h>
#include <gflags/gflags.h>
#include "melon/var/gflag.h"

namespace melon::var {

    GFlag::GFlag(const butil::StringPiece &gflag_name) {
        expose(gflag_name);
    }

    GFlag::GFlag(const butil::StringPiece &prefix,
                 const butil::StringPiece &gflag_name)
            : _gflag_name(gflag_name.data(), gflag_name.size()) {
        expose_as(prefix, gflag_name);
    }

    void GFlag::describe(std::ostream &os, bool quote_string) const {
        GFLAGS_NS::CommandLineFlagInfo info;
        if (!GFLAGS_NS::GetCommandLineFlagInfo(gflag_name().c_str(), &info)) {
            if (quote_string) {
                os << '"';
            }
            os << "Unknown gflag=" << gflag_name();
            if (quote_string) {
                os << '"';
            }
        } else {
            if (quote_string && info.type == "string") {
                os << '"' << info.current_value << '"';
            } else {
                os << info.current_value;
            }
        }
    }


    std::string GFlag::get_value() const {
        std::string str;
        if (!GFLAGS_NS::GetCommandLineOption(gflag_name().c_str(), &str)) {
            return "Unknown gflag=" + gflag_name();
        }
        return str;
    }

    bool GFlag::set_value(const char *value) {
        return !GFLAGS_NS::SetCommandLineOption(gflag_name().c_str(), value).empty();
    }

}  // namespace melon::var
