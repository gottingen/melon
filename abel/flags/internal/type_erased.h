//
//

#ifndef ABEL_FLAGS_INTERNAL_TYPE_ERASED_H_
#define ABEL_FLAGS_INTERNAL_TYPE_ERASED_H_

#include <string>

#include <abel/flags/internal/commandlineflag.h>
#include <abel/flags/internal/registry.h>

// --------------------------------------------------------------------
// Registry interfaces operating on type erased handles.

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace flags_internal {

// If a flag named "name" exists, store its current value in *OUTPUT
// and return true.  Else return false without changing *OUTPUT.
// Thread-safe.
bool GetCommandLineOption(abel::string_view name, std::string* value);

// Set the value of the flag named "name" to value.  If successful,
// returns true.  If not successful (e.g., the flag was not found or
// the value is not a valid value), returns false.
// Thread-safe.
bool SetCommandLineOption(abel::string_view name, abel::string_view value);

bool SetCommandLineOptionWithMode(abel::string_view name,
                                  abel::string_view value,
                                  FlagSettingMode set_mode);

//-----------------------------------------------------------------------------

// Returns true iff all of the following conditions are true:
// (a) "name" names a registered flag
// (b) "value" can be parsed succesfully according to the type of the flag
// (c) parsed value passes any validator associated with the flag
bool IsValidFlagValue(abel::string_view name, abel::string_view value);

//-----------------------------------------------------------------------------

// Returns true iff a flag named "name" was specified on the command line
// (either directly, or via one of --flagfile or --fromenv or --tryfromenv).
//
// Any non-command-line modification of the flag does not affect the
// result of this function.  So for example, if a flag was passed on
// the command line but then reset via SET_FLAGS_DEFAULT, this
// function will still return true.
bool SpecifiedOnCommandLine(abel::string_view name);

//-----------------------------------------------------------------------------

// If a flag with specified "name" exists and has type T, store
// its current value in *dst and return true.  Else return false
// without touching *dst.  T must obey all of the requirements for
// types passed to DEFINE_FLAG.
template <typename T>
ABEL_FORCE_INLINE bool GetByName(abel::string_view name, T* dst) {
  CommandLineFlag* flag = flags_internal::FindCommandLineFlag(name);
  if (!flag) return false;

  if (auto val = flag->Get<T>()) {
    *dst = *val;
    return true;
  }

  return false;
}

}  // namespace flags_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_TYPE_ERASED_H_
