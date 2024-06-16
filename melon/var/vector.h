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
#include <gflags/gflags_declare.h>

namespace melon::var {

    DECLARE_bool(quote_vector);

    // Data inside a Vector will be plotted in a same graph.
    template<typename T, size_t N>
    class Vector {
    public:
        static const size_t WIDTH = N;

        Vector() {
            for (size_t i = 0; i < N; ++i) {
                _data[i] = T();
            }
        }

        Vector(const T &initial_value) {
            for (size_t i = 0; i < N; ++i) {
                _data[i] = initial_value;
            }
        }

        void operator+=(const Vector &rhs) {
            for (size_t i = 0; i < N; ++i) {
                _data[i] += rhs._data[i];
            }
        }

        void operator-=(const Vector &rhs) {
            for (size_t i = 0; i < N; ++i) {
                _data[i] -= rhs._data[i];
            }
        }

        template<typename S>
        void operator*=(const S &scalar) {
            for (size_t i = 0; i < N; ++i) {
                _data[i] *= scalar;
            }
        }

        template<typename S>
        void operator/=(const S &scalar) {
            for (size_t i = 0; i < N; ++i) {
                _data[i] /= scalar;
            }
        }

        bool operator==(const Vector &rhs) const {
            for (size_t i = 0; i < N; ++i) {
                if (!(_data[i] == rhs._data[i])) {
                    return false;
                }
            }
            return true;
        }

        bool operator!=(const Vector &rhs) const {
            return !operator==(rhs);
        }

        T &operator[](int index) { return _data[index]; }

        const T &operator[](int index) const { return _data[index]; }

    private:
        T _data[N];
    };

    template<typename T, size_t N>
    std::ostream &operator<<(std::ostream &os, const Vector<T, N> &vec) {
        if (FLAGS_quote_vector) {
            os << '"';
        }
        os << '[';
        if (N != 0) {
            os << vec[0];
            for (size_t i = 1; i < N; ++i) {
                os << ',' << vec[i];
            }
        }
        os << ']';
        if (FLAGS_quote_vector) {
            os << '"';
        }
        return os;
    }

    template<typename T>
    struct is_vector : public std::false_type {
    };

    template<typename T, size_t N>
    struct is_vector<Vector<T, N> > : public std::true_type {
    };

}  // namespace melon::var
