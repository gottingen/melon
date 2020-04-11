//
//

#include <abel/config/internal/flag.h>

#include <abel/base/profile.h>
#include <abel/config/usage_config.h>
#include <abel/thread/mutex.h>

namespace abel {

    namespace flags_internal {

// The help message indicating that the commandline flag has been
// 'stripped'. It will not show up when doing "-help" and its
// variants. The flag is stripped if ABEL_FLAGS_STRIP_HELP is set to 1
// before including abel/flags/flag.h
        const char kStrippedFlagHelp[] = "\001\002\003\004 (unknown) \004\003\002\001";

        namespace {

// Currently we only validate flag values for user-defined flag types.
            bool ShouldValidateFlagValue(flag_op_fn flag_type_id) {
#define DONT_VALIDATE(T) \
  if (flag_type_id == &flags_internal::flag_ops<T>) return false;
                ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(DONT_VALIDATE)
                DONT_VALIDATE(std::string)
                DONT_VALIDATE(std::vector<std::string>)
#undef DONT_VALIDATE

                return true;
            }

// RAII helper used to temporarily unlock and relock `abel::mutex`.
// This is used when we need to ensure that locks are released while
// invoking user supplied callbacks and then reacquired, since callbacks may
// need to acquire these locks themselves.
            class MutexRelock {
            public:
                explicit MutexRelock(abel::mutex *mu) : mu_(mu) { mu_->unlock(); }

                ~MutexRelock() { mu_->lock(); }

                MutexRelock(const MutexRelock &) = delete;

                MutexRelock &operator=(const MutexRelock &) = delete;

            private:
                abel::mutex *mu_;
            };

// This global lock guards the initialization and destruction of data_guard_,
// which is used to guard the other Flag data.
            ABEL_CONST_INIT static abel::mutex flag_mutex_lifetime_guard(abel::kConstInit);

        }  // namespace

        void FlagImpl::Init() {
            {
                abel::mutex_lock lock(&flag_mutex_lifetime_guard);

                // Must initialize data guard for this flag.
                if (!is_data_guard_inited_) {
                    new(&data_guard_) abel::mutex;
                    is_data_guard_inited_ = true;
                }
            }

            abel::mutex_lock lock(reinterpret_cast<abel::mutex *>(&data_guard_));

            if (cur_ != nullptr) {
                inited_.store(true, std::memory_order_release);
            } else {
                // Need to initialize cur field.
                cur_ = MakeInitValue().release();
                StoreAtomic();
                inited_.store(true, std::memory_order_release);
                InvokeCallback();
            }
        }

// Ensures that the lazily initialized data is initialized,
// and returns pointer to the mutex guarding flags data.
        abel::mutex *FlagImpl::DataGuard() const {
            if (ABEL_UNLIKELY(!inited_.load(std::memory_order_acquire))) {
                const_cast<FlagImpl *>(this)->Init();
            }

            // data_guard_ is initialized.
            return reinterpret_cast<abel::mutex *>(&data_guard_);
        }

        void FlagImpl::Destroy() {
            {
                abel::mutex_lock l(DataGuard());

                // Values are heap allocated for abel Flags.
                if (cur_) remove(op_, cur_);

                // Release the dynamically allocated default value if any.
                if (def_kind_ == FlagDefaultSrcKind::kDynamicValue) {
                    remove(op_, default_src_.dynamic_value);
                }

                // If this flag has an assigned callback, release callback data.
                if (callback_data_) delete callback_data_;
            }

            abel::mutex_lock l(&flag_mutex_lifetime_guard);
            DataGuard()->~mutex();
            is_data_guard_inited_ = false;
        }

        std::unique_ptr<void, DynValueDeleter> FlagImpl::MakeInitValue() const {
            void *res = nullptr;
            if (def_kind_ == FlagDefaultSrcKind::kDynamicValue) {
                res = clone(op_, default_src_.dynamic_value);
            } else {
                res = (*default_src_.gen_func)();
            }
            return {res, DynValueDeleter{op_}};
        }

        abel::string_view FlagImpl::Name() const { return name_; }

        std::string FlagImpl::Filename() const {
            return flags_internal::get_usage_config().normalize_filename(filename_);
        }

        std::string FlagImpl::Help() const {
            return help_source_kind_ == FlagHelpSrcKind::kLiteral ? help_.literal
                                                                  : help_.gen_func();
        }

