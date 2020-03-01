//
//

#ifndef ABEL_FLAGS_INTERNAL_FLAG_H_
#define ABEL_FLAGS_INTERNAL_FLAG_H_

#include <atomic>
#include <cstring>

#include <abel/threading/thread_annotations.h>
#include <abel/flags/internal/commandlineflag.h>
#include <abel/flags/internal/registry.h>
#include <abel/memory/memory.h>
#include <abel/strings/str_cat.h>
#include <abel/synchronization/mutex.h>

namespace abel {

    namespace flags_internal {

        constexpr int64_t AtomicInit() { return 0xababababababababll; }

        template<typename T>
        class Flag;

        template<typename T>
        class FlagState : public flags_internal::FlagStateInterface {
        public:
            FlagState(Flag<T> *flag, T &&cur, bool modified, bool on_command_line,
                      int64_t counter)
                    : flag_(flag),
                      cur_value_(std::move(cur)),
                      modified_(modified),
                      on_command_line_(on_command_line),
                      counter_(counter) {}

            ~FlagState() override = default;

        private:
            friend class Flag<T>;

            // Restores the flag to the saved state.
            void Restore() const override;

            // Flag and saved flag data.
            Flag<T> *flag_;
            T cur_value_;
            bool modified_;
            bool on_command_line_;
            int64_t counter_;
        };

// This is help argument for abel::Flag encapsulating the string literal pointer
// or pointer to function generating it as well as enum descriminating two
// cases.
        using HelpGenFunc = std::string (*)();

        union FlagHelpSrc {
            constexpr explicit FlagHelpSrc(const char *help_msg) : literal(help_msg) {}

            constexpr explicit FlagHelpSrc(HelpGenFunc help_gen) : gen_func(help_gen) {}

            const char *literal;
            HelpGenFunc gen_func;
        };

        enum class FlagHelpSrcKind : int8_t {
            kLiteral, kGenFunc
        };

        struct HelpInitArg {
            FlagHelpSrc source;
            FlagHelpSrcKind kind;
        };

        extern const char kStrippedFlagHelp[];

// HelpConstexprWrap is used by struct AbelFlagHelpGenFor##name generated by
// ABEL_FLAG macro. It is only used to silence the compiler in the case where
// help message expression is not constexpr and does not have type const char*.
// If help message expression is indeed constexpr const char* HelpConstexprWrap
// is just a trivial identity function.
        template<typename T>
        const char *HelpConstexprWrap(const T &) {
            return nullptr;
        }

        constexpr const char *HelpConstexprWrap(const char *p) { return p; }

        constexpr const char *HelpConstexprWrap(char *p) { return p; }

// These two HelpArg overloads allows us to select at compile time one of two
// way to pass Help argument to abel::Flag. We'll be passing
// AbelFlagHelpGenFor##name as T and integer 0 as a single argument to prefer
// first overload if possible. If T::Const is evaluatable on constexpr
// context (see non template int parameter below) we'll choose first overload.
// In this case the help message expression is immediately evaluated and is used
// to construct the abel::Flag. No additionl code is generated by ABEL_FLAG.
// Otherwise SFINAE kicks in and first overload is dropped from the
// consideration, in which case the second overload will be used. The second
// overload does not attempt to evaluate the help message expression
// immediately and instead delays the evaluation by returing the function
// pointer (&T::NonConst) genering the help message when necessary. This is
// evaluatable in constexpr context, but the cost is an extra function being
// generated in the ABEL_FLAG code.
        template<typename T, int = (T::Const(), 1)>
        constexpr flags_internal::HelpInitArg HelpArg(int) {
            return {flags_internal::FlagHelpSrc(T::Const()),
                    flags_internal::FlagHelpSrcKind::kLiteral};
        }

        template<typename T>
        constexpr flags_internal::HelpInitArg HelpArg(char) {
            return {flags_internal::FlagHelpSrc(&T::NonConst),
                    flags_internal::FlagHelpSrcKind::kGenFunc};
        }

// Signature for the function generating the initial flag value (usually
// based on default value supplied in flag's definition)
        using FlagDfltGenFunc = void *(*)();

