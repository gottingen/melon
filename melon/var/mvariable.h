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

#include <ostream>                      // std::ostream
#include <sstream>                      // std::ostringstream
#include <list>                         // std::list
#include <string>                       // std::string
#include <melon/utility/macros.h>               // DISALLOW_COPY_AND_ASSIGN
#include <melon/utility/strings/string_piece.h>

namespace melon::var {

    class Dumper;

    struct DumpOptions;

    class MVariable {
    public:
        explicit MVariable(const std::list<std::string> &labels);

        virtual ~MVariable();

        // Implement this method to print the mvariable info into ostream.
        virtual void describe(std::ostream &) = 0;

        // string form of describe().
        std::string get_description();

        // Get mvariable name
        const std::string &name() const { return _name; }

        // Get mvariable labels
        const std::list<std::string> &labels() const { return _labels; }

        // Get number of mvariable labels
        size_t count_labels() const { return _labels.size(); }

        // Expose this mvariable globally so that it's counted in following
        // functions:
        //      list_exposed
        //      count_exposed
        // Return 0 on success, -1 otherwise.
        int expose(const std::string_view &name) {
            return expose_impl(std::string_view(), name);
        }

        // Expose this mvariable globally with a prefix
        // Return 0 on success, -1 otherwise.
        int expose_as(const std::string_view &prefix,
                      const std::string_view &name) {
            return expose_impl(prefix, name);
        }

        // Dump this mvariable
        virtual size_t dump(Dumper *dumper, const DumpOptions *options) = 0;

        // Hide this variable so that it's not counted in *_exposed functions.
        // Returns false if this variable is already hidden.
        // CAUTION!! Subclasses must call hide() manually to avoid displaying
        // a variable that is just destructing.
        bool hide();

        // Get number of exposed mvariables.
        static size_t count_exposed();

        // Put names of all exposed mvariable into `name'.
        static void list_exposed(std::vector<std::string> *names);

        // Find all exposed mvariables matching `white_wildcards' but
        // `black_wildcards' and send them to `dumper'.
        // Use default options when `options' is NULL.
        // Return number of dumped mvariables, -1 on error.
        static size_t dump_exposed(Dumper *dumper, const DumpOptions *options);

        // Find an exposed mvariable by `name' and put its description into `os'.
        // Returns 0 on found, -1 otherwise.
        static int describe_exposed(const std::string &name,
                                    std::ostream &os);

        // String form. Returns empty string when not found.
        static std::string describe_exposed(const std::string &name);

#ifdef UNIT_TEST
        // Hide all mvariables so that all mvariables not counted in following
        // functions:
        //      list_exposed
        //      count_exposed
        // CAUTION!!! Just For Debug!!!
        static void hide_all();
#endif

    protected:
        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name);

    protected:
        std::string _name;
        std::list<std::string> _labels;

        // mvar uses var, var uses TLS, thus copying/assignment need to copy TLS stuff as well,
        // which is heavy. We disable copying/assignment now.
        DISALLOW_COPY_AND_ASSIGN(MVariable);
    };

} // namespace melon::var
