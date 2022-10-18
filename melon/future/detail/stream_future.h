
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_STREAM_FUTURE_H_
#define MELON_FUTURE_DETAIL_STREAM_FUTURE_H_

#include "melon/future/detail/foreach_handler.h"

#include <future>

namespace melon {

    template <typename Alloc, typename... Ts>
    basic_stream_future<Alloc, Ts...>::basic_stream_future(
            future_internal::storage_ptr<storage_type> s)
            : storage_(std::move(s)) {}

    template <typename Alloc, typename... Ts>
    template <typename CbT>
    basic_future<Alloc, void> basic_stream_future<Alloc, Ts...>::for_each(
            CbT&& cb) {
        future_internal::immediate_queue queue;
        return this->for_each(queue, std::forward<CbT>(cb));
    }

    template <typename Alloc, typename... Ts>
    template <typename QueueT, typename CbT>
    [[nodiscard]] basic_future<Alloc, void>
    basic_stream_future<Alloc, Ts...>::for_each(QueueT& queue, CbT&& cb) {
        assert(storage_);
        static_assert(std::is_invocable_v<CbT, Ts...>,
                      "for_each should be accepting the correct arguments");

        using handler_t =
        future_internal::future_stream_foreach_handler<Alloc, std::decay_t<CbT>, QueueT,
                Ts...>;

        // This must be done BEFORE set_handler
        auto result_fut = storage_->get_final_future();

        storage_->template set_handler<handler_t>(&queue, std::move(cb));
        storage_.reset();

        return result_fut;
    }

}  // namespace melon
#endif  // MELON_FUTURE_DETAIL_STREAM_FUTURE_H_
