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


#include <stdlib.h>
#include <gflags/gflags.h>
#include "melon/var/gflag.h"

namespace melon::var {

    GFlag::GFlag(const mutil::StringPiece &gflag_name) {
        expose(gflag_name);
    }

    GFlag::GFlag(const mutil::StringPiece &prefix,
                 const mutil::StringPiece &gflag_name)
            : _gflag_name(gflag_name.data(), gflag_name.size()) {
        expose_as(prefix, gflag_name);
    }

    void GFlag::describe(std::ostream &os, bool quote_string) const {
        google::CommandLineFlagInfo info;
        if (!google::GetCommandLineFlagInfo(gflag_name().c_str(), &info)) {
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
        if (!google::GetCommandLineOption(gflag_name().c_str(), &str)) {
            return "Unknown gflag=" + gflag_name();
        }
        return str;
    }

    bool GFlag::set_value(const char *value) {
        return !google::SetCommandLineOption(gflag_name().c_str(), value).empty();
    }

}  // namespace melon::var
