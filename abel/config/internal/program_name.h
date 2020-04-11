//

#ifndef ABEL_CONFIG_INTERNAL_PROGRAM_NAME_H_
#define ABEL_CONFIG_INTERNAL_PROGRAM_NAME_H_

#include <string>

#include <abel/asl/string_view.h>

// --------------------------------------------------------------------
// Program name

namespace abel {

    namespace flags_internal {

// Returns program invocation name or "UNKNOWN" if `set_program_invocation_name()`
// is never called. At the moment this is always set to argv[0] as part of
// library initialization.
        std::string program_invocation_name();

// Returns base name for program invocation name. For example, if
//   program_invocation_name() == "a/b/mybinary"
// then
//   short_program_invocation_name() == "mybinary"
        std::string short_program_invocation_name();

// Sets program invocation name to a new value. Should only be called once
// during program initialization, before any threads are spawned.
        void set_program_invocation_name(abel::string_view prog_name_str);

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_CONFIG_INTERNAL_PROGRAM_NAME_H_
