#ifndef ABEL_STATS_RAND_SEED_SEED_GEN_EXCEPTION_H_
#define ABEL_STATS_RAND_SEED_SEED_GEN_EXCEPTION_H_

#include <exception>
#include <abel/base/profile.h>

namespace abel {


//------------------------------------------------------------------------------
// seed_gen_exception
//------------------------------------------------------------------------------
    class seed_gen_exception : public std::exception {
    public:
        seed_gen_exception() = default;

        ~seed_gen_exception() override;

        const char *what() const noexcept override;
    };

    namespace random_internal {

// throw delegator
        [[noreturn]] void throw_seed_gen_exception();

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_STATS_RAND_SEED_SEED_GEN_EXCEPTION_H_
