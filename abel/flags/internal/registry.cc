//
//

#include <abel/flags/internal/registry.h>

#include <abel/threading/dynamic_annotations.h>
#include <abel/log/raw_logging.h>
#include <abel/flags/config.h>
#include <abel/flags/usage_config.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/string_view.h>
#include <abel/synchronization/mutex.h>

// --------------------------------------------------------------------
// FlagRegistry implementation
//    A FlagRegistry holds all flag objects indexed
//    by their names so that if you know a flag's name you can access or
//    set it.

namespace abel {

namespace flags_internal {

// --------------------------------------------------------------------
// FlagRegistry
//    A FlagRegistry singleton object holds all flag objects indexed
//    by their names so that if you know a flag's name (as a C
//    string), you can access or set it.  If the function is named
//    FooLocked(), you must own the registry lock before calling
//    the function; otherwise, you should *not* hold the lock, and
//    the function will acquire it itself if needed.
// --------------------------------------------------------------------

class FlagRegistry {
 public:
  FlagRegistry() = default;
  ~FlagRegistry() {
    for (auto& p : flags_) {
      p.second->Destroy();
    }
  }

  // Store a flag in this registry.  Takes ownership of *flag.
  void RegisterFlag(CommandLineFlag* flag);

  void lock() ABEL_EXCLUSIVE_LOCK_FUNCTION(lock_) { lock_.lock(); }
  void unlock() ABEL_UNLOCK_FUNCTION(lock_) { lock_.unlock(); }

  // Returns the flag object for the specified name, or nullptr if not found.
  // Will emit a warning if a 'retired' flag is specified.
  CommandLineFlag* FindFlagLocked(abel::string_view name);

  // Returns the retired flag object for the specified name, or nullptr if not
  // found or not retired.  Does not emit a warning.
  CommandLineFlag* FindRetiredFlagLocked(abel::string_view name);

  static FlagRegistry* GlobalRegistry();  // returns a singleton registry

 private:
  friend class FlagSaverImpl;  // reads all the flags in order to copy them
  friend void ForEachFlagUnlocked(
      std::function<void(CommandLineFlag*)> visitor);

  // The map from name to flag, for FindFlagLocked().
  using FlagMap = std::map<abel::string_view, CommandLineFlag*>;
  using FlagIterator = FlagMap::iterator;
  using FlagConstIterator = FlagMap::const_iterator;
  FlagMap flags_;

  abel::mutex lock_;

  // Disallow
  FlagRegistry(const FlagRegistry&);
  FlagRegistry& operator=(const FlagRegistry&);
};

FlagRegistry* FlagRegistry::GlobalRegistry() {
  static FlagRegistry* global_registry = new FlagRegistry;
  return global_registry;
}

namespace {

class FlagRegistryLock {
 public:
  explicit FlagRegistryLock(FlagRegistry* fr) : fr_(fr) { fr_->lock(); }
  ~FlagRegistryLock() { fr_->unlock(); }

