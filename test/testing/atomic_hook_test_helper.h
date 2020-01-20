//

#ifndef ABEL_TESTING_ATOMIC_HOOK_TEST_HELPER_H_
#define ABEL_TESTING_ATOMIC_HOOK_TEST_HELPER_H_

#include <abel/base/internal/atomic_hook.h>

namespace abel {

namespace atomic_hook_internal {

using VoidF = void (*)();
extern abel::base_internal::AtomicHook<VoidF> func;
extern int default_func_calls;
void DefaultFunc();
void RegisterFunc(VoidF func);

}  // namespace atomic_hook_internal

}  // namespace abel

#endif  // ABEL_TESTING_ATOMIC_HOOK_TEST_HELPER_H_
