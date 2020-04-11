//
//
#include <abel/config/usage.h>

#include <string>

#include <abel/config/internal/usage.h>
#include <abel/thread/mutex.h>


namespace abel {

    namespace flags_internal {
        namespace {
            ABEL_CONST_INIT abel::mutex usage_message_guard(abel::kConstInit);
            ABEL_CONST_INIT std::string *program_usage_message
                    ABEL_GUARDED_BY(usage_message_guard) = nullptr;
        }  // namespace
    }  // namespace flags_internal

// --------------------------------------------------------------------
// Sets the "usage" message to be used by help reporting routines.
    void set_program_usage_message(abel::string_view new_usage_message) {
        abel::mutex_lock l(&flags_internal::usage_message_guard);

        if (flags_internal::program_usage_message != nullptr) {
            ABEL_INTERNAL_LOG(FATAL, "set_program_usage_message() called twice.");
            std::exit(1);
        }

        flags_internal::program_usage_message = new std::string(new_usage_message);
    }

// --------------------------------------------------------------------
// Returns the usage message set by set_program_usage_message().
// Note: We able to return string_view here only because calling
// set_program_usage_message twice is prohibited.
    abel::string_view program_usage_message() {
        abel::mutex_lock l(&flags_internal::usage_message_guard);

        return flags_internal::program_usage_message != nullptr
               ? abel::string_view(*flags_internal::program_usage_message)
               : "Warning: set_program_usage_message() never called";
    }


}  // namespace abel
