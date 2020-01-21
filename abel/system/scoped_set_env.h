//
//

#ifndef ABEL_SYSTEM_SCOPED_SET_ENV_H_
#define ABEL_SYSTEM_SCOPED_SET_ENV_H_

#include <string>

#include <abel/base/profile.h>

namespace abel {


class scoped_set_env {
 public:
    scoped_set_env(const char* var_name, const char* new_value);
  ~scoped_set_env();

 private:
  std::string var_name_;
  std::string old_value_;

  // True if the environment variable was initially not set.
  bool was_unset_;
};

}  // namespace abel

#endif  // ABEL_SYSTEM_SCOPED_SET_ENV_H_
