

#include <testing/atomic_hook_test_helper.h>
#include <abel/base/profile.h>
#include <abel/atomic/atomic_hook.h>

namespace abel {

    namespace atomic_hook_internal {

        ABEL_CONST_INIT abel::atomic_hook<VoidF> func(DefaultFunc);
        ABEL_CONST_INIT int default_func_calls = 0;

        void DefaultFunc() { default_func_calls++; }

        void RegisterFunc(VoidF f) { func.Store(f); }

    }  // namespace atomic_hook_internal

}  // namespace abel
