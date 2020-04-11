//

#include <abel/config/internal/program_name.h>
#include <string>
#include <abel/config/internal/path_util.h>
#include <abel/thread/mutex.h>

namespace abel {

    namespace flags_internal {

        ABEL_CONST_INIT static abel::mutex program_name_guard(abel::kConstInit);
        ABEL_CONST_INIT static std::string *program_name
                ABEL_GUARDED_BY(program_name_guard) = nullptr;

        std::string ProgramInvocationName() {
            abel::mutex_lock l(&program_name_guard);

            return program_name ? *program_name : "UNKNOWN";
        }

        std::string ShortProgramInvocationName() {
            abel::mutex_lock l(&program_name_guard);

            return program_name ? std::string(flags_internal::base_name(*program_name))
                                : "UNKNOWN";
        }

        void SetProgramInvocationName(abel::string_view prog_name_str) {
            abel::mutex_lock l(&program_name_guard);

            if (!program_name)
                program_name = new std::string(prog_name_str);
            else
                program_name->assign(prog_name_str.data(), prog_name_str.size());
        }

    }  // namespace flags_internal

}  // namespace abel
