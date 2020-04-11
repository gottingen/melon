//
//

#include <abel/config/internal/registry.h>

#include <abel/thread/dynamic_annotations.h>
#include <abel/log/raw_logging.h>
#include <abel/config/config.h>
#include <abel/config/usage_config.h>
#include <abel/strings/str_cat.h>
#include <abel/asl/string_view.h>
#include <abel/thread/mutex.h>

// --------------------------------------------------------------------
// flag_registry implementation
//    A flag_registry holds all flag objects indexed
//    by their names so that if you know a flag's name you can access or
//    set it.

namespace abel {

    namespace flags_internal {

// --------------------------------------------------------------------
// flag_registry
//    A flag_registry singleton object holds all flag objects indexed
//    by their names so that if you know a flag's name (as a C
//    string), you can access or set it.  If the function is named
//    FooLocked(), you must own the registry lock before calling
//    the function; otherwise, you should *not* hold the lock, and
//    the function will acquire it itself if needed.
// --------------------------------------------------------------------

        class flag_registry {
        public:
            flag_registry() = default;

            ~flag_registry() {
                for (auto &p : flags_) {
                    p.second->Destroy();
                }
            }

            // Store a flag in this registry.  Takes ownership of *flag.
            void register_flag(command_line_flag *flag);

            void lock() ABEL_EXCLUSIVE_LOCK_FUNCTION(lock_) { lock_.lock(); }

            void unlock() ABEL_UNLOCK_FUNCTION(lock_) { lock_.unlock(); }

            // Returns the flag object for the specified name, or nullptr if not found.
            // Will emit a warning if a 'retired' flag is specified.
            command_line_flag *find_flag_locked(abel::string_view name);

            // Returns the retired flag object for the specified name, or nullptr if not
            // found or not retired.  Does not emit a warning.
            command_line_flag *find_retired_flag_locked(abel::string_view name);

            static flag_registry *global_registry();  // returns a singleton registry

        private:
            friend class flag_saver_impl;  // reads all the flags in order to copy thems
            friend void for_each_flag_unlocked(
                    std::function<void(command_line_flag *)> visitor);

            // The map from name to flag, for find_flag_locked().
            using FlagMap = std::map<abel::string_view, command_line_flag *>;
            using FlagIterator = FlagMap::iterator;
            using FlagConstIterator = FlagMap::const_iterator;
            FlagMap flags_;

            abel::mutex lock_;

            // Disallow
            flag_registry(const flag_registry &);

            flag_registry &operator=(const flag_registry &);
        };

        flag_registry *flag_registry::global_registry() {
            static flag_registry *global_registry = new flag_registry;
            return global_registry;
        }

        namespace {

            class flag_registry_lock {
            public:
                explicit flag_registry_lock(flag_registry *fr) : fr_(fr) { fr_->lock(); }

                ~flag_registry_lock() { fr_->unlock(); }

            private:
                flag_registry *const fr_;
            };

        }  // namespace

        void flag_registry::register_flag(command_line_flag *flag) {
            flag_registry_lock registry_lock(this);
            std::pair<FlagIterator, bool> ins =
                    flags_.insert(FlagMap::value_type(flag->Name(), flag));
            if (ins.second == false) {  // means the name was already in the map
                command_line_flag *old_flag = ins.first->second;
                if (flag->IsRetired() != old_flag->IsRetired()) {
                    // All registrations must agree on the 'retired' flag.
                    flags_internal::report_usage_error(
                            abel::string_cat(
                                    "Retired flag '", flag->Name(),
                                    "' was defined normally in file '",
                                    (flag->IsRetired() ? old_flag->Filename() : flag->Filename()),
                                    "'."),
                            true);
                } else if (flag->TypeId() != old_flag->TypeId()) {
                    flags_internal::report_usage_error(
                            abel::string_cat("Flag '", flag->Name(),
                                             "' was defined more than once but with "
                                             "differing types. Defined in files '",
                                             old_flag->Filename(), "' and '", flag->Filename(),
                                             "' with types '", old_flag->Typename(), "' and '",
                                             flag->Typename(), "', respectively."),
                            true);
                } else if (old_flag->IsRetired()) {
                    // Retired definitions are idempotent. Just keep the old one.
                    flag->Destroy();
                    return;
                } else if (old_flag->Filename() != flag->Filename()) {
                    flags_internal::report_usage_error(
                            abel::string_cat("Flag '", flag->Name(),
                                             "' was defined more than once (in files '",
                                             old_flag->Filename(), "' and '", flag->Filename(),
                                             "')."),
                            true);
                } else {
                    flags_internal::report_usage_error(
                            abel::string_cat(
                                    "Something wrong with flag '", flag->Name(), "' in file '",
                                    flag->Filename(), "'. One possibility: file '", flag->Filename(),
                                    "' is being linked both statically and dynamically into this "
                                    "executable. e.g. some files listed as srcs to a test and also "
                                    "listed as srcs of some shared lib deps of the same test."),
                            true);
                }
                // All cases above are fatal, except for the retired flags.
                std::exit(1);
            }
        }

        command_line_flag *flag_registry::find_flag_locked(abel::string_view name) {
            FlagConstIterator i = flags_.find(name);
            if (i == flags_.end()) {
                return nullptr;
            }

            if (i->second->IsRetired()) {
                flags_internal::report_usage_error(
                        abel::string_cat("Accessing retired flag '", name, "'"), false);
            }

            return i->second;
        }

