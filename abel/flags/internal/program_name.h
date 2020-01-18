//

#ifndef ABEL_FLAGS_INTERNAL_PROGRAM_NAME_H_
#define ABEL_FLAGS_INTERNAL_PROGRAM_NAME_H_

#include <string>

#include <abel/strings/string_view.h>

// --------------------------------------------------------------------
// Program name

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace flags_internal {

// Returns program invocation name or "UNKNOWN" if `SetProgramInvocationName()`
// is never called. At the moment this is always set to argv[0] as part of
// library initialization.
std::string ProgramInvocationName();

// Returns base name for program invocation name. For example, if
//   ProgramInvocationName() == "a/b/mybinary"
// then
//   ShortProgramInvocationName() == "mybinary"
std::string ShortProgramInvocationName();

// Sets program invocation name to a new value. Should only be called once
// during program initialization, before any threads are spawned.
void SetProgramInvocationName(abel::string_view prog_name_str);

}  // namespace flags_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_PROGRAM_NAME_H_
