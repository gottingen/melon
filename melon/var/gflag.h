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


#ifndef  MELON_VAR_GFLAG_H_
#define  MELON_VAR_GFLAG_H_

#include <string>                       // std::string
#include "melon/var/variable.h"

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

#endif  //  MELON_VAR_GFLAG_H_