        command_line_flag *flag_registry::find_retired_flag_locked(abel::string_view name) {
            FlagConstIterator i = flags_.find(name);
            if (i == flags_.end() || !i->second->IsRetired()) {
                return nullptr;
            }

            return i->second;
        }

// --------------------------------------------------------------------
// flag_saver
// flag_saver_impl
//    This class stores the states of all flags at construct time,
//    and restores all flags to that state at destruct time.
//    Its major implementation challenge is that it never modifies
//    pointers in the 'main' registry, so global FLAG_* vars always
//    point to the right place.
// --------------------------------------------------------------------

        class flag_saver_impl {
        public:
            flag_saver_impl() = default;

            flag_saver_impl(const flag_saver_impl &) = delete;

            void operator=(const flag_saver_impl &) = delete;

            // Saves the flag states from the flag registry into this object.
            // It's an error to call this more than once.
            void SaveFromRegistry() {
                assert(backup_registry_.empty());  // call only once!
                flags_internal::for_each_flag([&](flags_internal::command_line_flag *flag) {
                    if (auto flag_state = flag->SaveState()) {
                        backup_registry_.emplace_back(std::move(flag_state));
                    }
                });
            }

            // Restores the saved flag states into the flag registry.
            void restore_to_registry() {
                for (const auto &flag_state : backup_registry_) {
                    flag_state->Restore();
                }
            }

        private:
            std::vector<std::unique_ptr<flags_internal::flag_state_interface>>
                    backup_registry_;
        };

        flag_saver::flag_saver() : impl_(new flag_saver_impl) { impl_->SaveFromRegistry(); }

        void flag_saver::ignore() {
            delete impl_;
            impl_ = nullptr;
        }

        flag_saver::~flag_saver() {
            if (!impl_) return;

            impl_->restore_to_registry();
            delete impl_;
        }

// --------------------------------------------------------------------

        command_line_flag *find_command_line_flag(abel::string_view name) {
            if (name.empty()) return nullptr;
            flag_registry *const registry = flag_registry::global_registry();
            flag_registry_lock frl(registry);

            return registry->find_flag_locked(name);
        }

        command_line_flag *find_retired_flag(abel::string_view name) {
            flag_registry *const registry = flag_registry::global_registry();
            flag_registry_lock frl(registry);

            return registry->find_retired_flag_locked(name);
        }

// --------------------------------------------------------------------

        void for_each_flag_unlocked(std::function<void(command_line_flag *)> visitor) {
            flag_registry *const registry = flag_registry::global_registry();
            for (flag_registry::FlagConstIterator i = registry->flags_.begin();
                 i != registry->flags_.end(); ++i) {
                visitor(i->second);
            }
        }

        void for_each_flag(std::function<void(command_line_flag *)> visitor) {
            flag_registry *const registry = flag_registry::global_registry();
            flag_registry_lock frl(registry);
            for_each_flag_unlocked(visitor);
        }

// --------------------------------------------------------------------

        bool register_command_line_flag(command_line_flag *flag) {
            flag_registry::global_registry()->register_flag(flag);
            return true;
        }

// --------------------------------------------------------------------

        namespace {

            class retired_flag_obj final : public flags_internal::command_line_flag {
            public:
                constexpr retired_flag_obj(const char *name, flag_op_fn ops)
                        : name_(name), op_(ops) {}

            private:
                void Destroy() override {
                    // Values are heap allocated for Retired Flags.
                    delete this;
                }

                abel::string_view Name() const override { return name_; }

                std::string Filename() const override { return "RETIRED"; }

                abel::string_view Typename() const override { return ""; }

                flags_internal::flag_op_fn TypeId() const override { return op_; }

                std::string Help() const override { return ""; }

                bool IsRetired() const override { return true; }

                bool IsModified() const override { return false; }

                bool IsSpecifiedOnCommandLine() const override { return false; }

                std::string DefaultValue() const override { return ""; }

                std::string CurrentValue() const override { return ""; }

                // Any input is valid
                bool ValidateInputValue(abel::string_view) const override { return true; }

                std::unique_ptr<flags_internal::flag_state_interface> SaveState() override {
                    return nullptr;
                }

                bool SetFromString(abel::string_view, flags_internal::FlagSettingMode,
                                   flags_internal::ValueSource, std::string *) override {
                    return false;
                }

                void CheckDefaultValueParsingRoundtrip() const override {}

                void Read(void *) const override {}

                // Data members
                const char *const name_;
                const flag_op_fn op_;
            };

        }  // namespace

        bool Retire(const char *name, flag_op_fn ops) {
            auto *flag = new flags_internal::retired_flag_obj(name, ops);
            flag_registry::global_registry()->register_flag(flag);
            return true;
        }

// --------------------------------------------------------------------

        bool is_retired_flag(abel::string_view name, bool *type_is_bool) {
            assert(!name.empty());
            command_line_flag *flag = flags_internal::find_retired_flag(name);
            if (flag == nullptr) {
                return false;
            }
            assert(type_is_bool);
            *type_is_bool = flag->IsOfType<bool>();
            return true;
        }

    }  // namespace flags_internal

}  // namespace abel
