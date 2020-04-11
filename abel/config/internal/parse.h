//
//

#ifndef ABEL_FLAGS_INTERNAL_PARSE_H_
#define ABEL_FLAGS_INTERNAL_PARSE_H_

#include <string>
#include <vector>

#include <abel/config/declare.h>

ABEL_DECLARE_FLAG(std::vector<std::string>, flagfile);
ABEL_DECLARE_FLAG(std::vector<std::string>, fromenv);
ABEL_DECLARE_FLAG(std::vector<std::string>, tryfromenv);
ABEL_DECLARE_FLAG(std::vector<std::string>, undefok);

namespace abel {

    namespace flags_internal {

        enum class argv_list_action {
            kRemoveParsedArgs, kKeepParsedArgs
        };
        enum class usage_flags_action {
            kHandleUsage, kIgnoreUsage
        };
        enum class on_undefined_flag {
            kIgnoreUndefined,
            kReportUndefined,
            kAbortIfUndefined
        };

        std::vector<char *> parse_command_line_impl(int argc, char *argv[],
                                                 argv_list_action arg_list_act,
                                                 usage_flags_action usage_flag_act,
                                                 on_undefined_flag on_undef_flag);

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_PARSE_H_
