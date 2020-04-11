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

        void flag_impl::init() {
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
                cur_ = make_init_value().release();
                store_atomic();
                inited_.store(true, std::memory_order_release);
                invoke_callback();
            }
        }

// Ensures that the lazily initialized data is initialized,
// and returns pointer to the mutex guarding flags data.
        abel::mutex *flag_impl::data_guard() const {
            if (ABEL_UNLIKELY(!inited_.load(std::memory_order_acquire))) {
                const_cast<flag_impl *>(this)->init();
            }

            // data_guard_ is initialized.
            return reinterpret_cast<abel::mutex *>(&data_guard_);
        }

        void flag_impl::destroy() {
            {
                abel::mutex_lock l(data_guard());

                // Values are heap allocated for abel Flags.
                if (cur_) remove(op_, cur_);

                // Release the dynamically allocated default value if any.
                if (def_kind_ == flag_dfault_src_kind::kDynamicValue) {
                    remove(op_, default_src_.dynamic_value);
                }

                // If this flag has an assigned callback, release callback data.
                if (callback_data_) delete callback_data_;
            }

            abel::mutex_lock l(&flag_mutex_lifetime_guard);
            data_guard()->~mutex();
            is_data_guard_inited_ = false;
        }

        std::unique_ptr<void, dyn_value_deleter> flag_impl::make_init_value() const {
            void *res = nullptr;
            if (def_kind_ == flag_dfault_src_kind::kDynamicValue) {
                res = clone(op_, default_src_.dynamic_value);
            } else {
                res = (*default_src_.gen_func)();
            }
            return {res, dyn_value_deleter{op_}};
        }

        abel::string_view flag_impl::name() const { return name_; }

        std::string flag_impl::file_name() const {
            return flags_internal::get_usage_config().normalize_filename(filename_);
        }

        std::string flag_impl::help() const {
            return help_source_kind_ == flag_help_src_kind::kLiteral ? help_.literal
                                                                  : help_.gen_func();
        }

        bool flag_impl::is_modified() const {
            abel::mutex_lock l(data_guard());
            return modified_;
        }

        bool flag_impl::is_specified_on_command_line() const {
            abel::mutex_lock l(data_guard());
            return on_command_line_;
        }

        std::string flag_impl::default_value() const {
            abel::mutex_lock l(data_guard());

            auto obj = make_init_value();
            return unparse(marshalling_op_, obj.get());
        }

        std::string flag_impl::current_value() const {
            abel::mutex_lock l(data_guard());

            return unparse(marshalling_op_, cur_);
        }

        void flag_impl::set_callback(
                const flags_internal::flag_callback mutation_callback) {
            abel::mutex_lock l(data_guard());

            if (callback_data_ == nullptr) {
                callback_data_ = new callback_data;
            }
            callback_data_->func = mutation_callback;

            invoke_callback();
        }

        void flag_impl::invoke_callback() const {
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
            MutexRelock relock(data_guard());
            abel::mutex_lock lock(&callback_data_->guard);
            cb();
        }

        bool flag_impl::restore_state(const void *value, bool modified,
                                    bool on_command_line, int64_t counter) {
            {
                abel::mutex_lock l(data_guard());

                if (counter_ == counter) return false;
            }

            write(value, op_);

            {
                abel::mutex_lock l(data_guard());

                modified_ = modified;
                on_command_line_ = on_command_line;
            }

            return true;
        }

