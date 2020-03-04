//

#include <abel/stats/random/seed/seed_gen_exception.h>

#include <iostream>

#include <abel/base/profile.h>

namespace abel {


    static constexpr const char kExceptionMessage[] =
            "Failed generating seed-material for URBG.";

    SeedGenException::~SeedGenException() = default;

    const char *SeedGenException::what() const noexcept {
        return kExceptionMessage;
    }

    namespace random_internal {

        void ThrowSeedGenException() {
#ifdef ABEL_HAVE_EXCEPTIONS
            throw abel::SeedGenException();
#else
            std::cerr << kExceptionMessage << std::endl;
            std::terminate();
#endif
        }

    }  // namespace random_internal

}  // namespace abel
