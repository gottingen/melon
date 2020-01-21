//

#ifndef ABEL_TESTING_INTERNAL_TIME_UTIL_H_
#define ABEL_TESTING_INTERNAL_TIME_UTIL_H_

#include <string>

#include <abel/time/time.h>

namespace abel {

namespace time_internal {

// Loads the named timezone, but dies on any failure.
abel::time_zone load_time_zone(const std::string& name);

}  // namespace time_internal

}  // namespace abel

#endif  // ABEL_TESTING_INTERNAL_TIME_UTIL_H_
