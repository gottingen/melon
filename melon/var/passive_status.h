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

#include <melon/var/variable.h>
#include <melon/var/reducer.h>

namespace melon::var {

    // Display a updated-by-need value. This is done by passing in an user callback
    // which is called to produce the value.
    // Example:
    //   int print_number(void* arg) {
    //      ...
    //      return 5;
    //   }
    //
    //   // number1 : 5
    //   melon::var::PassiveStatus<int> status1("number1", print_number, arg);
    //
    //   // foo_number2 : 5
    //   melon::var::PassiveStatus<int> status2("Foo", "number2", print_number, arg);
    template<typename Tp>
    class PassiveStatus : public Variable {
    public:
        typedef Tp value_type;
        typedef detail::ReducerSampler<PassiveStatus, Tp, detail::AddTo<Tp>,
                detail::MinusFrom<Tp> > sampler_type;

        struct PlaceHolderOp {
            void operator()(Tp &, const Tp &) const {}
        };

        static const bool ADDITIVE = (std::is_integral<Tp>::value ||
                                      std::is_floating_point<Tp>::value ||
                                      is_vector<Tp>::value);

        class SeriesSampler : public detail::Sampler {
        public:
            typedef typename std::conditional<
                    ADDITIVE, detail::AddTo<Tp>, PlaceHolderOp>::type Op;

            explicit SeriesSampler(PassiveStatus *owner)
                    : _owner(owner), _vector_names(NULL), _series(Op()) {}

            ~SeriesSampler() {
                delete _vector_names;
            }

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, _vector_names); }

            void set_vector_names(const std::string &names) {
                if (_vector_names == NULL) {
                    _vector_names = new std::string;
                }
                *_vector_names = names;
            }

        private:
            PassiveStatus *_owner;
            std::string *_vector_names;
            detail::Series<Tp, Op> _series;
        };

    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of PassiveStatus.
        PassiveStatus(const std::string_view &name,
                      Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(NULL), _series_sampler(NULL) {
            expose(name);
        }

        PassiveStatus(const std::string_view &prefix,
                      const std::string_view &name,
                      Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(NULL), _series_sampler(NULL) {
            expose_as(prefix, name);
        }

        PassiveStatus(Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(NULL), _series_sampler(NULL) {
        }

        ~PassiveStatus() {
            hide();
            if (_sampler) {
                _sampler->destroy();
                _sampler = NULL;
            }
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = NULL;
            }
        }

        int set_vector_names(const std::string &names) {
            if (_series_sampler) {
                _series_sampler->set_vector_names(names);
                return 0;
            }
            return -1;
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }


        Tp get_value() const {
            return (_getfn ? _getfn(_arg) : Tp());
        }

        sampler_type *get_sampler() {
            if (NULL == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        detail::AddTo<Tp> op() const { return detail::AddTo<Tp>(); }

        detail::MinusFrom<Tp> inv_op() const { return detail::MinusFrom<Tp>(); }

        int describe_series(std::ostream &os, const SeriesOptions &options) const override {
            if (_series_sampler == NULL) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

        Tp reset() {
            CHECK(false) << "PassiveStatus::reset() should never be called, abort";
            abort();
        }

    protected:
        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        DisplayFilter display_filter) override {
            const int rc = Variable::expose_impl(prefix, name, display_filter);
            if (ADDITIVE &&
                rc == 0 &&
                _series_sampler == NULL &&
                FLAGS_save_series) {
                _series_sampler = new SeriesSampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        Tp (*_getfn)(void *);

        void *_arg;
        sampler_type *_sampler;
        SeriesSampler *_series_sampler;
    };

    // ccover g++ may complain about ADDITIVE is undefined unless it's
    // explicitly declared here.
    template<typename Tp> const bool PassiveStatus<Tp>::ADDITIVE;

    // Specialize std::string for using std::ostream& as a more friendly
    // interface for user's callback.
    template<>
    class PassiveStatus<std::string> : public Variable {
    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of PassiveStatus.
        PassiveStatus(const std::string_view &name,
                      void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {
            expose(name);
        }

        PassiveStatus(const std::string_view &prefix,
                      const std::string_view &name,
                      void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {
            expose_as(prefix, name);
        }

        PassiveStatus(void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {}

        ~PassiveStatus() {
            hide();
        }

        void describe(std::ostream &os, bool quote_string) const override {
            if (quote_string) {
                if (_print) {
                    os << '"';
                    _print(os, _arg);
                    os << '"';
                } else {
                    os << "\"null\"";
                }
            } else {
                if (_print) {
                    _print(os, _arg);
                } else {
                    os << "null";
                }
            }
        }

    private:
        void (*_print)(std::ostream &, void *);

        void *_arg;
    };

    template<typename Tp>
    class BasicPassiveStatus : public PassiveStatus<Tp> {
    public:
        BasicPassiveStatus(const std::string_view &name,
                           Tp (*getfn)(void *), void *arg)
                : PassiveStatus<Tp>(name, getfn, arg) {}

        BasicPassiveStatus(const std::string_view &prefix,
                           const std::string_view &name,
                           Tp (*getfn)(void *), void *arg)
                : PassiveStatus<Tp>(prefix, name, getfn, arg) {}

        BasicPassiveStatus(Tp (*getfn)(void *), void *arg)
                : PassiveStatus<Tp>(getfn, arg) {}
    };

    template<>
    class BasicPassiveStatus<std::string> : public PassiveStatus<std::string> {
    public:
        BasicPassiveStatus(const std::string_view &name,
                           void (*print)(std::ostream &, void *), void *arg)
                : PassiveStatus<std::string>(name, print, arg) {}

        BasicPassiveStatus(const std::string_view &prefix,
                           const std::string_view &name,
                           void (*print)(std::ostream &, void *), void *arg)
                : PassiveStatus<std::string>(prefix, name, print, arg) {}

        BasicPassiveStatus(void (*print)(std::ostream &, void *), void *arg)
                : PassiveStatus<std::string>(print, arg) {}
    };


}  // namespace melon::var