        bool FlagImpl::is_modified() const {
            abel::mutex_lock l(DataGuard());
            return modified_;
        }

        bool FlagImpl::is_specified_on_command_line() const {
            abel::mutex_lock l(DataGuard());
            return on_command_line_;
        }

        std::string FlagImpl::DefaultValue() const {
            abel::mutex_lock l(DataGuard());

            auto obj = MakeInitValue();
            return unparse(marshalling_op_, obj.get());
        }

        std::string FlagImpl::CurrentValue() const {
            abel::mutex_lock l(DataGuard());

            return unparse(marshalling_op_, cur_);
        }

        void FlagImpl::SetCallback(
                const flags_internal::flag_callback mutation_callback) {
            abel::mutex_lock l(DataGuard());

            if (callback_data_ == nullptr) {
                callback_data_ = new callback_data;
            }
            callback_data_->func = mutation_callback;

            InvokeCallback();
        }

        void FlagImpl::InvokeCallback() const {
            if (!callback_data_) return;

            // Make a copy of the C-style function pointer that we are about to invoke
            // before we release the lock guarding it.
            flag_callback cb = callback_data_->func;

            // If the flag has a mutation callback this function invokes it. While the
            // callback is being invoked the primary flag's mutex is unlocked and it is
            // re-locked back after call to callback is completed. Callback invocation is
            // guarded by flag's secondary mutex instead which prevents concurrent
            // callback invocation. Note that it is possible for other thread to grab the
            // primary lock and update flag's value at any time during the callback
            // invocation. This is by design. Callback can get a value of the flag if
            // necessary, but it might be different from the value initiated the callback
            // and it also can be different by the time the callback invocation is
            // completed. Requires that *primary_lock be held in exclusive mode; it may be
            // released and reacquired by the implementation.
            MutexRelock relock(DataGuard());
            abel::mutex_lock lock(&callback_data_->guard);
            cb();
        }

        bool FlagImpl::restore_state(const void *value, bool modified,
                                    bool on_command_line, int64_t counter) {
            {
                abel::mutex_lock l(DataGuard());

                if (counter_ == counter) return false;
            }

            Write(value, op_);

            {
                abel::mutex_lock l(DataGuard());

                modified_ = modified;
                on_command_line_ = on_command_line;
            }

            return true;
        }

// Attempts to parse supplied `value` string using parsing routine in the `flag`
// argument. If parsing successful, this function replaces the dst with newly
// parsed value. In case if any error is encountered in either step, the error
// message is stored in 'err'
        bool FlagImpl::TryParse(void **dst, abel::string_view value,
                                std::string *err) const {
            auto tentative_value = MakeInitValue();

            std::string parse_err;
            if (!parse(marshalling_op_, value, tentative_value.get(), &parse_err)) {
                abel::string_view err_sep = parse_err.empty() ? "" : "; ";
                *err = abel::string_cat("Illegal value '", value, "' specified for flag '",
                                        Name(), "'", err_sep, parse_err);
                return false;
            }

            void *old_val = *dst;
            *dst = tentative_value.release();
            tentative_value.reset(old_val);

            return true;
        }

        void FlagImpl::Read(void *dst, const flags_internal::flag_op_fn dst_op) const {
            abel::reader_mutex_lock l(DataGuard());

            // `dst_op` is the unmarshaling operation corresponding to the declaration
            // visibile at the call site. `op` is the Flag's defined unmarshalling
            // operation. They must match for this operation to be well-defined.
            if (ABEL_UNLIKELY(dst_op != op_)) {
                ABEL_INTERNAL_LOG(
                        ERROR,
                        abel::string_cat("Flag '", Name(),
                                         "' is defined as one type and declared as another"));
            }
            copy_construct(op_, cur_, dst);
        }

        void FlagImpl::StoreAtomic() {
            size_t data_size = size_of(op_);

            if (data_size <= sizeof(int64_t)) {
                int64_t t = 0;
                std::memcpy(&t, cur_, data_size);
                atomic_.store(t, std::memory_order_release);
            }
        }

