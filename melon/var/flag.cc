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
#include <turbo/flags/reflection.h>
#include <melon/var/flag.h>

namespace melon::var {

    Flag::Flag(const mutil::StringPiece &gflag_name) {
        expose(gflag_name);
    }

    Flag::Flag(const mutil::StringPiece &prefix,
                 const mutil::StringPiece &gflag_name)
            : _flag_name(gflag_name.data(), gflag_name.size()) {
        expose_as(prefix, gflag_name);
    }

    void Flag::describe(std::ostream &os, bool quote_string) const {
        auto flag_info = turbo::find_command_line_flag(flag_name());
        if (!flag_info) {
            if (quote_string) {
                os << '"';
            }
            os << "Unknown flag=" << flag_name();
            if (quote_string) {
                os << '"';
            }
        } else {
            if (quote_string && flag_info->is_of_type<std::string>()) {
                os << '"' << flag_info->current_value() << '"';
            } else {
                os << flag_info->current_value();
            }
        }
    }


    std::string Flag::get_value() const {
        std::string str;
        auto flag_info = turbo::find_command_line_flag(flag_name());
        if (!flag_info) {
            return "Unknown flag=" + flag_name();
        }
        return flag_info->current_value();
    }

    bool Flag::set_value(const char *value) {
        auto flag_info = turbo::find_command_line_flag(flag_name());
        if (!flag_info) {
            return false;
        }
        std::string err;
        return flag_info->parse_from(value, &err);
    }

}  // namespace melon::var
