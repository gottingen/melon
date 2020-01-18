//
//

#ifndef ABEL_BASE_INTERNAL_SCOPED_SET_ENV_H_
#define ABEL_BASE_INTERNAL_SCOPED_SET_ENV_H_

#include <string>

#include <abel/base/profile.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {

class ScopedSetEnv {
 public:
  ScopedSetEnv(const char* var_name, const char* new_value);
  ~ScopedSetEnv();

 private:
  std::string var_name_;
  std::string old_value_;

  // True if the environment variable was initially not set.
  bool was_unset_;
};

}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_SCOPED_SET_ENV_H_
