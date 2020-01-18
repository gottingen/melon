//

#include <abel/types/bad_any_cast.h>

#ifndef ABEL_USES_STD_ANY

#include <cstdlib>

#include <abel/base/profile.h>
#include <abel/base/internal/raw_logging.h>

namespace abel {
ABEL_NAMESPACE_BEGIN

bad_any_cast::~bad_any_cast() = default;

const char* bad_any_cast::what() const noexcept { return "Bad any cast"; }

namespace any_internal {

void ThrowBadAnyCast() {
#ifdef ABEL_HAVE_EXCEPTIONS
  throw bad_any_cast();
#else
  ABEL_RAW_LOG(FATAL, "Bad any cast");
  std::abort();
#endif
}

}  // namespace any_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_USES_STD_ANY
