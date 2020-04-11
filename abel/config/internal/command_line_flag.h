//
//

#ifndef ABEL_CONFIG_INTERNAL_COMMANDLINEFLAG_H_
#define ABEL_CONFIG_INTERNAL_COMMANDLINEFLAG_H_

#include <memory>
#include <abel/base/profile.h>
#include <abel/config/marshalling.h>
#include <abel/asl/optional.h>

namespace abel {

    namespace flags_internal {

// Type-specific operations, eg., parsing, copying, etc. are provided
// by function specific to that type with a signature matching flag_op_fn.
        enum flag_op {
            kDelete,
            kClone,
            kCopy,
            kCopyConstruct,
            kSizeof,
            kParse,
            kUnparse
        };
        using flag_op_fn = void *(*)(flag_op, const void *, void *);
        using flag_marshalling_op_fn = void *(*)(flag_op, const void *, void *, void *);

// Options that control set_command_line_option_with_mode.
        enum flag_setting_mode {
            // update the flag's value unconditionally (can call this multiple times).
            SET_FLAGS_VALUE,
            // update the flag's value, but *only if* it has not yet been updated
            // with SET_FLAGS_VALUE, SET_FLAG_IF_DEFAULT, or "FLAGS_xxx = nondef".
            SET_FLAG_IF_DEFAULT,
            // set the flag's default value to this.  If the flag has not been updated
            // yet (via SET_FLAGS_VALUE, SET_FLAG_IF_DEFAULT, or "FLAGS_xxx = nondef")
            // change the flag's current value to the new default value as well.
            SET_FLAGS_DEFAULT
        };

// Options that control set_from_string: Source of a value.
        enum value_source {
            // Flag is being set by value specified on a command line.
            kCommandLine,
            // Flag is being set by value specified in the code.
            kProgrammaticChange,
        };

// The per-type function
        template<typename T>
        void *flag_ops(flag_op op, const void *v1, void *v2) {
            switch (op) {
                case kDelete:
                    delete static_cast<const T *>(v1);
                    return nullptr;
                case kClone:
                    return new T(*static_cast<const T *>(v1));
                case kCopy:
                    *static_cast<T *>(v2) = *static_cast<const T *>(v1);
                    return nullptr;
                case kCopyConstruct:
                    new(v2) T(*static_cast<const T *>(v1));
                    return nullptr;
                case kSizeof:
                    return reinterpret_cast<void *>(sizeof(T));
                default:
                    return nullptr;
            }
        }

        template<typename T>
        void *flag_marshalling_ops(flag_op op, const void *v1, void *v2, void *v3) {
            switch (op) {
                case kParse: {
                    // initialize the temporary instance of type T based on current value in
                    // destination (which is going to be flag's default value).
                    T temp(*static_cast<T *>(v2));
                    if (!abel::parse_flag<T>(*static_cast<const abel::string_view *>(v1), &temp,
                                            static_cast<std::string *>(v3))) {
                        return nullptr;
                    }
                    *static_cast<T *>(v2) = std::move(temp);
                    return v2;
                }
                case kUnparse:
                    *static_cast<std::string *>(v2) =
                            abel::unparse_flag<T>(*static_cast<const T *>(v1));
                    return nullptr;
                default:
                    return nullptr;
            }
        }

// Functions that invoke flag-type-specific operations.
        ABEL_FORCE_INLINE void remove(flag_op_fn op, const void *obj) {
            op(flags_internal::kDelete, obj, nullptr);
        }

        ABEL_FORCE_INLINE void *clone(flag_op_fn op, const void *obj) {
            return op(flags_internal::kClone, obj, nullptr);
        }

        ABEL_FORCE_INLINE void copy(flag_op_fn op, const void *src, void *dst) {
            op(flags_internal::kCopy, src, dst);
        }

        ABEL_FORCE_INLINE void copy_construct(flag_op_fn op, const void *src, void *dst) {
            op(flags_internal::kCopyConstruct, src, dst);
        }

        ABEL_FORCE_INLINE bool parse(flag_marshalling_op_fn op, abel::string_view text, void *dst,
                                     std::string *error) {
            return op(flags_internal::kParse, &text, dst, error) != nullptr;
        }

        ABEL_FORCE_INLINE std::string unparse(flag_marshalling_op_fn op, const void *val) {
            std::string result;
            op(flags_internal::kUnparse, val, &result, nullptr);
            return result;
        }

        ABEL_FORCE_INLINE size_t size_of(flag_op_fn op) {
            // This sequence of casts reverses the sequence from base::internal::flag_ops()
            return static_cast<size_t>(reinterpret_cast<intptr_t>(
                    op(flags_internal::kSizeof, nullptr, nullptr)));
        }

// Handle to flag_state objects. Specific flag state objects will restore state
// of a flag produced this flag state from method command_line_flag::save_state().
        class flag_state_interface {
        public:
            virtual ~flag_state_interface() {}

