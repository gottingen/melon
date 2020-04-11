//

#include <abel/asl/bad_any_cast.h>

#ifndef ABEL_USES_STD_ANY

#include <cstdlib>

#include <abel/base/profile.h>
#include <abel/log/abel_logging.h>

namespace abel {


    bad_any_cast::~bad_any_cast() = default;

    const char *bad_any_cast::what() const noexcept { return "Bad any cast"; }

    namespace any_internal {

        void ThrowBadAnyCast() {
#ifdef ABEL_HAVE_EXCEPTIONS
            throw bad_any_cast();
#else
            ABEL_RAW_CRITICAL("Bad any cast");
            std::abort();
#endif
        }

    }  // namespace any_internal

}  // namespace abel

#endif  // ABEL_USES_STD_ANY
