//

#ifndef ABEL_BASE_ATOMIC_HOOK_TEST_HELPER_H_
#define ABEL_BASE_ATOMIC_HOOK_TEST_HELPER_H_

#include <abel/base/internal/atomic_hook.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace atomic_hook_internal {

using VoidF = void (*)();
extern abel::base_internal::AtomicHook<VoidF> func;
extern int default_func_calls;
void DefaultFunc();
void RegisterFunc(VoidF func);

}  // namespace atomic_hook_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_ATOMIC_HOOK_TEST_HELPER_H_
