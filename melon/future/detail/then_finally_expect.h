
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_THEN_FINALLY_EXPECT_H_
#define MELON_FUTURE_DETAIL_THEN_FINALLY_EXPECT_H_


#include "melon/future/detail/storage_fwd.h"

namespace melon::future_internal {

    // handling for Future::finally()
    template<typename CbT, typename QueueT, typename... Ts>
    class future_finally_handler : public future_handler_base<QueueT, void, Ts...> {
        using parent_type = future_handler_base<QueueT, void, Ts...>;

        using full_fill_type = typename parent_type::full_fill_type;
        using finish_type = typename parent_type::finish_type;
        using fail_type = typename parent_type::fail_type;

        CbT cb_;

    public:
        future_finally_handler(QueueT *q, CbT cb)
                : parent_type(q), cb_(std::move(cb)) {}

        void full_fill(full_fill_type v) override {
            do_full_fill(this->get_queue(), std::move(v), std::move(cb_));
        };

        void finish(finish_type f) override {
            do_finish(this->get_queue(), std::move(f), std::move(cb_));
        };

        static void do_full_fill(QueueT *q, full_fill_type v, CbT cb) {
            auto cb_args =
                    full_fill_to_finish<0, 0, std::tuple<expected<Ts, std::exception_ptr>...>>(std::move(v));
            do_finish(q, std::move(cb_args), std::move(cb));
        }

        static void do_finish(QueueT *q, finish_type f, CbT cb) {
            enqueue(q, [cb = std::move(cb), f = std::move(f)]() mutable {
                std::apply(cb, std::move(f));
            });
        }

        static void do_fail(QueueT *q, fail_type e, CbT cb) {
            auto cb_args = fail_to_expect<0, std::tuple<expected<Ts, std::exception_ptr>...>>(std::move(e));
            do_finish(q, std::move(cb_args), std::move(cb));
        }
    };
}  // namespace melon::future_internal

#endif  // MELON_FUTURE_DETAIL_THEN_FINALLY_EXPECT_H_
