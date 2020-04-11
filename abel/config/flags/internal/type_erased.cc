//
//

#include <abel/config/flags/internal/type_erased.h>

#include <abel/log/raw_logging.h>
#include <abel/config/flags/config.h>
#include <abel/config/flags/usage_config.h>
#include <abel/strings/str_cat.h>

namespace abel {

    namespace flags_internal {

        bool GetCommandLineOption(abel::string_view name, std::string *value) {
            if (name.empty()) return false;
            assert(value);

            command_line_flag *flag = flags_internal::find_command_line_flag(name);
            if (flag == nullptr || flag->IsRetired()) {
                return false;
            }

            *value = flag->CurrentValue();
            return true;
        }

        bool SetCommandLineOption(abel::string_view name, abel::string_view value) {
            return SetCommandLineOptionWithMode(name, value,
                                                flags_internal::SET_FLAGS_VALUE);
        }

        bool SetCommandLineOptionWithMode(abel::string_view name,
                                          abel::string_view value,
                                          FlagSettingMode set_mode) {
            command_line_flag *flag = flags_internal::find_command_line_flag(name);

            if (!flag || flag->IsRetired()) return false;

            std::string error;
            if (!flag->SetFromString(value, set_mode, kProgrammaticChange, &error)) {
                // Errors here are all of the form: the provided name was a recognized
                // flag, but the value was invalid (bad type, or validation failed).
                flags_internal::report_usage_error(error, false);
                return false;
            }

            return true;
        }

// --------------------------------------------------------------------

        bool IsValidFlagValue(abel::string_view name, abel::string_view value) {
            command_line_flag *flag = flags_internal::find_command_line_flag(name);

            return flag != nullptr &&
                   (flag->IsRetired() || flag->ValidateInputValue(value));
        }

// --------------------------------------------------------------------

        bool SpecifiedOnCommandLine(abel::string_view name) {
            command_line_flag *flag = flags_internal::find_command_line_flag(name);
            if (flag != nullptr && !flag->IsRetired()) {
                return flag->IsSpecifiedOnCommandLine();
            }
            return false;
        }

    }  // namespace flags_internal

}  // namespace abel
