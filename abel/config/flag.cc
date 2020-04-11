//

#include <abel/config/flag.h>
#include <cstring>
#include <abel/config/internal/registry.h>

namespace abel {


// We want to validate the type mismatch between type definition and
// declaration. The lock-free implementation does not allow us to do it,
// so in debug builds we always use the slower implementation, which always
// validates the type.
#ifndef NDEBUG
#define ABEL_FLAGS_ATOMIC_GET(T) \
  T get_flag(const abel::abel_flag<T>& flag) { return flag.get(); }
#else
#define ABEL_FLAGS_ATOMIC_GET(T)         \
  T get_flag(const abel::abel_flag<T>& flag) { \
    T result;                            \
    if (flag.atomic_get(&result)) {       \
      return result;                     \
    }                                    \
    return flag.get();                   \
  }
#endif

    ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(ABEL_FLAGS_ATOMIC_GET)

#undef ABEL_FLAGS_ATOMIC_GET

// This global nutex protects on-demand construction of flag objects in MSVC
// builds.
#if defined(_MSC_VER) && !defined(__clang__)

    namespace flags_internal {

    ABEL_CONST_INIT static abel::mutex construction_guard(abel::kConstInit);

    abel::mutex* get_global_construction_guard() { return &construction_guard; }

    }  // namespace flags_internal

#endif

    std::vector<command_line_flag *> get_all_flags() {
        std::vector<command_line_flag *> ret;
        flags_internal::for_each_flag([&ret](command_line_flag *flag) {
            ret.push_back(flag);
        });
        return ret;
    }

    std::vector<command_line_flag *> get_all_flags_unlock() {
        std::vector<command_line_flag *> ret;
        flags_internal::for_each_flag_unlocked([&ret](command_line_flag *flag) {
            ret.push_back(flag);
        });
        return ret;
    }

    void visit_flags_unlock(const flag_visitor &fv) {
        flags_internal::for_each_flag_unlocked(fv);
    }

    void visit_flags(const flag_visitor &fv) {
        flags_internal::for_each_flag(fv);
    }


}  // namespace abel
