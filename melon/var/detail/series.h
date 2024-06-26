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

#include <math.h>                       // round
#include <ostream>
#include <melon/utility/scoped_lock.h>           // MELON_SCOPED_LOCK
#include <melon/utility/type_traits.h>
#include <melon/var/vector.h>
#include <melon/var/detail/call_op_returning_void.h>
#include <melon/utility/string_splitter.h>

namespace melon::var::detail {

        template<typename T, typename Op, typename Enabler = void>
        struct DivideOnAddition {
            static void inplace_divide(T & /*obj*/, const Op &, int /*number*/) {
                // do nothing
            }
        };

        template<typename T, typename Op>
        struct ProbablyAddtition {
            ProbablyAddtition(const Op &op) {
                T res(32);
                call_op_returning_void(op, res, T(64));
                _ok = (res == T(96));  // works for integral/floating point.
            }

            operator bool() const { return _ok; }

        private:
            bool _ok;
        };

        template<typename T, typename Op>
        struct DivideOnAddition<T, Op, typename mutil::enable_if<
                mutil::is_integral<T>::value>::type> {
            static void inplace_divide(T &obj, const Op &op, int number) {
                static ProbablyAddtition<T, Op> probably_add(op);
                if (probably_add) {
                    obj = (T) round(obj / (double) number);
                }
            }
        };

        template<typename T, typename Op>
        struct DivideOnAddition<T, Op, typename mutil::enable_if<
                mutil::is_floating_point<T>::value>::type> {
            static void inplace_divide(T &obj, const Op &op, int number) {
                static ProbablyAddtition<T, Op> probably_add(op);
                if (probably_add) {
                    obj /= number;
                }
            }
        };

        template<typename T, size_t N, typename Op>
        struct DivideOnAddition<Vector < T, N>, Op, typename mutil::enable_if<
                mutil::is_integral<T>::value>::type> {
        static void inplace_divide(Vector <T, N> &obj, const Op &op, int number) {
            static ProbablyAddtition<Vector<T, N>, Op> probably_add(op);
            if (probably_add) {
                for (size_t i = 0; i < N; ++i) {
                    obj[i] = (T) round(obj[i] / (double) number);
                }
            }
        }
    };

    template<typename T, size_t N, typename Op>
    struct DivideOnAddition<Vector<T, N>, Op, typename mutil::enable_if<
            mutil::is_floating_point<T>::value>::type> {
        static void inplace_divide(Vector<T, N> &obj, const Op &op, int number) {
            static ProbablyAddtition <Vector<T, N>, Op> probably_add(op);
            if (probably_add) {
                obj /= number;
            }
        }
    };

    template<typename T, typename Op>
    class SeriesBase {
    public:
        explicit SeriesBase(const Op &op)
                : _op(op), _nsecond(0), _nminute(0), _nhour(0), _nday(0) {
            pthread_mutex_init(&_mutex, NULL);
        }

        ~SeriesBase() {
            pthread_mutex_destroy(&_mutex);
        }

        void append(const T &value) {
            MELON_SCOPED_LOCK(_mutex);
            return append_second(value, _op);
        }

    private:
        void append_second(const T &value, const Op &op);

        void append_minute(const T &value, const Op &op);

        void append_hour(const T &value, const Op &op);

        void append_day(const T &value);

        struct Data {
        public:
            Data() {
                // is_pod does not work for gcc 3.4
                if (mutil::is_integral<T>::value ||
                    mutil::is_floating_point<T>::value) {
                    memset(static_cast<void *>(_array), 0, sizeof(_array));
                }
            }

            T &second(int index) { return _array[index]; }

            const T &second(int index) const { return _array[index]; }

            T &minute(int index) { return _array[60 + index]; }

            const T &minute(int index) const { return _array[60 + index]; }

            T &hour(int index) { return _array[120 + index]; }

            const T &hour(int index) const { return _array[120 + index]; }

            T &day(int index) { return _array[144 + index]; }

            const T &day(int index) const { return _array[144 + index]; }

        private:
            T _array[60 + 60 + 24 + 30];
        };

