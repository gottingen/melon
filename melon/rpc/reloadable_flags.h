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
