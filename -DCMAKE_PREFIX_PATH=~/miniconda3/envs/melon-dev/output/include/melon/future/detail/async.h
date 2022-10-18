
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_ASYNC_H_
#define MELON_FUTURE_DETAIL_ASYNC_H_


namespace melon {
    template<typename QueueT, typename CbT>
    auto async(QueueT &q, CbT &&cb) {
        using cb_result_type = decltype(cb());

        using dst_storage_type =
        future_internal::storage_for_cb_result_t<std::allocator<void>, cb_result_type>;
        using result_fut_t = typename dst_storage_type::future_type;

        future_internal::storage_ptr <dst_storage_type> res;
        res.allocate(std::allocator<void>());

        future_internal::enqueue(&q, [cb = std::move(cb), res] {
            try {
                if constexpr (std::is_same_v<void, cb_result_type>) {
                    cb();
                    res->full_fill();
                } else {
                    res->full_fill(cb());
                }
            } catch (...) {
                res->fail(std::current_exception());
            }
        });

        return result_fut_t{res};
    }
}  // namespace melon

#endif  // MELON_FUTURE_DETAIL_ASYNC_H_
