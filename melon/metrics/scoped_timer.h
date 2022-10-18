
#ifndef  MELON_VARIABLE_SCOPED_TIMER_H_
#define  MELON_VARIABLE_SCOPED_TIMER_H_

#include "melon/times/time.h"

// Accumulate microseconds spent by scopes into variable, useful for debugging.
// Example:
//   melon::counter<int64_t> g_function1_spent;
//   ...
//   void function1() {
//     // time cost by function1() will be sent to g_spent_time when
//     // the function returns.
//     melon::scoped_timer tm(g_function1_spent);
//     ...
//   }
// To check how many microseconds the function spend in last second, you
// can wrap the variable within per_second and make it viewable from /vars
//   melon::per_second<melon::Adder<int64_t> > g_function1_spent_second(
//     "function1_spent_second", &g_function1_spent);
namespace melon {
    template<typename T>
    class scoped_timer {
    public:
        explicit scoped_timer(T &variable)
                : _start_time(melon::get_current_time_micros()), _var(&variable) {}

        ~scoped_timer() {
            *_var << (melon::get_current_time_micros() - _start_time);
        }

        void reset() { _start_time = melon::get_current_time_micros(); }

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(scoped_timer);

        int64_t _start_time;
        T *_var;
    };
} // namespace melon

#endif  // MELON_VARIABLE_SCOPED_TIMER_H_
