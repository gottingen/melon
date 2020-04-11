//

#include <abel/config/flag.h>

#include <cstring>

namespace abel {


// We want to validate the type mismatch between type definition and
// declaration. The lock-free implementation does not allow us to do it,
// so in debug builds we always use the slower implementation, which always
// validates the type.
#ifndef NDEBUG
#define ABEL_FLAGS_ATOMIC_GET(T) \
  T get_flag(const abel::Flag<T>& flag) { return flag.Get(); }
#else
#define ABEL_FLAGS_ATOMIC_GET(T)         \
  T get_flag(const abel::Flag<T>& flag) { \
    T result;                            \
    if (flag.AtomicGet(&result)) {       \
      return result;                     \
    }                                    \
    return flag.Get();                   \
  }
#endif

    ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(ABEL_FLAGS_ATOMIC_GET)

#undef ABEL_FLAGS_ATOMIC_GET

// This global nutex protects on-demand construction of flag objects in MSVC
// builds.
#if defined(_MSC_VER) && !defined(__clang__)

    namespace flags_internal {

    ABEL_CONST_INIT static abel::mutex construction_guard(abel::kConstInit);

    abel::mutex* GetGlobalConstructionGuard() { return &construction_guard; }

    }  // namespace flags_internal

#endif


}  // namespace abel