        union FlagDefaultSrc {
            constexpr explicit FlagDefaultSrc(FlagDfltGenFunc gen_func_arg)
                    : gen_func(gen_func_arg) {}

            void *dynamic_value;
            FlagDfltGenFunc gen_func;
        };

        enum class FlagDefaultSrcKind : int8_t {
            kDynamicValue, kGenFunc
        };

// Signature for the mutation callback used by watched Flags
// The callback is noexcept.
// TODO(rogeeff): add noexcept after C++17 support is added.
        using FlagCallback = void (*)();

        struct DynValueDeleter {
            void operator()(void *ptr) const { Delete(op, ptr); }

            const FlagOpFn op;
        };

// The class encapsulates the Flag's data and safe access to it.
        class FlagImpl {
        public:
            constexpr FlagImpl(const char *name, const char *filename,
                               const flags_internal::FlagOpFn op,
                               const flags_internal::FlagMarshallingOpFn marshalling_op,
                               const HelpInitArg help,
                               const flags_internal::FlagDfltGenFunc default_value_gen)
                    : name_(name),
                      filename_(filename),
                      op_(op),
                      marshalling_op_(marshalling_op),
                      help_(help.source),
                      help_source_kind_(help.kind),
                      def_kind_(flags_internal::FlagDefaultSrcKind::kGenFunc),
                      default_src_(default_value_gen),
                      data_guard_{} {}

            // Forces destruction of the Flag's data.
            void Destroy();

            // Constant access methods
            abel::string_view Name() const;

            std::string Filename() const;

            std::string Help() const;

            bool IsModified() const ABEL_LOCKS_EXCLUDED(*DataGuard());

            bool IsSpecifiedOnCommandLine() const ABEL_LOCKS_EXCLUDED(*DataGuard());

            std::string DefaultValue() const ABEL_LOCKS_EXCLUDED(*DataGuard());

            std::string CurrentValue() const ABEL_LOCKS_EXCLUDED(*DataGuard());

            void Read(void *dst, const flags_internal::FlagOpFn dst_op) const
            ABEL_LOCKS_EXCLUDED(*DataGuard());

            // Attempts to parse supplied `value` std::string. If parsing is successful, then
            // it replaces `dst` with the new value.
            bool TryParse(void **dst, abel::string_view value, std::string *err) const
            ABEL_EXCLUSIVE_LOCKS_REQUIRED(*DataGuard());

            template<typename T>
            bool AtomicGet(T *v) const {
                const int64_t r = atomic_.load(std::memory_order_acquire);
                if (r != flags_internal::AtomicInit()) {
                    std::memcpy(v, &r, sizeof(T));
                    return true;
                }

                return false;
            }

            // Mutating access methods
            void Write(const void *src, const flags_internal::FlagOpFn src_op)
            ABEL_LOCKS_EXCLUDED(*DataGuard());

            bool SetFromString(abel::string_view value, FlagSettingMode set_mode,
                               ValueSource source, std::string *err)
            ABEL_LOCKS_EXCLUDED(*DataGuard());

            // If possible, updates copy of the Flag's value that is stored in an
            // atomic word.
            void StoreAtomic() ABEL_EXCLUSIVE_LOCKS_REQUIRED(*DataGuard());

            // Interfaces to operate on callbacks.
            void SetCallback(const flags_internal::FlagCallback mutation_callback)
            ABEL_LOCKS_EXCLUDED(*DataGuard());

            void InvokeCallback() const ABEL_EXCLUSIVE_LOCKS_REQUIRED(*DataGuard());

            // Interfaces to save/restore mutable flag data
            template<typename T>
            std::unique_ptr<flags_internal::FlagStateInterface> SaveState(
                    Flag<T> *flag) const ABEL_LOCKS_EXCLUDED(*DataGuard()) {
                T &&cur_value = flag->Get();
                abel::mutex_lock l(DataGuard());

                return abel::make_unique<flags_internal::FlagState<T>>(
                        flag, std::move(cur_value), modified_, on_command_line_, counter_);
            }

            bool RestoreState(const void *value, bool modified, bool on_command_line,
                              int64_t counter) ABEL_LOCKS_EXCLUDED(*DataGuard());

