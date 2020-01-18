//

#include <abel/base/internal/throw_delegate.h>

#include <cstdlib>
#include <functional>
#include <new>
#include <stdexcept>
#include <abel/base/profile.h>
#include <abel/base/internal/raw_logging.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {

namespace {
template <typename T>
[[noreturn]] void Throw(const T& error) {
#ifdef ABEL_HAVE_EXCEPTIONS
  throw error;
#else
  ABEL_RAW_LOG(FATAL, "%s", error.what());
  std::abort();
#endif
}
}  // namespace

void ThrowStdLogicError(const std::string& what_arg) {
  Throw(std::logic_error(what_arg));
}
void ThrowStdLogicError(const char* what_arg) {
  Throw(std::logic_error(what_arg));
}
void ThrowStdInvalidArgument(const std::string& what_arg) {
  Throw(std::invalid_argument(what_arg));
}
void ThrowStdInvalidArgument(const char* what_arg) {
  Throw(std::invalid_argument(what_arg));
}

void ThrowStdDomainError(const std::string& what_arg) {
  Throw(std::domain_error(what_arg));
}
void ThrowStdDomainError(const char* what_arg) {
  Throw(std::domain_error(what_arg));
}

void ThrowStdLengthError(const std::string& what_arg) {
  Throw(std::length_error(what_arg));
}
void ThrowStdLengthError(const char* what_arg) {
  Throw(std::length_error(what_arg));
}

void ThrowStdOutOfRange(const std::string& what_arg) {
  Throw(std::out_of_range(what_arg));
}
void ThrowStdOutOfRange(const char* what_arg) {
  Throw(std::out_of_range(what_arg));
}

void ThrowStdRuntimeError(const std::string& what_arg) {
  Throw(std::runtime_error(what_arg));
}
void ThrowStdRuntimeError(const char* what_arg) {
  Throw(std::runtime_error(what_arg));
}

void ThrowStdRangeError(const std::string& what_arg) {
  Throw(std::range_error(what_arg));
}
void ThrowStdRangeError(const char* what_arg) {
  Throw(std::range_error(what_arg));
}

void ThrowStdOverflowError(const std::string& what_arg) {
  Throw(std::overflow_error(what_arg));
}
void ThrowStdOverflowError(const char* what_arg) {
  Throw(std::overflow_error(what_arg));
}

void ThrowStdUnderflowError(const std::string& what_arg) {
  Throw(std::underflow_error(what_arg));
}
void ThrowStdUnderflowError(const char* what_arg) {
  Throw(std::underflow_error(what_arg));
}

void ThrowStdBadFunctionCall() { Throw(std::bad_function_call()); }

void ThrowStdBadAlloc() { Throw(std::bad_alloc()); }

}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel
