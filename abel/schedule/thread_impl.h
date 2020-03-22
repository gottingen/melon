//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_SCHEDULE_THREAD_IMPL_H_
#define ABEL_SCHEDULE_THREAD_IMPL_H_

#include <abel/schedule/preempt.h>
#include <setjmp.h>
#include <ucontext.h>
#include <abel/chrono/time.h>
#include <abel/types/optional.h>
#include <abel/chrono/clock.h>

namespace abel {

    class thread_context;

    class schedule_group;


    struct jmp_buf_link {
#ifdef ABEL_ASAN_ENABLED_DEBUG
        ucontext_t context;
        void *fake_stack = nullptr;
        const void *stack_bottom;
        size_t stack_size;
#else
        jmp_buf jmpbuf;
#endif
        jmp_buf_link *link;
        thread_context *thread;
        abel::optional <abel::abel_time> yield_at = {};
    public:
        void initial_switch_in(ucontext_t *initial_context, const void *stack_bottom, size_t stack_size);

        void switch_in();

        void switch_out();

        void initial_switch_in_completed();

        void final_switch_out();
    };

    extern thread_local jmp_buf_link *g_current_context;


    namespace thread_impl {

        inline thread_context *get() {
            return g_current_context->thread;
        }

        inline bool should_yield() {
            if (abel::need_preempt()) {
                return true;
            } else if (g_current_context->yield_at) {
                return abel::now() >= *(g_current_context->yield_at);
            } else {
                return false;
            }
        }

        schedule_group sched_group(const thread_context *);

        void yield();

        void switch_in(thread_context *to);

        void switch_out(thread_context *from);

        void init();

    }
}
#endif //ABEL_SCHEDULE_THREAD_IMPL_H_