            // Value validation interfaces.
            void CheckDefaultValueParsingRoundtrip() const
            ABEL_LOCKS_EXCLUDED(*DataGuard());

            bool ValidateInputValue(abel::string_view value) const
            ABEL_LOCKS_EXCLUDED(*DataGuard());

        private:
            // Lazy initialization of the Flag's data.
            void Init();

            // Ensures that the lazily initialized data is initialized,
            // and returns pointer to the mutex guarding flags data.
            abel::mutex *DataGuard() const ABEL_LOCK_RETURNED((abel::mutex *) &data_guard_);

            // Returns heap allocated value of type T initialized with default value.
            std::unique_ptr<void, DynValueDeleter> MakeInitValue() const
            ABEL_EXCLUSIVE_LOCKS_REQUIRED(*DataGuard());

            // Immutable Flag's data.
            // Constant configuration for a particular flag.
            const char *const name_;      // Flags name passed to ABEL_FLAG as second arg.
            const char *const filename_;  // The file name where ABEL_FLAG resides.
            const FlagOpFn op_;           // Type-specific handler.
            const FlagMarshallingOpFn marshalling_op_;  // Marshalling ops handler.
            const FlagHelpSrc help_;  // Help message literal or function to generate it.
            // Indicates if help message was supplied as literal or generator func.
            const FlagHelpSrcKind help_source_kind_;

            // Indicates that the Flag state is initialized.
            std::atomic<bool> inited_{false};
            // Mutable Flag state (guarded by data_guard_).
            // Additional bool to protect against multiple concurrent constructions
            // of `data_guard_`.
            bool is_data_guard_inited_ = false;
            // Has flag value been modified?
            bool modified_ ABEL_GUARDED_BY(*DataGuard()) = false;
            // Specified on command line.
            bool on_command_line_ ABEL_GUARDED_BY(*DataGuard()) = false;
            // If def_kind_ == kDynamicValue, default_src_ holds a dynamically allocated
            // value.
            FlagDefaultSrcKind def_kind_ ABEL_GUARDED_BY(*DataGuard());
            // Either a pointer to the function generating the default value based on the
            // value specified in ABEL_FLAG or pointer to the dynamically set default
            // value via SetCommandLineOptionWithMode. def_kind_ is used to distinguish
            // these two cases.
            FlagDefaultSrc default_src_ ABEL_GUARDED_BY(*DataGuard());
            // Lazily initialized pointer to current value
            void *cur_ ABEL_GUARDED_BY(*DataGuard()) = nullptr;
            // Mutation counter
            int64_t counter_ ABEL_GUARDED_BY(*DataGuard()) = 0;
            // For some types, a copy of the current value is kept in an atomically
            // accessible field.
            std::atomic<int64_t> atomic_{flags_internal::AtomicInit()};

            struct CallbackData {
                FlagCallback func;
                abel::mutex guard;  // Guard for concurrent callback invocations.
            };
            CallbackData *callback_data_ ABEL_GUARDED_BY(*DataGuard()) = nullptr;
            // This is reserved space for an abel::mutex to guard flag data. It will be
            // initialized in FlagImpl::Init via placement new.
            // We can't use "abel::mutex data_guard_", since this class is not literal.
            // We do not want to use "abel::mutex* data_guard_", since this would require
            // heap allocation during initialization, which is both slows program startup
            // and can fail. Using reserved space + placement new allows us to avoid both
            // problems.
            alignas(abel::mutex) mutable char data_guard_[sizeof(abel::mutex)];
        };

// This is "unspecified" implementation of abel::Flag<T> type.
        template<typename T>
        class Flag final : public flags_internal::CommandLineFlag {
        public:
            constexpr Flag(const char *name, const char *filename,
                           const flags_internal::FlagMarshallingOpFn marshalling_op,
                           const flags_internal::HelpInitArg help,
                           const flags_internal::FlagDfltGenFunc default_value_gen)
                    : impl_(name, filename, &flags_internal::FlagOps<T>, marshalling_op, help,
                            default_value_gen) {}

