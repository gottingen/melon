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



#include <unistd.h>                    // write, _exit
#include <gflags/gflags.h>
#include <melon/utility/macros.h>
#include <melon/rpc/reloadable_flags.h>

namespace melon {

bool PassValidate(const char*, bool) {
    return true;
}
bool PassValidate(const char*, int32_t) {
    return true;
}
bool PassValidate(const char*, uint32_t) {
    return true;
}
bool PassValidate(const char*, int64_t) {
    return true;
}
bool PassValidate(const char*, uint64_t) {
    return true;
}
bool PassValidate(const char*, double) {
    return true;
}

bool PositiveInteger(const char*, int32_t val) {
    return val > 0;
}
bool PositiveInteger(const char*, uint32_t val) {
    return val > 0;
}
bool PositiveInteger(const char*, int64_t val) {
    return val > 0;
}
bool PositiveInteger(const char*, uint64_t val) {
    return val > 0;
}

bool NonNegativeInteger(const char*, int32_t val) {
    return val >= 0;
}
bool NonNegativeInteger(const char*, int64_t val) {
    return val >= 0;
}

template <typename T>
static bool RegisterFlagValidatorOrDieImpl(
    const T* flag, bool (*validate_fn)(const char*, T val)) {
    if (google::RegisterFlagValidator(flag, validate_fn)) {
        return true;
    }
    // Error printed by gflags does not have newline. Add one to it.
    char newline = '\n';
    mutil::ignore_result(write(2, &newline, 1));
    _exit(1);
}

bool RegisterFlagValidatorOrDie(const bool* flag,
                                bool (*validate_fn)(const char*, bool)) {
    return RegisterFlagValidatorOrDieImpl(flag, validate_fn);
}
bool RegisterFlagValidatorOrDie(const int32_t* flag,
                                bool (*validate_fn)(const char*, int32_t)) {
    return RegisterFlagValidatorOrDieImpl(flag, validate_fn);
}
bool RegisterFlagValidatorOrDie(const int64_t* flag,
                                bool (*validate_fn)(const char*, int64_t)) {
    return RegisterFlagValidatorOrDieImpl(flag, validate_fn);
}
bool RegisterFlagValidatorOrDie(const uint64_t* flag,
                                bool (*validate_fn)(const char*, uint64_t)) {
    return RegisterFlagValidatorOrDieImpl(flag, validate_fn);
}
bool RegisterFlagValidatorOrDie(const double* flag,
                                bool (*validate_fn)(const char*, double)) {
    return RegisterFlagValidatorOrDieImpl(flag, validate_fn);
}

} // namespace melon