// Attempts to parse supplied `value` string using parsing routine in the `flag`
// argument. If parsing successful, this function replaces the dst with newly
// parsed value. In case if any error is encountered in either step, the error
// message is stored in 'err'
        bool flag_impl::try_parse(void **dst, abel::string_view value,
                                std::string *err) const {
            auto tentative_value = make_init_value();

            std::string parse_err;
            if (!parse(marshalling_op_, value, tentative_value.get(), &parse_err)) {
                abel::string_view err_sep = parse_err.empty() ? "" : "; ";
                *err = abel::string_cat("Illegal value '", value, "' specified for flag '",
                                        name(), "'", err_sep, parse_err);
                return false;
            }

            void *old_val = *dst;
            *dst = tentative_value.release();
            tentative_value.reset(old_val);

            return true;
        }

        void flag_impl::read(void *dst, const flags_internal::flag_op_fn dst_op) const {
            abel::reader_mutex_lock l(data_guard());

            // `dst_op` is the unmarshaling operation corresponding to the declaration
            // visibile at the call site. `op` is the Flag's defined unmarshalling
            // operation. They must match for this operation to be well-defined.
            if (ABEL_UNLIKELY(dst_op != op_)) {
                ABEL_INTERNAL_LOG(
                        ERROR,
                        abel::string_cat("Flag '", name(),
                                         "' is defined as one type and declared as another"));
            }
            copy_construct(op_, cur_, dst);
        }

        void flag_impl::store_atomic() {
            size_t data_size = size_of(op_);

            if (data_size <= sizeof(int64_t)) {
                int64_t t = 0;
                std::memcpy(&t, cur_, data_size);
                atomic_.store(t, std::memory_order_release);
            }
        }

        void flag_impl::write(const void *src, const flags_internal::flag_op_fn src_op) {
            abel::mutex_lock l(data_guard());

            // `src_op` is the marshalling operation corresponding to the declaration
            // visible at the call site. `op` is the Flag's defined marshalling operation.
            // They must match for this operation to be well-defined.
            if (ABEL_UNLIKELY(src_op != op_)) {
                ABEL_INTERNAL_LOG(
                        ERROR,
                        abel::string_cat("Flag '", name(),
                                         "' is defined as one type and declared as another"));
            }

            if (ShouldValidateFlagValue(op_)) {
                void *obj = clone(op_, src);
                std::string ignored_error;
                std::string src_as_str = unparse(marshalling_op_, src);
                if (!parse(marshalling_op_, src_as_str, obj, &ignored_error)) {
                    ABEL_INTERNAL_LOG(ERROR, abel::string_cat("Attempt to set flag '", name(),
                                                              "' to invalid value ", src_as_str));
                }
                remove(op_, obj);
            }

            modified_ = true;
            counter_++;
            copy(op_, src, cur_);

            store_atomic();
            invoke_callback();
        }

// Sets the value of the flag based on specified string `value`. If the flag
// was successfully set to new value, it returns true. Otherwise, sets `err`
// to indicate the error, leaves the flag unchanged, and returns false. There
// are three ways to set the flag's value:
//  * Update the current flag value
//  * Update the flag's default value
//  * Update the current flag value if it was never set before
// The mode is selected based on 'set_mode' parameter.
        bool flag_impl::set_from_string(abel::string_view value, flag_setting_mode set_mode,
                                     value_source source, std::string *err) {
            abel::mutex_lock l(data_guard());

            switch (set_mode) {
                case SET_FLAGS_VALUE: {
                    // set or modify the flag's value
                    if (!try_parse(&cur_, value, err)) return false;
                    modified_ = true;
                    counter_++;
                    store_atomic();
                    invoke_callback();

                    if (source == kCommandLine) {
                        on_command_line_ = true;
                    }
                    break;
                }
                case SET_FLAG_IF_DEFAULT: {
                    // set the flag's value, but only if it hasn't been set by someone else
                    if (!modified_) {
                        if (!try_parse(&cur_, value, err)) return false;
                        modified_ = true;
                        counter_++;
                        store_atomic();
                        invoke_callback();
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
                    if (def_kind_ == flag_dfault_src_kind::kDynamicValue) {
                        if (!try_parse(&default_src_.dynamic_value, value, err)) {
                            return false;
                        }
                    } else {
                        void *new_default_val = nullptr;
                        if (!try_parse(&new_default_val, value, err)) {
                            return false;
                        }

                        default_src_.dynamic_value = new_default_val;
                        def_kind_ = flag_dfault_src_kind::kDynamicValue;
                    }

                    if (!modified_) {
                        // Need to set both default value *and* current, in this case
                        copy(op_, default_src_.dynamic_value, cur_);
                        store_atomic();
                        invoke_callback();
                    }
                    break;
                }
            }

            return true;
        }

        void flag_impl::check_default_value_parsing_roundtrip() const {
            std::string v = default_value();

            abel::mutex_lock lock(data_guard());

            auto dst = make_init_value();
            std::string error;
            if (!flags_internal::parse(marshalling_op_, v, dst.get(), &error)) {
                ABEL_INTERNAL_LOG(
                        FATAL,
                        abel::string_cat("Flag ", name(), " (from ", file_name(),
                                         "): std::string form of default value '", v,
                                         "' could not be parsed; error=", error));
            }

            // We do not compare dst to def since parsing/unparsing may make
            // small changes, e.g., precision loss for floating point types.
        }

        bool flag_impl::validate_input_value(abel::string_view value) const {
            abel::mutex_lock l(data_guard());

            auto obj = make_init_value();
            std::string ignored_error;
            return flags_internal::parse(marshalling_op_, value, obj.get(),
                                         &ignored_error);
        }

    }  // namespace flags_internal

}  // namespace abel
