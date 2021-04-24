// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_SYSTEM_SCOPED_SET_ENV_H_
#define ABEL_SYSTEM_SCOPED_SET_ENV_H_

#include <string>
#include "abel/base/profile.h"

namespace abel {

void set_env_var(const char *name, const char *value);

ABEL_FORCE_INLINE void set_env_var(const std::string &name, const std::string &value) {
    set_env_var(name.c_str(), value.c_str());
}


class scoped_set_env {
  public:
    scoped_set_env(const char *var_name, const char *new_value);

    ~scoped_set_env();

  private:
    std::string _var_name;
    std::string _old_value;

    // True if the environment variable was initially not set.
    bool _was_unset;
};

}  // namespace abel

#endif  // ABEL_SYSTEM_SCOPED_SET_ENV_H_
