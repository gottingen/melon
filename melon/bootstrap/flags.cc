

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/bootstrap/flags.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include "melon/memory/resident.h"

namespace melon::detail {

    namespace {

        using flags_registry = std::unordered_map<std::string, std::pair<std::string, bool>>;

        flags_registry *get_registry() {
            static melon::resident<flags_registry> registry;
            return registry.get();
        }

    }  // namespace

    void register_flags_overrider(const std::string &name, const std::string &to,
                               bool forcibly) {
        auto &&value = (*get_registry())[name];
        if (!value.first.empty()) {
            MELON_LOG(FATAL)<<"Duplicate override for flag "<<name<<", was ["<<value.first<<"], now ["<<to<<"]";
        }
        value = std::pair(to, forcibly);
    }

    void apply_flags_overrider() {
        for (auto&&[k, v] : *get_registry()) {
            // Make sure the flag name is present.
            auto current = google::GetCommandLineFlagInfoOrDie(k.c_str());
            if (current.is_default || v.second) {
                google::SetCommandLineOption(k.c_str(), v.first.c_str());
                MELON_VLOG(10)<<"Overriding flag ["<<k<<"] with ["<<v.first<<"].";
            }
        }
    }

}  // namespace melon::detail
