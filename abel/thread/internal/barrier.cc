//
// Created by liyinbin on 2021/4/5.
//


#include "abel/thread/internal/barrier.h"

#include <sys/mman.h>
#include <mutex>

#include "abel/log/logging.h"
#include "abel/memory/non_destroy.h"

namespace abel {
    namespace thread_internal {

        namespace {

            void *create_one_byte_dummy_page() {
                auto ptr = mmap(nullptr, 1, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                DCHECK(ptr, "Cannot create dummy page for asymmetric memory barrier.");
                (void) mlock(ptr, 1);
                return ptr;
            }

            // `membarrier()` is not usable until Linux 4.3, which is not available until
            //
            // Here Folly provides a workaround by mutating our page tables. Mutating page
            // tables, for the moment, implicitly cause the system to execute a barrier on
            // every core running our threads. I shamelessly copied their solution here.
            //
            // @sa:
            // https://github.com/facebook/folly/blob/master/folly/synchronization/AsymmetricMemoryBarrier.cpp
            void homemade_membarrier() {
                static void *dummy_page = create_one_byte_dummy_page();  // Never freed.

                // Previous memory accesses may not be reordered after syscalls below.
                // (Frankly this is implied by acquired lock below.)
                memory_barrier();

                static non_destroy <std::mutex> lock;
                std::scoped_lock _(*lock);

                // Upgrading protection does not always result in fence in each core (as it
                // can be delayed until #PF).
                DCHECK(mprotect(dummy_page, 1, PROT_READ | PROT_WRITE) == 0);
                *static_cast<char *>(dummy_page) = 0;  // Make sure the page is present.
                // This time a barrier should be issued to every cores.
                DCHECK(mprotect(dummy_page, 1, PROT_READ) == 0);

                // Subsequent memory accesses may not be reordered before syscalls above. (Not
                // sure if it's already implied by `mprotect`?)
                memory_barrier();
            }

        }  // namespace

        void asymmetric_barrier_heavy() {
            homemade_membarrier();
        }
    }  // namespace thread_internal
}  // namespace abel
