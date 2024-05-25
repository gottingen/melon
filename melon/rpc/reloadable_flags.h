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



#ifndef MELON_RPC_RELOADABLE_FLAGS_H_
#define MELON_RPC_RELOADABLE_FLAGS_H_



#include <stdint.h>

// Register an always-true valiator to a gflag so that the gflag is treated as
// reloadable by melon. If a validator exists, abort the program.
// You should call this macro within global scope. for example:
//
//   DEFINE_int32(foo, 0, "blah blah");
//   MELON_VALIDATE_GFLAG(foo, melon::PassValidate);
//
// This macro does not work for string-flags because they're thread-unsafe to
// modify directly. To emphasize this, you have to write the validator by
// yourself and use google::GetCommandLineOption() to acess the flag.
#define MELON_VALIDATE_GFLAG(flag, validate_fn)                     \
    const int register_FLAGS_ ## flag ## _dummy                         \
                 __attribute__((__unused__)) =                          \
        ::melon::RegisterFlagValidatorOrDie(                       \
            &FLAGS_##flag, (validate_fn))


namespace melon {

extern bool PassValidate(const char*, bool);
extern bool PassValidate(const char*, int32_t);
extern bool PassValidate(const char*, uint32_t);
extern bool PassValidate(const char*, int64_t);
extern bool PassValidate(const char*, uint64_t);
extern bool PassValidate(const char*, double);

extern bool PositiveInteger(const char*, int32_t);
extern bool PositiveInteger(const char*, uint32_t);
extern bool PositiveInteger(const char*, int64_t);
extern bool PositiveInteger(const char*, uint64_t);

extern bool NonNegativeInteger(const char*, int32_t);
extern bool NonNegativeInteger(const char*, int64_t);

extern bool RegisterFlagValidatorOrDie(const bool* flag,
                                  bool (*validate_fn)(const char*, bool));
extern bool RegisterFlagValidatorOrDie(const int32_t* flag,
                                  bool (*validate_fn)(const char*, int32_t));
extern bool RegisterFlagValidatorOrDie(const uint32_t* flag,
                                  bool (*validate_fn)(const char*, uint32_t));
extern bool RegisterFlagValidatorOrDie(const int64_t* flag,
                                  bool (*validate_fn)(const char*, int64_t));
extern bool RegisterFlagValidatorOrDie(const uint64_t* flag,
                                  bool (*validate_fn)(const char*, uint64_t));
extern bool RegisterFlagValidatorOrDie(const double* flag,
                                  bool (*validate_fn)(const char*, double));
} // namespace melon


#endif  // MELON_RPC_RELOADABLE_FLAGS_H_
