//
// -----------------------------------------------------------------------------
// File: seed_gen_exception.h
// -----------------------------------------------------------------------------
//
// This header defines an exception class which may be thrown if unpredictable
// events prevent the derivation of suitable seed-material for constructing a
// bit generator conforming to [rand.req.urng] (eg. entropy cannot be read from
// /dev/urandom on a Unix-based system).
//
// Note: if exceptions are disabled, `std::terminate()` is called instead.

#ifndef ABEL_RANDOM_SEED_GEN_EXCEPTION_H_
#define ABEL_RANDOM_SEED_GEN_EXCEPTION_H_

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

#endif  // ABEL_RANDOM_SEED_GEN_EXCEPTION_H_
