//

#include <abel/system/scoped_set_env.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdlib>

#include <abel/log/raw_logging.h>

namespace abel {

#ifdef _WIN32
    const int kMaxEnvVarValueSize = 1024;
#endif

    void set_env_var(const char *name, const char *value) {
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

    scoped_set_env::scoped_set_env(const char *var_name, const char *new_value)
            : _var_name(var_name), _was_unset(false) {
#ifdef _WIN32
        char buf[kMaxEnvVarValueSize];
        auto get_res = GetEnvironmentVariableA(var_name_.c_str(), buf, sizeof(buf));
        ABEL_INTERNAL_CHECK(get_res < sizeof(buf), "value exceeds buffer size");

        if (get_res == 0) {
          _was_unset = (GetLastError() == ERROR_ENVVAR_NOT_FOUND);
        } else {
          _old_value.assign(buf, get_res);
        }

        SetEnvironmentVariableA(var_name_.c_str(), new_value);
#else
        const char *val = ::getenv(_var_name.c_str());
        if (val == nullptr) {
            _was_unset = true;
        } else {
            _old_value = val;
        }
#endif

        set_env_var(_var_name.c_str(), new_value);
    }

    scoped_set_env::~scoped_set_env() {
        set_env_var(_var_name.c_str(), _was_unset ? nullptr : _old_value.c_str());
    }

}  // namespace abel
