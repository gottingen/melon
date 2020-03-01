//

#include <abel/types/bad_variant_access.h>

#ifndef ABEL_USES_STD_VARIANT

#include <cstdlib>
#include <stdexcept>

#include <abel/base/profile.h>
#include <abel/log/raw_logging.h>

namespace abel {


//////////////////////////
// [variant.bad.access] //
//////////////////////////

    bad_variant_access::~bad_variant_access() = default;

    const char *bad_variant_access::what() const noexcept {
        return "Bad variant access";
    }

    namespace variant_internal {

        void throw_bad_variant_access() {
#ifdef ABEL_HAVE_EXCEPTIONS
            throw bad_variant_access();
#else
            ABEL_RAW_LOG(FATAL, "Bad variant access");
            abort();  // TODO(calabrese) Remove once RAW_LOG FATAL is noreturn.
#endif
        }

        void rethrow() {
#ifdef ABEL_HAVE_EXCEPTIONS
            throw;
#else
            ABEL_RAW_LOG(FATAL,
                         "Internal error in abel::variant implementation. Attempted to "
                         "rethrow an exception when building with exceptions disabled.");
            abort();  // TODO(calabrese) Remove once RAW_LOG FATAL is noreturn.
#endif
        }

    }  // namespace variant_internal

}  // namespace abel

#endif  // ABEL_USES_STD_VARIANT
