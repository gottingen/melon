//
//

#ifndef ABEL_FLAGS_INTERNAL_COMMANDLINEFLAG_H_
#define ABEL_FLAGS_INTERNAL_COMMANDLINEFLAG_H_

#include <memory>

#include <abel/base/profile.h>
#include <abel/config/flags/marshalling.h>
#include <abel/asl/optional.h>

namespace abel {

    namespace flags_internal {

// Type-specific operations, eg., parsing, copying, etc. are provided
// by function specific to that type with a signature matching flag_op_fn.
        enum FlagOp {
            kDelete,
            kClone,
            kCopy,
            kCopyConstruct,
            kSizeof,
            kParse,
            kUnparse
        };
        using flag_op_fn = void *(*)(FlagOp, const void *, void *);
        using FlagMarshallingOpFn = void *(*)(FlagOp, const void *, void *, void *);

// Options that control SetCommandLineOptionWithMode.
        enum FlagSettingMode {
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

// Options that control SetFromString: Source of a value.
        enum ValueSource {
            // Flag is being set by value specified on a command line.
            kCommandLine,
            // Flag is being set by value specified in the code.
            kProgrammaticChange,
        };

// The per-type function
        template<typename T>
        void *FlagOps(FlagOp op, const void *v1, void *v2) {
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
        void *FlagMarshallingOps(FlagOp op, const void *v1, void *v2, void *v3) {
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
        ABEL_FORCE_INLINE void Delete(flag_op_fn op, const void *obj) {
            op(flags_internal::kDelete, obj, nullptr);
        }

        ABEL_FORCE_INLINE void *Clone(flag_op_fn op, const void *obj) {
            return op(flags_internal::kClone, obj, nullptr);
        }

        ABEL_FORCE_INLINE void Copy(flag_op_fn op, const void *src, void *dst) {
            op(flags_internal::kCopy, src, dst);
        }

        ABEL_FORCE_INLINE void CopyConstruct(flag_op_fn op, const void *src, void *dst) {
            op(flags_internal::kCopyConstruct, src, dst);
        }

        ABEL_FORCE_INLINE bool Parse(FlagMarshallingOpFn op, abel::string_view text, void *dst,
                                     std::string *error) {
            return op(flags_internal::kParse, &text, dst, error) != nullptr;
        }

        ABEL_FORCE_INLINE std::string Unparse(FlagMarshallingOpFn op, const void *val) {
            std::string result;
            op(flags_internal::kUnparse, val, &result, nullptr);
            return result;
        }

        ABEL_FORCE_INLINE size_t Sizeof(flag_op_fn op) {
            // This sequence of casts reverses the sequence from base::internal::FlagOps()
            return static_cast<size_t>(reinterpret_cast<intptr_t>(
                    op(flags_internal::kSizeof, nullptr, nullptr)));
        }

// Handle to flag_state objects. Specific flag state objects will restore state
// of a flag produced this flag state from method command_line_flag::SaveState().
        class FlagStateInterface {
        public:
            virtual ~FlagStateInterface() {}

            // Restores the flag originated this object to the saved state.
            virtual void Restore() const = 0;
        };

// Holds all information for a flag.
        class command_line_flag {
        public:
            constexpr command_line_flag() = default;

            // Virtual destructor
            virtual void Destroy() = 0;

            // Not copyable/assignable.
            command_line_flag(const command_line_flag &) = delete;

            command_line_flag &operator=(const command_line_flag &) = delete;

            // Non-polymorphic access methods.

            // Return true iff flag has type T.
            template<typename T>
            ABEL_FORCE_INLINE bool IsOfType() const {
                return TypeId() == &flags_internal::FlagOps<T>;
            }

            // Attempts to retrieve the flag value. Returns value on success,
            // abel::nullopt otherwise.
            template<typename T>
            abel::optional<T> Get() const {
                if (IsRetired() || !IsOfType<T>()) {
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
            virtual abel::string_view Name() const = 0;

            // Returns name of the file where this flag is defined.
            virtual std::string Filename() const = 0;

            // Returns name of the flag's value type for some built-in types or empty
            // std::string.
            virtual abel::string_view Typename() const = 0;

            // Returns help message associated with this flag.
            virtual std::string Help() const = 0;

            // Returns true iff this object corresponds to retired flag.
            virtual bool IsRetired() const { return false; }

            // Returns true iff this is a handle to an abel Flag.
            virtual bool IsAbelFlag() const { return true; }

            // Returns id of the flag's value type.
            virtual flags_internal::flag_op_fn TypeId() const = 0;

            virtual bool IsModified() const = 0;

            virtual bool IsSpecifiedOnCommandLine() const = 0;

            virtual std::string DefaultValue() const = 0;

            virtual std::string CurrentValue() const = 0;

            // Interfaces to operate on validators.
            virtual bool ValidateInputValue(abel::string_view value) const = 0;

            // Interface to save flag to some persistent state. Returns current flag state
            // or nullptr if flag does not support saving and restoring a state.
            virtual std::unique_ptr<FlagStateInterface> SaveState() = 0;

            // Sets the value of the flag based on specified std::string `value`. If the flag
            // was successfully set to new value, it returns true. Otherwise, sets `error`
            // to indicate the error, leaves the flag unchanged, and returns false. There
            // are three ways to set the flag's value:
            //  * Update the current flag value
            //  * Update the flag's default value
            //  * Update the current flag value if it was never set before
            // The mode is selected based on `set_mode` parameter.
            virtual bool SetFromString(abel::string_view value,
                                       flags_internal::FlagSettingMode set_mode,
                                       flags_internal::ValueSource source,
                                       std::string *error) = 0;

            // Checks that flags default value can be converted to std::string and back to the
            // flag's value type.
            virtual void CheckDefaultValueParsingRoundtrip() const = 0;

        protected:
            ~command_line_flag() = default;

        private:
            // Copy-construct a new value of the flag's type in a memory referenced by
            // the dst based on the current flag's value.
            virtual void Read(void *dst) const = 0;
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

#endif  // ABEL_FLAGS_INTERNAL_COMMANDLINEFLAG_H_
