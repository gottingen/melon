//

#include <abel/asl/bad_optional_access.h>

#ifndef ABEL_USES_STD_OPTIONAL

#include <cstdlib>

#include <abel/base/profile.h>
#include <abel/log/raw_logging.h>

namespace abel {


    bad_optional_access::~bad_optional_access() = default;

    const char *bad_optional_access::what() const noexcept {
        return "optional has no value";
    }

    namespace optional_internal {

        void throw_bad_optional_access() {
#ifdef ABEL_HAVE_EXCEPTIONS
            throw bad_optional_access();
#else
            ABEL_RAW_LOG(FATAL, "Bad optional access");
            abort();
#endif
        }

    }  // namespace optional_internal

}  // namespace abel

#endif  // ABEL_USES_STD_OPTIONAL
