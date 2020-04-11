//
//

#include <abel/config/internal/type_erased.h>

#include <abel/log/abel_logging.h>
#include <abel/config/config.h>
#include <abel/config/usage_config.h>
#include <abel/strings/str_cat.h>

namespace abel {

    namespace flags_internal {

        bool get_command_line_option(abel::string_view name, std::string *value) {
            if (name.empty()) return false;
            assert(value);

            command_line_flag *flag = flags_internal::find_command_line_flag(name);
            if (flag == nullptr || flag->is_retired()) {
                return false;
            }

            *value = flag->current_value();
            return true;
        }

        bool set_command_line_option(abel::string_view name, abel::string_view value) {
            return set_command_line_option_with_mode(name, value,
                                                flags_internal::SET_FLAGS_VALUE);
        }

        bool set_command_line_option_with_mode(abel::string_view name,
                                          abel::string_view value,
                                          flag_setting_mode set_mode) {
            command_line_flag *flag = flags_internal::find_command_line_flag(name);

            if (!flag || flag->is_retired()) return false;

            std::string error;
            if (!flag->set_from_string(value, set_mode, kProgrammaticChange, &error)) {
                // Errors here are all of the form: the provided name was a recognized
                // flag, but the value was invalid (bad type, or validation failed).
                flags_internal::report_usage_error(error, false);
                return false;
            }

            return true;
        }

// --------------------------------------------------------------------

        bool is_valid_flag_value(abel::string_view name, abel::string_view value) {
            command_line_flag *flag = flags_internal::find_command_line_flag(name);

            return flag != nullptr &&
                   (flag->is_retired() || flag->validate_input_value(value));
        }

// --------------------------------------------------------------------

        bool specified_on_command_line(abel::string_view name) {
            command_line_flag *flag = flags_internal::find_command_line_flag(name);
            if (flag != nullptr && !flag->is_retired()) {
                return flag->is_specified_on_command_line();
            }
            return false;
        }

    }  // namespace flags_internal

}  // namespace abel
