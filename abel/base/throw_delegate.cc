//

#include <abel/base/throw_delegate.h>

#include <cstdlib>
#include <functional>
#include <new>
#include <stdexcept>
#include <abel/base/profile.h>
#include <abel/log/abel_logging.h>

namespace abel {

        namespace {
            template<typename T>
            [[noreturn]] void Throw(const T &error) {
#ifdef ABEL_HAVE_EXCEPTIONS
                throw error;
#else
                ABEL_RAW_CRITICAL("{}", error.what());
                std::abort();
#endif
            }
        }  // namespace

        void throw_std_logic_error(const std::string &what_arg) {
            Throw(std::logic_error(what_arg));
        }

        void throw_std_logic_error(const char *what_arg) {
            Throw(std::logic_error(what_arg));
        }

        void throw_std_invalid_argument(const std::string &what_arg) {
            Throw(std::invalid_argument(what_arg));
        }

        void throw_std_invalid_argument(const char *what_arg) {
            Throw(std::invalid_argument(what_arg));
        }

        void throw_std_domain_error(const std::string &what_arg) {
            Throw(std::domain_error(what_arg));
        }

        void throw_std_domain_error(const char *what_arg) {
            Throw(std::domain_error(what_arg));
        }

        void throw_std_length_error(const std::string &what_arg) {
            Throw(std::length_error(what_arg));
        }

        void throw_std_length_error(const char *what_arg) {
            Throw(std::length_error(what_arg));
        }

        void throw_std_out_of_range(const std::string &what_arg) {
            Throw(std::out_of_range(what_arg));
        }

        void throw_std_out_of_range(const char *what_arg) {
            Throw(std::out_of_range(what_arg));
        }

        void throw_std_runtime_error(const std::string &what_arg) {
            Throw(std::runtime_error(what_arg));
        }

        void throw_std_runtime_error(const char *what_arg) {
            Throw(std::runtime_error(what_arg));
        }

        void throw_std_range_error(const std::string &what_arg) {
            Throw(std::range_error(what_arg));
        }

        void throw_std_range_error(const char *what_arg) {
            Throw(std::range_error(what_arg));
        }

        void throw_std_overflow_error(const std::string &what_arg) {
            Throw(std::overflow_error(what_arg));
        }

        void throw_std_overflow_error(const char *what_arg) {
            Throw(std::overflow_error(what_arg));
        }

        void throw_std_underflow_error(const std::string &what_arg) {
            Throw(std::underflow_error(what_arg));
        }

        void throw_std_underflow_error(const char *what_arg) {
            Throw(std::underflow_error(what_arg));
        }

        void throw_std_bad_function_call() { Throw(std::bad_function_call()); }

        void throw_std_bad_alloc() { Throw(std::bad_alloc()); }


}  // namespace abel
