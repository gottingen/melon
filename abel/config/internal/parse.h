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

        enum class ArgvListAction {
            kRemoveParsedArgs, kKeepParsedArgs
        };
        enum class UsageFlagsAction {
            kHandleUsage, kIgnoreUsage
        };
        enum class OnUndefinedFlag {
            kIgnoreUndefined,
            kReportUndefined,
            kAbortIfUndefined
        };

        std::vector<char *> ParseCommandLineImpl(int argc, char *argv[],
                                                 ArgvListAction arg_list_act,
                                                 UsageFlagsAction usage_flag_act,
                                                 OnUndefinedFlag on_undef_flag);

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_PARSE_H_
