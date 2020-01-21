//

#include <abel/system/scoped_set_env.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdlib>

#include <abel/base/internal/raw_logging.h>

namespace abel {


namespace {

#ifdef _WIN32
const int kMaxEnvVarValueSize = 1024;
#endif

void SetEnvVar(const char* name, const char* value) {
#ifdef _WIN32
  SetEnvironmentVariableA(name, value);
#else
  if (value == nullptr) {
    ::unsetenv(name);
  } else {
    ::setenv(name, value, 1);
  }
#endif
}

}  // namespace

scoped_set_env::scoped_set_env(const char* var_name, const char* new_value)
    : var_name_(var_name), was_unset_(false) {
#ifdef _WIN32
  char buf[kMaxEnvVarValueSize];
  auto get_res = GetEnvironmentVariableA(var_name_.c_str(), buf, sizeof(buf));
  ABEL_INTERNAL_CHECK(get_res < sizeof(buf), "value exceeds buffer size");

  if (get_res == 0) {
    was_unset_ = (GetLastError() == ERROR_ENVVAR_NOT_FOUND);
  } else {
    old_value_.assign(buf, get_res);
  }

  SetEnvironmentVariableA(var_name_.c_str(), new_value);
#else
  const char* val = ::getenv(var_name_.c_str());
  if (val == nullptr) {
    was_unset_ = true;
  } else {
    old_value_ = val;
  }
#endif

  SetEnvVar(var_name_.c_str(), new_value);
}

scoped_set_env::~scoped_set_env() {
  SetEnvVar(var_name_.c_str(), was_unset_ ? nullptr : old_value_.c_str());
}


}  // namespace abel
