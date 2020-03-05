//

#include <abel/stats/random/seed/seed_gen_exception.h>

#include <iostream>

#include <abel/base/profile.h>

namespace abel {


    static constexpr const char kExceptionMessage[] =
            "Failed generating seed-material for URBG.";

    seed_gen_exception::~seed_gen_exception() = default;

    const char *seed_gen_exception::what() const noexcept {
        return kExceptionMessage;
    }

    namespace random_internal {

        void throw_seed_gen_exception() {
#ifdef ABEL_HAVE_EXCEPTIONS
            throw abel::seed_gen_exception();
#else
            std::cerr << kExceptionMessage << std::endl;
            std::terminate();
#endif
        }

    }  // namespace random_internal

}  // namespace abel
