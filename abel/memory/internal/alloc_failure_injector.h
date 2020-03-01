//
// Created by liyinbin on 2020/2/2.
//

#ifndef ABEL_MEMORY_INTERNAL_ALLOC_FAILURE_INJECTOR_H_
#define ABEL_MEMORY_INTERNAL_ALLOC_FAILURE_INJECTOR_H_

#include <limits>
#include <cstdint>
#include <functional>
#include <abel/utility/non_copyable.h>

namespace abel {
    namespace memory_internal {

        class alloc_failure_injector {
            uint64_t _alloc_count;
            uint64_t _fail_at = std::numeric_limits<uint64_t>::max();
            noncopyable_function<void()> _on_alloc_failure = [] { throw std::bad_alloc(); };
            bool _failed;
            uint64_t _suppressed = 0;
            friend struct disable_failure_guard;
        private:
            void fail();

        public:
            void on_alloc_point() {
                if (_suppressed) {
                    return;
                }
                if (_alloc_count >= _fail_at) {
                    fail();
                }
                ++_alloc_count;
            }

            // Counts encountered allocation points which didn't fail and didn't have failure suppressed
            uint64_t alloc_count() const {
                return _alloc_count;
            }

            // Will cause count-th allocation point from now to fail, counting from 0
            void fail_after(uint64_t count) {
                _fail_at = _alloc_count + count;
                _failed = false;
            }

            // Cancels the failure scheduled by fail_after()
            void cancel() {
                _fail_at = std::numeric_limits<uint64_t>::max();
            }

            // Returns true iff allocation was failed since last fail_after()
            bool failed() const {
                return _failed;
            }

            // Runs given function with a custom failure action instead of the default std::bad_alloc throw.
            void run_with_callback(noncopyable_function<void()> callback, noncopyable_function<void()> to_run);
        };

        extern thread_local alloc_failure_injector the_alloc_failure_injector;

        inline
        alloc_failure_injector &local_failure_injector() {
            return the_alloc_failure_injector;
        }

#ifdef ABEL_ENABLE_ALLOC_FAILURE_INJECTION

        struct disable_failure_guard {
            disable_failure_guard() { ++local_failure_injector()._suppressed; }
            ~disable_failure_guard() { --local_failure_injector()._suppressed; }
        };

#else

        struct disable_failure_guard {
            ~disable_failure_guard() {}
        };

#endif

// Marks a point in code which should be considered for failure injection
        inline
        void on_alloc_point() {
#ifdef ABEL_ENABLE_ALLOC_FAILURE_INJECTION
            local_failure_injector().on_alloc_point();
#endif
        }
    } //namespace memory_internal
} //namespace abel

#endif //ABEL_MEMORY_INTERNAL_ALLOC_FAILURE_INJECTOR_H_
