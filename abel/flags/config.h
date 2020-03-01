//

#ifndef ABEL_FLAGS_CONFIG_H_
#define ABEL_FLAGS_CONFIG_H_

// Determine if we should strip string literals from the Flag objects.
// By default we strip string literals on mobile platforms.
#if !defined(ABEL_FLAGS_STRIP_NAMES)

#if defined(__ANDROID__)
#define ABEL_FLAGS_STRIP_NAMES 1

#elif defined(__APPLE__)

#include <TargetConditionals.h>

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define ABEL_FLAGS_STRIP_NAMES 1
#elif defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED
#define ABEL_FLAGS_STRIP_NAMES 1
#endif  // TARGET_OS_*
#endif

#endif  // !defined(ABEL_FLAGS_STRIP_NAMES)

#if !defined(ABEL_FLAGS_STRIP_NAMES)
// If ABEL_FLAGS_STRIP_NAMES wasn't set on the command line or above,
// the default is not to strip.
#define ABEL_FLAGS_STRIP_NAMES 0
#endif

#if !defined(ABEL_FLAGS_STRIP_HELP)
// By default, if we strip names, we also strip help.
#define ABEL_FLAGS_STRIP_HELP ABEL_FLAGS_STRIP_NAMES
#endif

#endif  // ABEL_FLAGS_CONFIG_H_