 private:
  FlagRegistry* const fr_;
};

}  // namespace

void FlagRegistry::RegisterFlag(CommandLineFlag* flag) {
  FlagRegistryLock registry_lock(this);
  std::pair<FlagIterator, bool> ins =
      flags_.insert(FlagMap::value_type(flag->Name(), flag));
  if (ins.second == false) {  // means the name was already in the map
    CommandLineFlag* old_flag = ins.first->second;
    if (flag->IsRetired() != old_flag->IsRetired()) {
      // All registrations must agree on the 'retired' flag.
      flags_internal::ReportUsageError(
          abel::string_cat(
              "Retired flag '", flag->Name(),
              "' was defined normally in file '",
              (flag->IsRetired() ? old_flag->Filename() : flag->Filename()),
              "'."),
          true);
    } else if (flag->TypeId() != old_flag->TypeId()) {
      flags_internal::ReportUsageError(
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
      flags_internal::ReportUsageError(
          abel::string_cat("Flag '", flag->Name(),
                       "' was defined more than once (in files '",
                       old_flag->Filename(), "' and '", flag->Filename(),
                       "')."),
          true);
    } else {
      flags_internal::ReportUsageError(
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

CommandLineFlag* FlagRegistry::FindFlagLocked(abel::string_view name) {
  FlagConstIterator i = flags_.find(name);
  if (i == flags_.end()) {
    return nullptr;
  }

  if (i->second->IsRetired()) {
    flags_internal::ReportUsageError(
        abel::string_cat("Accessing retired flag '", name, "'"), false);
  }

  return i->second;
}

CommandLineFlag* FlagRegistry::FindRetiredFlagLocked(abel::string_view name) {
  FlagConstIterator i = flags_.find(name);
  if (i == flags_.end() || !i->second->IsRetired()) {
    return nullptr;
  }

  return i->second;
}

// --------------------------------------------------------------------
// FlagSaver
// FlagSaverImpl
//    This class stores the states of all flags at construct time,
//    and restores all flags to that state at destruct time.
//    Its major implementation challenge is that it never modifies
//    pointers in the 'main' registry, so global FLAG_* vars always
//    point to the right place.
// --------------------------------------------------------------------

class FlagSaverImpl {
 public:
  FlagSaverImpl() = default;
  FlagSaverImpl(const FlagSaverImpl&) = delete;
  void operator=(const FlagSaverImpl&) = delete;

  // Saves the flag states from the flag registry into this object.
  // It's an error to call this more than once.
  void SaveFromRegistry() {
    assert(backup_registry_.empty());  // call only once!
    flags_internal::ForEachFlag([&](flags_internal::CommandLineFlag* flag) {
      if (auto flag_state = flag->SaveState()) {
        backup_registry_.emplace_back(std::move(flag_state));
      }
    });
  }

  // Restores the saved flag states into the flag registry.
  void RestoreToRegistry() {
    for (const auto& flag_state : backup_registry_) {
      flag_state->Restore();
    }
  }

 private:
  std::vector<std::unique_ptr<flags_internal::FlagStateInterface>>
      backup_registry_;
};

FlagSaver::FlagSaver() : impl_(new FlagSaverImpl) { impl_->SaveFromRegistry(); }

void FlagSaver::Ignore() {
  delete impl_;
  impl_ = nullptr;
}

FlagSaver::~FlagSaver() {
  if (!impl_) return;

  impl_->RestoreToRegistry();
  delete impl_;
}

// --------------------------------------------------------------------

CommandLineFlag* FindCommandLineFlag(abel::string_view name) {
  if (name.empty()) return nullptr;
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  FlagRegistryLock frl(registry);

  return registry->FindFlagLocked(name);
}

CommandLineFlag* FindRetiredFlag(abel::string_view name) {
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  FlagRegistryLock frl(registry);

  return registry->FindRetiredFlagLocked(name);
}

// --------------------------------------------------------------------

void ForEachFlagUnlocked(std::function<void(CommandLineFlag*)> visitor) {
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  for (FlagRegistry::FlagConstIterator i = registry->flags_.begin();
       i != registry->flags_.end(); ++i) {
    visitor(i->second);
  }
}

void ForEachFlag(std::function<void(CommandLineFlag*)> visitor) {
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  FlagRegistryLock frl(registry);
  ForEachFlagUnlocked(visitor);
}

// --------------------------------------------------------------------

bool RegisterCommandLineFlag(CommandLineFlag* flag) {
  FlagRegistry::GlobalRegistry()->RegisterFlag(flag);
  return true;
}

// --------------------------------------------------------------------

namespace {

class RetiredFlagObj final : public flags_internal::CommandLineFlag {
 public:
  constexpr RetiredFlagObj(const char* name, FlagOpFn ops)
      : name_(name), op_(ops) {}

 private:
  void Destroy() override {
    // Values are heap allocated for Retired Flags.
    delete this;
  }

  abel::string_view Name() const override { return name_; }
  std::string Filename() const override { return "RETIRED"; }
  abel::string_view Typename() const override { return ""; }
  flags_internal::FlagOpFn TypeId() const override { return op_; }
  std::string Help() const override { return ""; }
  bool IsRetired() const override { return true; }
  bool IsModified() const override { return false; }
  bool IsSpecifiedOnCommandLine() const override { return false; }
  std::string DefaultValue() const override { return ""; }
  std::string CurrentValue() const override { return ""; }

  // Any input is valid
  bool ValidateInputValue(abel::string_view) const override { return true; }

  std::unique_ptr<flags_internal::FlagStateInterface> SaveState() override {
    return nullptr;
  }

  bool SetFromString(abel::string_view, flags_internal::FlagSettingMode,
                     flags_internal::ValueSource, std::string*) override {
    return false;
  }

  void CheckDefaultValueParsingRoundtrip() const override {}

  void Read(void*) const override {}

  // Data members
  const char* const name_;
  const FlagOpFn op_;
};

}  // namespace

bool Retire(const char* name, FlagOpFn ops) {
  auto* flag = new flags_internal::RetiredFlagObj(name, ops);
  FlagRegistry::GlobalRegistry()->RegisterFlag(flag);
  return true;
}

// --------------------------------------------------------------------

bool IsRetiredFlag(abel::string_view name, bool* type_is_bool) {
  assert(!name.empty());
  CommandLineFlag* flag = flags_internal::FindRetiredFlag(name);
  if (flag == nullptr) {
    return false;
  }
  assert(type_is_bool);
  *type_is_bool = flag->IsOfType<bool>();
  return true;
}

}  // namespace flags_internal

}  // namespace abel