    protected:
        Op _op;
        mutable pthread_mutex_t _mutex;
        char _nsecond;
        char _nminute;
        char _nhour;
        char _nday;
        Data _data;
    };

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_second(const T &value, const Op &op) {
        _data.second(_nsecond) = value;
        ++_nsecond;
        if (_nsecond >= 60) {
            _nsecond = 0;
            T tmp = _data.second(0);
            for (int i = 1; i < 60; ++i) {
                call_op_returning_void(op, tmp, _data.second(i));
            }
            DivideOnAddition<T, Op>::inplace_divide(tmp, op, 60);
            append_minute(tmp, op);
        }
    }

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_minute(const T &value, const Op &op) {
        _data.minute(_nminute) = value;
        ++_nminute;
        if (_nminute >= 60) {
            _nminute = 0;
            T tmp = _data.minute(0);
            for (int i = 1; i < 60; ++i) {
                call_op_returning_void(op, tmp, _data.minute(i));
            }
            DivideOnAddition<T, Op>::inplace_divide(tmp, op, 60);
            append_hour(tmp, op);
        }
    }

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_hour(const T &value, const Op &op) {
        _data.hour(_nhour) = value;
        ++_nhour;
        if (_nhour >= 24) {
            _nhour = 0;
            T tmp = _data.hour(0);
            for (int i = 1; i < 24; ++i) {
                call_op_returning_void(op, tmp, _data.hour(i));
            }
            DivideOnAddition<T, Op>::inplace_divide(tmp, op, 24);
            append_day(tmp);
        }
    }

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_day(const T &value) {
        _data.day(_nday) = value;
        ++_nday;
        if (_nday >= 30) {
            _nday = 0;
        }
    }

    template<typename T, typename Op>
    class Series : public SeriesBase<T, Op> {
        typedef SeriesBase<T, Op> Base;
    public:
        explicit Series(const Op &op) : Base(op) {}

        void describe(std::ostream &os, const std::string *vector_names) const;
    };

    template<typename T, size_t N, typename Op>
    class Series<Vector<T, N>, Op> : public SeriesBase<Vector<T, N>, Op> {
        typedef SeriesBase<Vector<T, N>, Op> Base;
    public:
        explicit Series(const Op &op) : Base(op) {}

        void describe(std::ostream &os, const std::string *vector_names) const;
    };

    template<typename T, typename Op>
    void Series<T, Op>::describe(std::ostream &os,
                                 const std::string *vector_names) const {
        CHECK(vector_names == NULL);
        pthread_mutex_lock(&this->_mutex);
        const int second_begin = this->_nsecond;
        const int minute_begin = this->_nminute;
        const int hour_begin = this->_nhour;
        const int day_begin = this->_nday;
        // NOTE: we don't save _data which may be inconsistent sometimes, but
        // this output is generally for "peeking the trend" and does not need
        // to exactly accurate.
        pthread_mutex_unlock(&this->_mutex);
        int c = 0;
        os << "{\"label\":\"trend\",\"data\":[";
        for (int i = 0; i < 30; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.day((i + day_begin) % 30) << ']';
        }
        for (int i = 0; i < 24; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.hour((i + hour_begin) % 24) << ']';
        }
        for (int i = 0; i < 60; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.minute((i + minute_begin) % 60) << ']';
        }
        for (int i = 0; i < 60; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.second((i + second_begin) % 60) << ']';
        }
        os << "]}";
    }

    template<typename T, size_t N, typename Op>
    void Series<Vector<T, N>, Op>::describe(std::ostream &os,
                                            const std::string *vector_names) const {
        pthread_mutex_lock(&this->_mutex);
        const int second_begin = this->_nsecond;
        const int minute_begin = this->_nminute;
        const int hour_begin = this->_nhour;
        const int day_begin = this->_nday;
        // NOTE: we don't save _data which may be inconsistent sometimes, but
        // this output is generally for "peeking the trend" and does not need
        // to exactly accurate.
        pthread_mutex_unlock(&this->_mutex);

        mutil::StringSplitter sp(vector_names ? vector_names->c_str() : "", ',');
        os << '[';
        for (size_t j = 0; j < N; ++j) {
            if (j) {
                os << ',';
            }
            int c = 0;
            os << "{\"label\":\"";
            if (sp) {
                os << mutil::StringPiece(sp.field(), sp.length());
                ++sp;
            } else {
                os << "Vector[" << j << ']';
            }
            os << "\",\"data\":[";
            for (int i = 0; i < 30; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.day((i + day_begin) % 30)[j] << ']';
            }
            for (int i = 0; i < 24; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.hour((i + hour_begin) % 24)[j] << ']';
            }
            for (int i = 0; i < 60; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.minute((i + minute_begin) % 60)[j] << ']';
            }
            for (int i = 0; i < 60; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.second((i + second_begin) % 60)[j] << ']';
            }
            os << "]}";
        }
        os << ']';
    }

}  // namespace melon::var::detail
