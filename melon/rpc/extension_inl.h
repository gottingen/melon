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
