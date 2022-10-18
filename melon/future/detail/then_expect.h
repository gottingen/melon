
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_THEN_EXPECT_H_
#define MELON_FUTURE_DETAIL_THEN_EXPECT_H_


#include "melon/future/detail/storage_fwd.h"

namespace melon::future_internal {

    // handling for Future::finally()
    template<typename Alloc, typename CbT, typename QueueT, typename... Ts>
    class future_then_expect_handler
            : public future_handler_base<QueueT, void, Ts...> {
    public:
        using parent_type = future_handler_base<QueueT, void, Ts...>;

        using full_fill_type = typename parent_type::full_fill_type;
        using finish_type = typename parent_type::finish_type;
        using fail_type = typename parent_type::fail_type;

        using cb_result_type =
        decltype(std::apply(std::declval<CbT>(), std::declval<finish_type>()));

        using dst_storage_type = storage_for_cb_result_t<Alloc, cb_result_type>;
        using dst_type = storage_ptr<dst_storage_type>;

        future_then_expect_handler(QueueT *q, dst_type dst, CbT cb)
                : parent_type(q), dst_(std::move(dst)), cb_(std::move(cb)) {}

        void full_fill(full_fill_type v) override {
            do_full_fill(this->get_queue(), std::move(v), std::move(dst_),
                        std::move(cb_));
        };

        void finish(finish_type f) override {
            do_finish(this->get_queue(), std::move(f), std::move(dst_), std::move(cb_));
        };

        static void do_full_fill(QueueT *q, full_fill_type v, dst_type dst, CbT cb) {
            auto cb_args =
                    full_fill_to_finish<0, 0, std::tuple<expected<Ts, std::exception_ptr>...>>(std::move(v));

            do_finish(q, std::move(cb_args), std::move(dst), std::move(cb));
        }

        static void do_finish(QueueT *q, finish_type f, dst_type dst, CbT cb) {
            enqueue(q, [cb = std::move(cb), dst = std::move(dst), f = std::move(f)] {
                try {
                    if constexpr (std::is_same_v<void, cb_result_type>) {
                        std::apply(cb, std::move(f));
                        dst->full_fill(std::tuple<>{});
                    } else {
                        if constexpr (is_expected_v<cb_result_type>) {
                            dst->finish(std::apply(cb, std::move(f)));
                        } else {
                            dst->full_fill(std::apply(cb, std::move(f)));
                        }
                    }

                } catch (...) {
                    dst->fail(std::current_exception());
                }
            });
        }

        static void do_fail(QueueT *q, fail_type e, dst_type dst, CbT cb) {
            auto cb_args = fail_to_expect<0, std::tuple<expected<Ts, std::exception_ptr>...>>(e);
            do_finish(q, std::move(cb_args), std::move(dst), std::move(cb));
        }

    private:
        dst_type dst_;
        CbT cb_;
    };
}  // namespace lare::future_internal

#endif  // MELON_FUTURE_DETAIL_THEN_EXPECT_H_
