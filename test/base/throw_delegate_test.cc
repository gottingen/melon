//

#include <abel/base/internal/throw_delegate.h>

#include <functional>
#include <new>
#include <stdexcept>

#include <abel/base/profile.h>
#include <gtest/gtest.h>

namespace {

using abel::base_internal::ThrowStdLogicError;
using abel::base_internal::ThrowStdInvalidArgument;
using abel::base_internal::ThrowStdDomainError;
using abel::base_internal::ThrowStdLengthError;
using abel::base_internal::ThrowStdOutOfRange;
using abel::base_internal::ThrowStdRuntimeError;
using abel::base_internal::ThrowStdRangeError;
using abel::base_internal::ThrowStdOverflowError;
using abel::base_internal::ThrowStdUnderflowError;
using abel::base_internal::ThrowStdBadFunctionCall;
using abel::base_internal::ThrowStdBadAlloc;

constexpr const char *what_arg = "The quick brown fox jumps over the lazy dog";

template<typename E>
void ExpectThrowChar (void (*f) (const char *)) {
#ifdef ABEL_HAVE_EXCEPTIONS
    try {
        f(what_arg);
        FAIL() << "Didn't throw";
    } catch (const E &e) {
        EXPECT_STREQ(e.what(), what_arg);
    }
#else
    EXPECT_DEATH_IF_SUPPORTED(f(what_arg), what_arg);
#endif
}

template<typename E>
void ExpectThrowString (void (*f) (const std::string &)) {
#ifdef ABEL_HAVE_EXCEPTIONS
    try {
        f(what_arg);
        FAIL() << "Didn't throw";
    } catch (const E &e) {
        EXPECT_STREQ(e.what(), what_arg);
    }
#else
    EXPECT_DEATH_IF_SUPPORTED(f(what_arg), what_arg);
#endif
}

template<typename E>
void ExpectThrowNoWhat (void (*f) ()) {
#ifdef ABEL_HAVE_EXCEPTIONS
    try {
        f();
        FAIL() << "Didn't throw";
    } catch (const E &e) {
    }
#else
    EXPECT_DEATH_IF_SUPPORTED(f(), "");
#endif
}

TEST(ThrowHelper, Test) {
    // Not using EXPECT_THROW because we want to check the .what() message too.
    ExpectThrowChar<std::logic_error>(ThrowStdLogicError);
    ExpectThrowChar<std::invalid_argument>(ThrowStdInvalidArgument);
    ExpectThrowChar<std::domain_error>(ThrowStdDomainError);
    ExpectThrowChar<std::length_error>(ThrowStdLengthError);
    ExpectThrowChar<std::out_of_range>(ThrowStdOutOfRange);
    ExpectThrowChar<std::runtime_error>(ThrowStdRuntimeError);
    ExpectThrowChar<std::range_error>(ThrowStdRangeError);
    ExpectThrowChar<std::overflow_error>(ThrowStdOverflowError);
    ExpectThrowChar<std::underflow_error>(ThrowStdUnderflowError);

    ExpectThrowString<std::logic_error>(ThrowStdLogicError);
    ExpectThrowString<std::invalid_argument>(ThrowStdInvalidArgument);
    ExpectThrowString<std::domain_error>(ThrowStdDomainError);
    ExpectThrowString<std::length_error>(ThrowStdLengthError);
    ExpectThrowString<std::out_of_range>(ThrowStdOutOfRange);
    ExpectThrowString<std::runtime_error>(ThrowStdRuntimeError);
    ExpectThrowString<std::range_error>(ThrowStdRangeError);
    ExpectThrowString<std::overflow_error>(ThrowStdOverflowError);
    ExpectThrowString<std::underflow_error>(ThrowStdUnderflowError);

    ExpectThrowNoWhat<std::bad_function_call>(ThrowStdBadFunctionCall);
    ExpectThrowNoWhat<std::bad_alloc>(ThrowStdBadAlloc);
}

}  // namespace
