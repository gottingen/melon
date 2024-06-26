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
#include <string>                       // std::string
#include <melon/var/variable.h>

namespace melon::var {

// Expose important gflags as var so that they're monitored.
    class GFlag : public Variable {
    public:
        GFlag(const mutil::StringPiece &gflag_name);

        GFlag(const mutil::StringPiece &prefix,
              const mutil::StringPiece &gflag_name);

        // Calling hide() in dtor manually is a MUST required by Variable.
        ~GFlag() { hide(); }

        void describe(std::ostream &os, bool quote_string) const override;

        // Get value of the gflag.
        // We don't bother making the return type generic. This function
        // is just for consistency with other classes.
        std::string get_value() const;

        // Set the gflag with a new value.
        // Returns true on success.
        bool set_value(const char *value);

        // name of the gflag.
        const std::string &gflag_name() const {
            return _gflag_name.empty() ? name() : _gflag_name;
        }

    private:
        std::string _gflag_name;
    };

}  // namespace melon::var