        void FlagImpl::Write(const void *src, const flags_internal::flag_op_fn src_op) {
            abel::mutex_lock l(DataGuard());

            // `src_op` is the marshalling operation corresponding to the declaration
            // visible at the call site. `op` is the Flag's defined marshalling operation.
            // They must match for this operation to be well-defined.
            if (ABEL_UNLIKELY(src_op != op_)) {
                ABEL_INTERNAL_LOG(
                        ERROR,
                        abel::string_cat("Flag '", Name(),
                                         "' is defined as one type and declared as another"));
            }

            if (ShouldValidateFlagValue(op_)) {
                void *obj = clone(op_, src);
                std::string ignored_error;
                std::string src_as_str = unparse(marshalling_op_, src);
                if (!parse(marshalling_op_, src_as_str, obj, &ignored_error)) {
                    ABEL_INTERNAL_LOG(ERROR, abel::string_cat("Attempt to set flag '", Name(),
                                                              "' to invalid value ", src_as_str));
                }
                remove(op_, obj);
            }

            modified_ = true;
            counter_++;
            copy(op_, src, cur_);

            StoreAtomic();
            InvokeCallback();
        }

// Sets the value of the flag based on specified string `value`. If the flag
// was successfully set to new value, it returns true. Otherwise, sets `err`
// to indicate the error, leaves the flag unchanged, and returns false. There
// are three ways to set the flag's value:
//  * Update the current flag value
//  * Update the flag's default value
//  * Update the current flag value if it was never set before
// The mode is selected based on 'set_mode' parameter.
        bool FlagImpl::set_from_string(abel::string_view value, flag_setting_mode set_mode,
                                     value_source source, std::string *err) {
            abel::mutex_lock l(DataGuard());

            switch (set_mode) {
                case SET_FLAGS_VALUE: {
                    // set or modify the flag's value
                    if (!TryParse(&cur_, value, err)) return false;
                    modified_ = true;
                    counter_++;
                    StoreAtomic();
                    InvokeCallback();

                    if (source == kCommandLine) {
                        on_command_line_ = true;
                    }
                    break;
                }
                case SET_FLAG_IF_DEFAULT: {
                    // set the flag's value, but only if it hasn't been set by someone else
                    if (!modified_) {
                        if (!TryParse(&cur_, value, err)) return false;
                        modified_ = true;
                        counter_++;
                        StoreAtomic();
                        InvokeCallback();
                    } else {
                        // TODO(rogeeff): review and fix this semantic. Currently we do not fail
                        // in this case if flag is modified. This is misleading since the flag's
                        // value is not updated even though we return true.
                        // *err = abel::string_cat(Name(), " is already set to ",
                        //                     CurrentValue(), "\n");
                        // return false;
                        return true;
                    }
                    break;
                }
                case SET_FLAGS_DEFAULT: {
                    if (def_kind_ == FlagDefaultSrcKind::kDynamicValue) {
                        if (!TryParse(&default_src_.dynamic_value, value, err)) {
                            return false;
                        }
                    } else {
                        void *new_default_val = nullptr;
                        if (!TryParse(&new_default_val, value, err)) {
                            return false;
                        }

                        default_src_.dynamic_value = new_default_val;
                        def_kind_ = FlagDefaultSrcKind::kDynamicValue;
                    }

                    if (!modified_) {
                        // Need to set both default value *and* current, in this case
                        copy(op_, default_src_.dynamic_value, cur_);
                        StoreAtomic();
                        InvokeCallback();
                    }
                    break;
                }
            }

            return true;
        }

        void FlagImpl::check_default_value_parsing_roundtrip() const {
            std::string v = DefaultValue();

            abel::mutex_lock lock(DataGuard());

            auto dst = MakeInitValue();
            std::string error;
            if (!flags_internal::parse(marshalling_op_, v, dst.get(), &error)) {
                ABEL_INTERNAL_LOG(
                        FATAL,
                        abel::string_cat("Flag ", Name(), " (from ", Filename(),
                                         "): std::string form of default value '", v,
                                         "' could not be parsed; error=", error));
            }

            // We do not compare dst to def since parsing/unparsing may make
            // small changes, e.g., precision loss for floating point types.
        }

        bool FlagImpl::validate_input_value(abel::string_view value) const {
            abel::mutex_lock l(DataGuard());

            auto obj = MakeInitValue();
            std::string ignored_error;
            return flags_internal::parse(marshalling_op_, value, obj.get(),
                                         &ignored_error);
        }

    }  // namespace flags_internal

}  // namespace abel
