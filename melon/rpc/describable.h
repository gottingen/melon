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

#include <ostream>
#include <melon/base/macros.h>
#include <melon/base/class_name.h>

namespace melon {

    struct DescribeOptions {
        DescribeOptions()
                : verbose(true), use_html(false) {}

        bool verbose;
        bool use_html;
    };

    class Describable {
    public:
        virtual ~Describable() {}

        virtual void Describe(std::ostream &os, const DescribeOptions &) const {
            os << mutil::class_name_str(*this);
        }
    };

    class NonConstDescribable {
    public:
        virtual ~NonConstDescribable() {}

        virtual void Describe(std::ostream &os, const DescribeOptions &) {
            os << mutil::class_name_str(*this);
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const Describable &obj) {
        DescribeOptions options;
        options.verbose = false;
        obj.Describe(os, options);
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os,
                                    NonConstDescribable &obj) {
        DescribeOptions options;
        options.verbose = false;
        obj.Describe(os, options);
        return os;
    }

    // Append `indent' spaces after each newline.
    // Example:
    //   IndentingOStream os1(std::cout, 2);
    //   IndentingOStream os2(os1, 2);
    //   std::cout << "begin1\nhello" << std::endl << "world\nend1" << std::endl;
    //   os1 << "begin2\nhello" << std::endl << "world\nend2" << std::endl;
    //   os2 << "begin3\nhello" << std::endl << "world\nend3" << std::endl;
    // Output:
    // begin1
    // hello
    // world
    // end1
    // begin2
    //   hello
    //   world
    //   end2
    //   begin3
    //     hello
    //     world
    //     end3
    class IndentingOStream : virtual private std::streambuf, public std::ostream {
    public:
        IndentingOStream(std::ostream &dest, int indent)
                : std::ostream(this), _dest(dest.rdbuf()), _is_at_start_of_line(false), _indent(indent, ' ') {}

    protected:
        int overflow(int ch) override {
            if (_is_at_start_of_line && ch != '\n') {
                _dest->sputn(_indent.data(), _indent.size());
            }
            _is_at_start_of_line = (ch == '\n');
            return _dest->sputc(ch);
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(IndentingOStream);

        std::streambuf *_dest;
        bool _is_at_start_of_line;
        std::string _indent;
    };

} // namespace melon
