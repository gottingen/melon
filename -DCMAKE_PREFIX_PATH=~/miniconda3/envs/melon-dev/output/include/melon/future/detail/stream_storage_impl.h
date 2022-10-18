
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_FUTURE_DETAIL_STREAM_STORAGE_IMPL_H_
#define MELON_FUTURE_DETAIL_STREAM_STORAGE_IMPL_H_

#include "melon/future/detail/utility.h"
#include "melon/future/detail/stream_storage_fwd.h"
#include <type_traits>

namespace melon::future_internal {

    template<typename Alloc, typename... Ts>
    stream_storage<Alloc, Ts...>::~stream_storage() {
        if (cb_data_.callback_) {
            cb_data_.callback_->~stream_handler_interface<Ts...>();
            using alloc_traits = std::allocator_traits<Alloc>;
            using Real_alloc = typename alloc_traits::template rebind_alloc<
                    stream_handler_interface<Ts...>>;

            Real_alloc real_alloc(allocator());
            real_alloc.deallocate(cb_data_.callback_, 1);
        }
    }

    template<typename Alloc, typename... Ts>
    stream_storage<Alloc, Ts...>::stream_storage(const Alloc &alloc)
            : Alloc(alloc) {
        final_promise_.allocate(alloc);
    }

    template<typename Alloc, typename... Ts>
    template<typename... Us>
    void stream_storage<Alloc, Ts...>::push(Us &&... args) {
        auto flags = state_.load();

        assert((flags & (Stream_storage_state_fail_bit |
                         Stream_storage_state_complete_bit)) == 0);

        if (flags & Stream_storage_state_ready_bit) {
            // This is supposed to be by far and wide the most common case.
            cb_data_.callback_->push(std::forward<Us>(args)...);
        } else {
            std::unique_lock l(mtx_);
            flags = state_.load();

            if (flags & Stream_storage_state_ready_bit) {
                // This is extremely unlikely.
                l.unlock();
                cb_data_.callback_->push(std::forward<Us>(args)...);
            } else {
                full_filled_.push_back(std::move(args)...);
            }
        }
    }

    template<typename Alloc, typename... Ts>
    void stream_storage<Alloc, Ts...>::complete() {
        std::unique_lock l(mtx_);
        auto flags = state_.load();

        if (flags & Stream_storage_state_ready_bit) {
            l.unlock();
            cb_data_.callback_->complete();
        } else {
            state_.fetch_or(Stream_storage_state_complete_bit);
        }
    }

    template<typename Alloc, typename... Ts>
    void stream_storage<Alloc, Ts...>::fail(fail_type &&e) {
        std::unique_lock l(mtx_);
        auto flags = state_.load();

        if (flags & Stream_storage_state_ready_bit) {
            l.unlock();
            cb_data_.callback_->fail(std::move(e));
        } else {
            error_ = std::move(e);
            state_.fetch_or(Stream_storage_state_fail_bit);
        }
    }

    template<typename Alloc, typename... Ts>
    template<typename Handler_t, typename QueueT, typename... Args_t>
    void stream_storage<Alloc, Ts...>::set_handler(QueueT *queue,
                                                   Args_t &&... args) {
        using alloc_traits = std::allocator_traits<Alloc>;
        using Real_alloc = typename alloc_traits::template rebind_alloc<Handler_t>;

        Handler_t *new_handler = nullptr;

        Real_alloc real_alloc(allocator());
        auto ptr = real_alloc.allocate(1);
        try {
            new_handler = new(ptr)
                    Handler_t(final_promise_, queue, std::forward<Args_t>(args)...);
        } catch (...) {
            real_alloc.deallocate(ptr, 1);
            throw;
        }

        cb_data_.callback_ = new_handler;

        std::unique_lock l(mtx_);
        for (auto &v : full_filled_) {
            Handler_t::do_push(queue, new_handler->cb_, std::move(v));
        }

        auto flags = state_.fetch_or(Stream_storage_state_ready_bit);
        l.unlock();

        if ((flags & Stream_storage_state_complete_bit) != 0) {
            cb_data_.callback_->complete();
        } else if ((flags & Stream_storage_state_fail_bit) != 0) {
            cb_data_.callback_->fail(std::move(error_));
        }
    }

}  // namespace melon::future_internal

#endif  // MELON_FUTURE_DETAIL_STREAM_STORAGE_IMPL_H_
