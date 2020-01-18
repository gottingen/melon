//

#ifndef ABEL_FLAGS_USAGE_H_
#define ABEL_FLAGS_USAGE_H_

#include <abel/strings/string_view.h>

// --------------------------------------------------------------------
// Usage reporting interfaces

namespace abel {
ABEL_NAMESPACE_BEGIN

// Sets the "usage" message to be used by help reporting routines.
// For example:
//  abel::SetProgramUsageMessage(
//      abel::string_cat("This program does nothing.  Sample usage:\n", argv[0],
//                   " <uselessarg1> <uselessarg2>"));
// Do not include commandline flags in the usage: we do that for you!
// Note: Calling SetProgramUsageMessage twice will trigger a call to std::exit.
void SetProgramUsageMessage(abel::string_view new_usage_message);

// Returns the usage message set by SetProgramUsageMessage().
abel::string_view ProgramUsageMessage();

ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_FLAGS_USAGE_H_
