
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_THEN_H_
#define MELON_FUTURE_DETAIL_THEN_H_


#include "melon/future/detail/storage_fwd.h"

namespace melon::future_internal {


        // Handler class for defered Future::then() execution
        template <typename Alloc, typename CbT, typename QueueT, typename... Ts>
        class future_then_handler : public future_handler_base<QueueT, void, Ts...> {
        public:
            using parent_type = future_handler_base<QueueT, void, Ts...>;

            using full_fill_type = typename parent_type::full_fill_type;
            using finish_type = typename parent_type::finish_type;
            using fail_type = typename parent_type::fail_type;

            using cb_result_type =
            decltype(std::apply(std::declval<CbT>(), std::declval<full_fill_type>()));

            using dst_storage_type = storage_for_cb_result_t<Alloc, cb_result_type>;
            using dst_type = storage_ptr<dst_storage_type>;

            future_then_handler(QueueT* q, dst_type dst, CbT cb)
                    : parent_type(q), dst_(std::move(dst)), cb_(std::move(cb)) {}

            void full_fill(full_fill_type v) override {
                do_full_fill(this->get_queue(), std::move(v), std::move(dst_),
                            std::move(cb_));
            };

            void finish(finish_type f) override {
                do_finish(this->get_queue(), f, std::move(dst_), std::move(cb_));
            }

            static void do_full_fill(QueueT* q, full_fill_type v, dst_type dst, CbT cb) {
                enqueue(q, [cb = std::move(cb), dst = std::move(dst), v = std::move(v)] {
                    try {
                        if constexpr (std::is_same_v<void, cb_result_type>) {
                            std::apply(cb, std::move(v));
                            dst->full_fill(std::tuple<>{});
                        } else {
                            if constexpr (is_expected_v<cb_result_type>) {
                                dst->finish(std::apply(cb, std::move(v)));
                            } else {
                                dst->full_fill(std::apply(cb, std::move(v)));
                            }
                        }

                    } catch (...) {
                        dst->fail(std::current_exception());
                    }
                });
            }

            static void do_finish(QueueT* q, finish_type f, dst_type dst, CbT cb) {
                auto err = std::apply(get_first_error<Ts...>, f);
                if (err) {
                    do_fail(q, *err, std::move(dst), std::move(cb));
                } else {
                    full_fill_type cb_args =
                            finish_to_full_fill<std::tuple_size_v<finish_type> - 1>(std::move(f));

                    do_full_fill(q, std::move(cb_args), std::move(dst), std::move(cb));
                }
            }

            static void do_fail(QueueT* q, fail_type e, dst_type dst, CbT) {
                // Straight propagation.
                enqueue(q, [dst = std::move(dst), e = std::move(e)]() mutable {
                    dst->fail(std::move(e));
                });
            }

        private:
            dst_type dst_;
            CbT cb_;
        };
}  // namespace melon::future_internal

#endif  // MELON_FUTURE_DETAIL_THEN_H_
