//

#include <abel/container/internal/hashtablez_sampler.h>

#include <abel/base/profile.h>

namespace abel {

    namespace container_internal {

// See hashtablez_sampler.h for details.
        extern "C" ABEL_WEAK bool AbelContainerInternalSampleEverything() {
            return false;
        }

    }  // namespace container_internal

}  // namespace abel