            T Get() const {
                // See implementation notes in CommandLineFlag::Get().
                union U {
                    T value;

                    U() {}

                    ~U() { value.~T(); }
                };
                U u;

                impl_.Read(&u.value, &flags_internal::FlagOps<T>);
                return std::move(u.value);
            }

            bool AtomicGet(T *v) const { return impl_.AtomicGet(v); }

            void Set(const T &v) { impl_.Write(&v, &flags_internal::FlagOps<T>); }

            void SetCallback(const flags_internal::FlagCallback mutation_callback) {
                impl_.SetCallback(mutation_callback);
            }

            // CommandLineFlag interface
            abel::string_view Name() const override { return impl_.Name(); }

            std::string Filename() const override { return impl_.Filename(); }

            abel::string_view Typename() const override { return abel::string_view(""); }

            std::string Help() const override { return impl_.Help(); }

            bool IsModified() const override { return impl_.IsModified(); }

            bool IsSpecifiedOnCommandLine() const override {
                return impl_.IsSpecifiedOnCommandLine();
            }

            std::string DefaultValue() const override { return impl_.DefaultValue(); }

            std::string CurrentValue() const override { return impl_.CurrentValue(); }

            bool ValidateInputValue(abel::string_view value) const override {
                return impl_.ValidateInputValue(value);
            }

            // Interfaces to save and restore flags to/from persistent state.
            // Returns current flag state or nullptr if flag does not support
            // saving and restoring a state.
            std::unique_ptr<flags_internal::FlagStateInterface> SaveState() override {
                return impl_.SaveState(this);
            }

            // Restores the flag state to the supplied state object. If there is
            // nothing to restore returns false. Otherwise returns true.
            bool RestoreState(const flags_internal::FlagState<T> &flag_state) {
                return impl_.RestoreState(&flag_state.cur_value_, flag_state.modified_,
                                          flag_state.on_command_line_, flag_state.counter_);
            }

            bool SetFromString(abel::string_view value,
                               flags_internal::FlagSettingMode set_mode,
                               flags_internal::ValueSource source,
                               std::string *error) override {
                return impl_.SetFromString(value, set_mode, source, error);
            }

            void CheckDefaultValueParsingRoundtrip() const override {
                impl_.CheckDefaultValueParsingRoundtrip();
            }

        private:
            friend class FlagState<T>;

            void Destroy() override { impl_.Destroy(); }

            void Read(void *dst) const override {
                impl_.Read(dst, &flags_internal::FlagOps<T>);
            }

            flags_internal::FlagOpFn TypeId() const override {
                return &flags_internal::FlagOps<T>;
            }

            // Flag's data
            FlagImpl impl_;
        };

        template<typename T>
        ABEL_FORCE_INLINE void FlagState<T>::Restore() const {
            if (flag_->RestoreState(*this)) {
                ABEL_INTERNAL_LOG(INFO,
                                  abel::string_cat("Restore saved value of ", flag_->Name(),
                                                   " to: ", flag_->CurrentValue()));
            }
        }

// This class facilitates Flag object registration and tail expression-based
// flag definition, for example:
// ABEL_FLAG(int, foo, 42, "Foo help").OnUpdate(NotifyFooWatcher);
        template<typename T, bool do_register>
        class FlagRegistrar {
        public:
            explicit FlagRegistrar(Flag<T> *flag) : flag_(flag) {
                if (do_register) flags_internal::RegisterCommandLineFlag(flag_);
            }

            FlagRegistrar &OnUpdate(flags_internal::FlagCallback cb) &&{
                flag_->SetCallback(cb);
                return *this;
            }

            // Make the registrar "die" gracefully as a bool on a line where registration
            // happens. Registrar objects are intended to live only as temporary.
            operator bool() const { return true; }  // NOLINT

        private:
            Flag<T> *flag_;  // Flag being registered (not owned).
        };

// This struct and corresponding overload to MakeDefaultValue are used to
// facilitate usage of {} as default value in ABEL_FLAG macro.
        struct EmptyBraces {
        };

        template<typename T>
        T *MakeFromDefaultValue(T t) {
            return new T(std::move(t));
        }

        template<typename T>
        T *MakeFromDefaultValue(EmptyBraces) {
            return new T;
        }

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_FLAG_H_
