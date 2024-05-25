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



#ifndef MELON_RPC_EXTENSION_INL_H_
#define MELON_RPC_EXTENSION_INL_H_


namespace melon {

    template<typename T>
    Extension<T> *Extension<T>::instance() {
        // NOTE: We don't delete extensions because in principle they can be
        // accessed during exiting, e.g. create a channel to send rpc at exit.
        return mutil::get_leaky_singleton<Extension<T> >();
    }

    template<typename T>
    Extension<T>::Extension() {
        _instance_map.init(29);
    }

    template<typename T>
    Extension<T>::~Extension() {
    }

    template<typename T>
    int Extension<T>::Register(const std::string &name, T *instance) {
        if (nullptr == instance) {
            MLOG(ERROR) << "instance to \"" << name << "\" is nullptr";
            return -1;
        }
        MELON_SCOPED_LOCK(_map_mutex);
        if (_instance_map.seek(name) != nullptr) {
            MLOG(ERROR) << "\"" << name << "\" was registered";
            return -1;
        }
        _instance_map[name] = instance;
        return 0;
    }

    template<typename T>
    int Extension<T>::RegisterOrDie(const std::string &name, T *instance) {
        if (Register(name, instance) == 0) {
            return 0;
        }
        exit(1);
    }

    template<typename T>
    T *Extension<T>::Find(const char *name) {
        if (nullptr == name) {
            return nullptr;
        }
        MELON_SCOPED_LOCK(_map_mutex);
        T **p = _instance_map.seek(name);
        if (p) {
            return *p;
        }
        return nullptr;
    }

    template<typename T>
    void Extension<T>::List(std::ostream &os, char separator) {
        MELON_SCOPED_LOCK(_map_mutex);
        for (typename mutil::CaseIgnoredFlatMap<T *>::iterator
                     it = _instance_map.begin(); it != _instance_map.end(); ++it) {
            // private extensions which is not intended to be seen by users starts
            // with underscore.
            if (it->first.data()[0] != '_') {
                if (it != _instance_map.begin()) {
                    os << separator;
                }
                os << it->first;
            }
        }
    }

} // namespace melon


#endif  // MELON_RPC_EXTENSION_INL_H_
