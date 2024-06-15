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

#include <ostream>                     // std::ostream
#include <string>                      // std::string
#include <vector>                      // std::vector
#include <string_view>
#include <gflags/gflags_declare.h>
#include <melon/utility/macros.h>               // DISALLOW_COPY_AND_ASSIGN

namespace melon::var {

    DECLARE_bool(save_series);

    // Bitwise masks of displayable targets
    enum DisplayFilter {
        DISPLAY_ON_HTML = 1,
        DISPLAY_ON_PLAIN_TEXT = 2,
        DISPLAY_ON_ALL = 3,
    };

    // Implement this class to write variables into different places.
    // If dump() returns false, Variable::dump_exposed() stops and returns -1.
    class Dumper {
    public:
        virtual ~Dumper() {}

        virtual bool dump(const std::string &name,
                          const std::string_view &description) = 0;

        virtual bool dump_comment(const std::string &, const std::string & /*type*/) {
            return true;
        }
    };

    // Options for Variable::dump_exposed().
    struct DumpOptions {
        // Constructed with default options.
        DumpOptions();

        // If this is true, string-type values will be quoted.
        bool quote_string;

        // The ? in wildcards. Wildcards in URL need to use another character
        // because ? is reserved.
        char question_mark;

        // Dump variables with matched display_filter
        DisplayFilter display_filter;

        // Name matched by these wildcards (or exact names) are kept.
        std::string white_wildcards;

        // Name matched by these wildcards (or exact names) are skipped.
        std::string black_wildcards;
    };

    struct SeriesOptions {
        SeriesOptions() : fixed_length(true), test_only(false) {}

        bool fixed_length; // useless now
        bool test_only;
    };

    // Base class of all var.
    //
    // About thread-safety:
    //   var is thread-compatible:
    //     Namely you can create/destroy/expose/hide or do whatever you want to
    //     different var simultaneously in different threads.
    //   var is NOT thread-safe:
    //     You should not operate one var from different threads simultaneously.
    //     If you need to, protect the ops with locks. Similarly with ordinary
    //     variables, const methods are thread-safe, namely you can call
    //     describe()/get_description()/get_value() etc from diferent threads
    //     safely (provided that there's no non-const methods going on).
    class Variable {
    public:
        Variable() {}

        virtual ~Variable();

        // Implement this method to print the variable into ostream.
        virtual void describe(std::ostream &, bool quote_string) const = 0;

        // string form of describe().
        std::string get_description() const;


        // Describe saved series as a json-string into the stream.
        // The output will be ploted by flot.js
        // Returns 0 on success, 1 otherwise(this variable does not save series).
        virtual int describe_series(std::ostream &, const SeriesOptions &) const { return 1; }

        // Expose this variable globally so that it's counted in following
        // functions:
        //   list_exposed
        //   count_exposed
        //   describe_exposed
        //   find_exposed
        // Return 0 on success, -1 otherwise.
        int expose(const std::string_view &name,
                   DisplayFilter display_filter = DISPLAY_ON_ALL) {
            return expose_impl(std::string_view(), name, display_filter);
        }

        // Expose this variable with a prefix.
        // Example:
        //   namespace foo {
        //   namespace bar {
        //   class ApplePie {
        //       ApplePie() {
        //           // foo_bar_apple_pie_error
        //           _error.expose_as("foo_bar_apple_pie", "error");
        //       }
        //   private:
        //       melon::var::Adder<int> _error;
        //   };
        //   }  // foo
        //   }  // bar
        // Returns 0 on success, -1 otherwise.
        int expose_as(const std::string_view &prefix,
                      const std::string_view &name,
                      DisplayFilter display_filter = DISPLAY_ON_ALL) {
            return expose_impl(prefix, name, display_filter);
        }

        // Hide this variable so that it's not counted in *_exposed functions.
        // Returns false if this variable is already hidden.
        // CAUTION!! Subclasses must call hide() manually to avoid displaying
        // a variable that is just destructing.
        bool hide();

        // Check if this variable is is_hidden.
        bool is_hidden() const;

        // Get exposed name. If this variable is not exposed, the name is empty.
        const std::string &name() const { return _name; }

        // ====================================================================

        // Put names of all exposed variables into `names'.
        // If you want to print all variables, you have to go through `names'
        // and call `describe_exposed' on each name. This prevents an iteration
        // from taking the lock too long.
        static void list_exposed(std::vector<std::string> *names,
                                 DisplayFilter = DISPLAY_ON_ALL);

        // Get number of exposed variables.
        static size_t count_exposed();

        // Find an exposed variable by `name' and put its description into `os'.
        // Returns 0 on found, -1 otherwise.
        static int describe_exposed(const std::string &name,
                                    std::ostream &os,
                                    bool quote_string = false,
                                    DisplayFilter = DISPLAY_ON_ALL);

        // String form. Returns empty string when not found.
        static std::string describe_exposed(const std::string &name,
                                            bool quote_string = false,
                                            DisplayFilter = DISPLAY_ON_ALL);

        // Describe saved series of variable `name' as a json-string into `os'.
        // The output will be ploted by flot.js
        // Returns 0 on success, 1 when the variable does not save series, -1
        // otherwise (no variable named so).
        static int describe_series_exposed(const std::string &name,
                                           std::ostream &,
                                           const SeriesOptions &);


        // Find all exposed variables matching `white_wildcards' but
        // `black_wildcards' and send them to `dumper'.
        // Use default options when `options' is NULL.
        // Return number of dumped variables, -1 on error.
        static int dump_exposed(Dumper *dumper, const DumpOptions *options);

    protected:
        virtual int expose_impl(const std::string_view &prefix,
                                const std::string_view &name,
                                DisplayFilter display_filter);

    private:
        std::string _name;

        // var uses TLS, thus copying/assignment need to copy TLS stuff as well,
        // which is heavy. We disable copying/assignment now.
        DISALLOW_COPY_AND_ASSIGN(Variable);
    };

    // Make name only use lowercased alphabets / digits / underscores, and append
    // the result to `out'.
    // Examples:
    //   foo-inl.h       -> foo_inl_h
    //   foo::bar::Apple -> foo_bar_apple
    //   Car_Rot         -> car_rot
    //   FooBar          -> foo_bar
    //   RPCTest         -> rpctest
    //   HELLO           -> hello
    void to_underscored_name(std::string *out, const std::string_view &name);

}  // namespace melon::var

// Make variables printable.
namespace std {

    inline ostream &operator<<(ostream &os, const ::melon::var::Variable &var) {
        var.describe(os, false);
        return os;
    }

}  // namespace std
