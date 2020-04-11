//

#include <abel/config/usage_config.h>

#include <iostream>
#include <memory>

#include <abel/base/profile.h>
#include <abel/config/internal/path_util.h>
#include <abel/config/internal/program_name.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/strip.h>
#include <abel/thread/mutex.h>

extern "C" {

// Additional report of fatal usage error message before we std::exit. Error is
// fatal if is_fatal argument to report_usage_error is true.
ABEL_WEAK void abel_report_fatal_usage_error(abel::string_view) {}

}  // extern "C"

namespace abel {

    namespace flags_internal {

        namespace {

// --------------------------------------------------------------------
// Returns true if flags defined in the filename should be reported with
// -helpshort flag.

            bool ContainsHelpshortFlags(abel::string_view filename) {
                // By default we only want flags in binary's main. We expect the main
                // routine to reside in <program>.cc or <program>-main.cc or
                // <program>_main.cc, where the <program> is the name of the binary.
                auto suffix = flags_internal::base_name(filename);
                if (!abel::consume_prefix(&suffix,
                                          flags_internal::ShortProgramInvocationName()))
                    return false;
                return abel::starts_with(suffix, ".") || abel::starts_with(suffix, "-main.") ||
                       abel::starts_with(suffix, "_main.");
            }

// --------------------------------------------------------------------
// Returns true if flags defined in the filename should be reported with
// -helppackage flag.

            bool ContainsHelppackageFlags(abel::string_view filename) {
                // TODO(rogeeff): implement properly when registry is available.
                return ContainsHelpshortFlags(filename);
            }

// --------------------------------------------------------------------
// Generates program version information into supplied output.

            std::string VersionString() {
                std::string version_str(flags_internal::ShortProgramInvocationName());

                version_str += "\n";

#if !defined(NDEBUG)
                version_str += "Debug build (NDEBUG not #defined)\n";
#endif

                return version_str;
            }

// --------------------------------------------------------------------
// Normalizes the filename specific to the build system/filesystem used.

            std::string NormalizeFilename(abel::string_view filename) {
                // Skip any leading slashes
                auto pos = filename.find_first_not_of("\\/");
                if (pos == abel::string_view::npos) return "";

                filename.remove_prefix(pos);
                return std::string(filename);
            }

// --------------------------------------------------------------------

            ABEL_CONST_INIT abel::mutex custom_usage_config_guard(abel::kConstInit);
            ABEL_CONST_INIT flags_usage_config *custom_usage_config
                    ABEL_GUARDED_BY(custom_usage_config_guard) = nullptr;

        }  // namespace

        flags_usage_config get_usage_config() {
            abel::mutex_lock l(&custom_usage_config_guard);

            if (custom_usage_config) return *custom_usage_config;

            flags_usage_config default_config;
            default_config.contains_helpshort_flags = &ContainsHelpshortFlags;
            default_config.contains_help_flags = &ContainsHelppackageFlags;
            default_config.contains_helppackage_flags = &ContainsHelppackageFlags;
            default_config.version_string = &VersionString;
            default_config.normalize_filename = &NormalizeFilename;

            return default_config;
        }

        void report_usage_error(abel::string_view msg, bool is_fatal) {
            std::cerr << "ERROR: " << msg << std::endl;

            if (is_fatal) {
                abel_report_fatal_usage_error(msg);
            }
        }

    }  // namespace flags_internal

    void set_flags_usage_config(flags_usage_config usage_config) {
        abel::mutex_lock l(&flags_internal::custom_usage_config_guard);

        if (!usage_config.contains_helpshort_flags)
            usage_config.contains_helpshort_flags =
                    flags_internal::ContainsHelpshortFlags;

        if (!usage_config.contains_help_flags)
            usage_config.contains_help_flags = flags_internal::ContainsHelppackageFlags;

        if (!usage_config.contains_helppackage_flags)
            usage_config.contains_helppackage_flags =
                    flags_internal::ContainsHelppackageFlags;

        if (!usage_config.version_string)
            usage_config.version_string = flags_internal::VersionString;

        if (!usage_config.normalize_filename)
            usage_config.normalize_filename = flags_internal::NormalizeFilename;

        if (flags_internal::custom_usage_config)
            *flags_internal::custom_usage_config = usage_config;
        else
            flags_internal::custom_usage_config = new flags_usage_config(usage_config);
    }


}  // namespace abel
