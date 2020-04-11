//
//

#ifndef ABEL_FLAGS_INTERNAL_REGISTRY_H_
#define ABEL_FLAGS_INTERNAL_REGISTRY_H_

#include <functional>
#include <map>
#include <string>

#include <abel/base/profile.h>
#include <abel/config/flags/internal/commandlineflag.h>

// --------------------------------------------------------------------
// Global flags registry API.

namespace abel {

    namespace flags_internal {

        command_line_flag *find_command_line_flag(abel::string_view name);

        command_line_flag *find_retired_flag(abel::string_view name);

// Executes specified visitor for each non-retired flag in the registry.
// Requires the caller hold the registry lock.
        void ForEachFlagUnlocked(std::function<void(command_line_flag *)> visitor);

// Executes specified visitor for each non-retired flag in the registry. While
// callback are executed, the registry is locked and can't be changed.
        void ForEachFlag(std::function<void(command_line_flag *)> visitor);

//-----------------------------------------------------------------------------

        bool RegisterCommandLineFlag(command_line_flag *);

//-----------------------------------------------------------------------------
// Retired registrations:
//
// Retired flag registrations are treated specially. A 'retired' flag is
// provided only for compatibility with automated invocations that still
// name it.  A 'retired' flag:
//   - is not bound to a C++ FLAGS_ reference.
//   - has a type and a value, but that value is intentionally inaccessible.
//   - does not appear in --help messages.
//   - is fully supported by _all_ flag parsing routines.
//   - consumes args normally, and complains about type mismatches in its
//     argument.
//   - emits a complaint but does not die (e.g. LOG(ERROR)) if it is
//     accessed by name through the flags API for parsing or otherwise.
//
// The registrations for a flag happen in an unspecified order as the
// initializers for the namespace-scope objects of a program are run.
// Any number of weak registrations for a flag can weakly define the flag.
// One non-weak registration will upgrade the flag from weak to non-weak.
// Further weak registrations of a non-weak flag are ignored.
//
// This mechanism is designed to support moving dead flags into a
// 'graveyard' library.  An example migration:
//
//   0: Remove references to this FLAGS_flagname in the C++ codebase.
//   1: Register as 'retired' in old_lib.
//   2: Make old_lib depend on graveyard.
//   3: Add a redundant 'retired' registration to graveyard.
//   4: Remove the old_lib 'retired' registration.
//   5: Eventually delete the graveyard registration entirely.
//

// Retire flag with name "name" and type indicated by ops.
        bool Retire(const char *name, flag_op_fn ops);

// Registered a retired flag with name 'flag_name' and type 'T'.
        template<typename T>
        ABEL_FORCE_INLINE bool RetiredFlag(const char *flag_name) {
            return flags_internal::Retire(flag_name, flags_internal::FlagOps<T>);
        }

// If the flag is retired, returns true and indicates in |*type_is_bool|
// whether the type of the retired flag is a bool.
// Only to be called by code that needs to explicitly ignore retired flags.
        bool is_retired_flag(abel::string_view name, bool *type_is_bool);

//-----------------------------------------------------------------------------
// Saves the states (value, default value, whether the user has set
// the flag, registered validators, etc) of all flags, and restores
// them when the flag_saver is destroyed.
//
// This class is thread-safe.  However, its destructor writes to
// exactly the set of flags that have changed value during its
// lifetime, so concurrent _direct_ access to those flags
// (i.e. FLAGS_foo instead of {Get,Set}CommandLineOption()) is unsafe.

        class flag_saver {
        public:
            flag_saver();

            ~flag_saver();

            flag_saver(const flag_saver &) = delete;

            void operator=(const flag_saver &) = delete;

            // Prevents saver from restoring the saved state of flags.
            void Ignore();

        private:
            class flag_saver_impl *impl_;  // we use pimpl here to keep API steady
        };

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_REGISTRY_H_
