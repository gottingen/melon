
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_STREAM_PROMISE_H_
#define MELON_FUTURE_DETAIL_STREAM_PROMISE_H_


namespace melon {

    template <typename Alloc, typename... Ts>
    basic_stream_promise<Alloc, Ts...>::basic_stream_promise() {}

    template <typename Alloc, typename... Ts>
    basic_stream_promise<Alloc, Ts...>::~basic_stream_promise() {
        if (storage_) {
            storage_->fail(std::make_exception_ptr(unfull_filled_promise{}));
        }
    }

    template <typename Alloc, typename... Ts>
    typename basic_stream_promise<Alloc, Ts...>::future_type
    basic_stream_promise<Alloc, Ts...>::get_future(const Alloc& alloc) {
        storage_.allocate(alloc);

        return future_type{storage_};
    }

    template <typename Alloc, typename... Ts>
    template <typename... Us>
    void basic_stream_promise<Alloc, Ts...>::push(Us&&... vals) {
        assert(storage_);
        storage_->push(std::forward<Us>(vals)...);
    }

    template <typename Alloc, typename... Ts>
    void basic_stream_promise<Alloc, Ts...>::complete() {
        assert(storage_);
        storage_->complete();

        storage_.reset();
    }

    template <typename Alloc, typename... Ts>
    void basic_stream_promise<Alloc, Ts...>::set_exception(fail_type e) {
        assert(storage_);

        storage_->fail(std::move(e));
        storage_.reset();
    }

    template <typename Alloc, typename... Ts>
    basic_stream_promise<Alloc, Ts...>::operator bool() const {
        return storage_;
    }
}  // namespace melon
#endif  // MELON_FUTURE_DETAIL_STREAM_PROMISE_H_