            // Restores the flag originated this object to the saved state.
            virtual void restore() const = 0;
        };

// Holds all information for a flag.
        class command_line_flag {
        public:
            constexpr command_line_flag() = default;

            // Virtual destructor
            virtual void destroy() = 0;

            // Not copyable/assignable.
            command_line_flag(const command_line_flag &) = delete;

            command_line_flag &operator=(const command_line_flag &) = delete;

            // Non-polymorphic access methods.

            // Return true iff flag has type T.
            template<typename T>
            ABEL_FORCE_INLINE bool is_of_type() const {
                return type_id() == &flags_internal::flag_ops<T>;
            }

            // Attempts to retrieve the flag value. Returns value on success,
            // abel::nullopt otherwise.
            template<typename T>
            abel::optional<T> get() const {
                if (is_retired() || !is_of_type<T>()) {
                    return abel::nullopt;
                }

                // Implementation notes:
                //
                // We are wrapping a union around the value of `T` to serve three purposes:
                //
                //  1. `U.value` has correct size and alignment for a value of type `T`
                //  2. The `U.value` constructor is not invoked since U's constructor does
                //     not do it explicitly.
                //  3. The `U.value` destructor is invoked since U's destructor does it
                //     explicitly. This makes `U` a kind of RAII wrapper around non default
                //     constructible value of T, which is destructed when we leave the
                //     scope. We do need to destroy U.value, which is constructed by
                //     command_line_flag::Read even though we left it in a moved-from state
                //     after std::move.
                //
                // All of this serves to avoid requiring `T` being default constructible.
                union U {
                    T value;

                    U() {}

                    ~U() { value.~T(); }
                };
                U u;

                Read(&u.value);
                return std::move(u.value);
            }

            // Polymorphic access methods

            // Returns name of this flag.
            virtual abel::string_view name() const = 0;

            // Returns name of the file where this flag is defined.
            virtual std::string file_name() const = 0;

            // Returns name of the flag's value type for some built-in types or empty
            // std::string.
            virtual abel::string_view type_name() const = 0;

            // Returns help message associated with this flag.
            virtual std::string help() const = 0;

            // Returns true iff this object corresponds to retired flag.
            virtual bool is_retired() const { return false; }

            // Returns true iff this is a handle to an abel Flag.
            virtual bool is_abel_flag() const { return true; }

            // Returns id of the flag's value type.
            virtual flags_internal::flag_op_fn type_id() const = 0;

            virtual bool is_modified() const = 0;

            virtual bool is_specified_on_command_line() const = 0;

            virtual std::string default_value() const = 0;

            virtual std::string current_value() const = 0;

            // Interfaces to operate on validators.
            virtual bool validate_input_value(abel::string_view value) const = 0;

            // Interface to save flag to some persistent state. Returns current flag state
            // or nullptr if flag does not support saving and restoring a state.
            virtual std::unique_ptr<flag_state_interface> save_state() = 0;

            // Sets the value of the flag based on specified std::string `value`. If the flag
            // was successfully set to new value, it returns true. Otherwise, sets `error`
            // to indicate the error, leaves the flag unchanged, and returns false. There
            // are three ways to set the flag's value:
            //  * Update the current flag value
            //  * Update the flag's default value
            //  * Update the current flag value if it was never set before
            // The mode is selected based on `set_mode` parameter.
            virtual bool set_from_string(abel::string_view value,
                                       flags_internal::flag_setting_mode set_mode,
                                       flags_internal::value_source source,
                                       std::string *error) = 0;

            // Checks that flags default value can be converted to std::string and back to the
            // flag's value type.
            virtual void check_default_value_parsing_roundtrip() const = 0;

        protected:
            ~command_line_flag() = default;

        private:
            // Copy-construct a new value of the flag's type in a memory referenced by
            // the dst based on the current flag's value.
            virtual void read(void *dst) const = 0;
        };

// This macro is the "source of truth" for the list of supported flag types we
// expect to perform lock free operations on. Specifically it generates code,
// a one argument macro operating on a type, supplied as a macro argument, for
// each type in the list.
#define ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(A) \
  A(bool)                                         \
  A(short)                                        \
  A(unsigned short)                               \
  A(int)                                          \
  A(unsigned int)                                 \
  A(long)                                         \
  A(unsigned long)                                \
  A(long long)                                    \
  A(unsigned long long)                           \
  A(double)                                       \
  A(float)

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_CONFIG_INTERNAL_COMMANDLINEFLAG_H_
