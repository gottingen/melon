
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_FOREACH_HANDLER_H_
#define MELON_FUTURE_DETAIL_FOREACH_HANDLER_H_


#include "melon/future/detail/stream_storage_fwd.h"
#include "melon/future/detail/utility.h"

namespace melon::future_internal {


    // handling for Future::finally()
    template<typename Alloc, typename CbT, typename QueueT, typename... Ts>
    class future_stream_foreach_handler
            : public stream_handler_base<QueueT, void, Ts...> {
        using parent_type = stream_handler_base<QueueT, void, Ts...>;
        using fail_type = typename parent_type::fail_type;

    public:
        CbT cb_;
        storage_ptr <future_storage<Alloc, void>> finalizer_;

        CbT &get_cb() { return &cb_; }

        future_stream_foreach_handler(storage_ptr <future_storage<Alloc, void>> fin,
                                      QueueT *q, CbT cb)
                : parent_type(q), cb_(std::move(cb)), finalizer_(std::move(fin)) {}

        void push(Ts... args) override {
            do_push(this->get_queue(), cb_, std::tuple<Ts...>(std::move(args)...));
        }

        static void do_push(QueueT *q, CbT cb, std::tuple<Ts...> args) {
            enqueue(q, [cb = std::move(cb), args = (std::move(args))]() mutable {
                std::apply(cb, std::move(args));
            });
        }

        void complete() override {
            enqueue(this->get_queue(), [fin = std::move(finalizer_)]() {
                fin->full_fill(full_fill_type_t<void>());
            });
        }

        void fail(fail_type f) override {
            enqueue(this->get_queue(),
                    [f = std::move(f), fin = std::move(finalizer_)]() mutable {
                        fin->fail(std::move(f));
                    });
        }
    };
}  // namespace melon::future_internal

#endif  // MELON_FUTURE_DETAIL_FOREACH_HANDLER_H_
