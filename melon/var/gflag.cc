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


#include <cstdlib>
#include <gflags/gflags.h>
#include <melon/var/gflag.h>

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
