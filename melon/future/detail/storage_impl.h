
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_STORAGE_IMPL_H_
#define MELON_FUTURE_DETAIL_STORAGE_IMPL_H_


#include "melon/future/detail/storage_fwd.h"
#include "melon/future/detail/utility.h"
#include <type_traits>

namespace melon::future_internal {

    template<typename Alloc, typename... Ts>
    future_storage<Alloc, Ts...>::future_storage(const Alloc &alloc)
            : Alloc(alloc) {}

    template<typename Alloc, typename... Ts>
    void future_storage<Alloc, Ts...>::full_fill(full_fill_type &&v) {
        auto prev_state = state_.load();

        if (prev_state & Future_storage_state_ready_bit) {
            cb_data_.callback_->full_fill(std::move(v));
        } else {
            // This is expected to be fairly rare...
            new(&finished_)
                    finish_type(full_fill_to_finish<0, 0, finish_type>(std::move(v)));
            prev_state = state_.fetch_or(Future_storage_state_finished_bit);

            // Handle the case where a handler was added just in time.
            // This should be extremely rare.
            if (prev_state & Future_storage_state_ready_bit) {
                cb_data_.callback_->finish(std::move(finished_));
            }
        }
    }

    template<typename Alloc, typename... Ts>
    template<typename... Us>
    void future_storage<Alloc, Ts...>::full_fill(
            segmented_callback_result<Us...> &&f) {
        full_fill(std::move(f.values_));
    }

    template<typename Alloc, typename... Ts>
    template<typename Arg_alloc>
    void future_storage<Alloc, Ts...>::full_fill(
            basic_future<Arg_alloc, Ts...> &&f) {
        storage_ptr <future_storage<Alloc, Ts...>> self(this);
        f.finally([self](expected <Ts, std::exception_ptr>... f) {
            self->finish(std::make_tuple(std::move(f)...));
        });
    }

    template<typename Alloc, typename... Ts>
    void future_storage<Alloc, Ts...>::finish(finish_type &&f) {
        auto prev_state = state_.load();

        if (prev_state & Future_storage_state_ready_bit) {
            // THis should be the likelyest scenario.
            cb_data_.callback_->finish(std::move(f));
            // No need to set the finished bit.
        } else {
            // This is expected to be fairly rare...
            new(&finished_) finish_type(std::move(f));
            prev_state = state_.fetch_or(Future_storage_state_finished_bit);

            // Handle the case where a handler was added just in time.
            // This should be extremely rare.
            if (prev_state & Future_storage_state_ready_bit) {
                cb_data_.callback_->finish(std::move(finished_));
            }
        }
    }

    template<typename Alloc, typename... Ts>
    template<typename Arg_alloc>
    void future_storage<Alloc, Ts...>::finish(basic_future<Arg_alloc, Ts...> &&f) {
        f.finally([this](expected <Ts, std::exception_ptr>... f) {
            this->finish(std::make_tuple(std::move(f)...));
        });
    }

    template<typename Alloc, typename... Ts>
    void future_storage<Alloc, Ts...>::fail(fail_type &&e) {
        auto prev_state = state_.load();

        if (prev_state & Future_storage_state_ready_bit) {
            cb_data_.callback_->finish(
                    fail_to_expect < 0, std::tuple<expected < Ts, std::exception_ptr>... >> (e));
        } else {
            // This is expected to be fairly rare...
            new(&finished_) finish_type(fail_to_expect<0, finish_type>(e));
            prev_state = state_.fetch_or(Future_storage_state_finished_bit);

            // Handle the case where a handler was added just in time.
            // This should be extremely rare.
            if (prev_state & Future_storage_state_ready_bit) {
                cb_data_.callback_->finish(
                        fail_to_expect < 0, std::tuple<expected <Ts, std::exception_ptr>... >> (e));
            }
        }
    }

    template<typename Alloc, typename... Ts>
    template<typename Handler_t, typename QueueT, typename... Args_t>
    void future_storage<Alloc, Ts...>::set_handler(QueueT *queue,
                                                   Args_t &&... args) {
        assert(cb_data_.callback_ == nullptr);

        using alloc_traits = std::allocator_traits<Alloc>;
        using Real_alloc = typename alloc_traits::template rebind_alloc<Handler_t>;

        Real_alloc real_alloc(allocator());
        auto ptr = real_alloc.allocate(1);
        try {
            cb_data_.callback_ =
                    new(ptr) Handler_t(queue, std::forward<Args_t>(args)...);
        } catch (...) {
            real_alloc.deallocate(ptr, 1);
            throw;
        }

        auto prev_state = state_.fetch_or(Future_storage_state_ready_bit);
        if ((prev_state & Future_storage_state_finished_bit) != 0) {
            // This is unlikely...
            cb_data_.callback_->finish(std::move(finished_));
        }
    }

    template<typename Alloc, typename... Ts>
    future_storage<Alloc, Ts...>::~future_storage() {
        auto state = state_.load();

        if (state & Future_storage_state_ready_bit) {
            assert(cb_data_.callback_ != nullptr);

            cb_data_.callback_->~future_handler_interface<Ts...>();
            using alloc_traits = std::allocator_traits<Alloc>;
            using real_alloc = typename alloc_traits::template rebind_alloc<
                    future_handler_interface < Ts...>>;

            real_alloc alloc(allocator());
            alloc.deallocate(cb_data_.callback_, 1);
        }

        if (state & Future_storage_state_finished_bit) {
            finished_.~finish_type();
        }
    }

}  // namespace melon::future_internal
#endif  // MELON_FUTURE_DETAIL_STORAGE_IMPL_H_
