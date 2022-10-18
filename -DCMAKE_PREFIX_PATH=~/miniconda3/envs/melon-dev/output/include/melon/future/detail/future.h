
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_FUTURE_H_
#define MELON_FUTURE_DETAIL_FUTURE_H_


#include "melon/future/detail/then.h"
#include "melon/future/detail/then_expect.h"
#include "melon/future/detail/then_finally_expect.h"

#include <future>

namespace melon {

    template<typename Alloc, typename... Ts>
    basic_future<Alloc, Ts...>::basic_future(future_internal::storage_ptr<storage_type> s)
            : storage_(std::move(s)) {}

    // Synchronously calls cb once the future has been fulfilled.
    // cb will be invoked directly in whichever thread fullfills
    // the future.
    //
    // returns a future of whichever type is returned by cb.
    // If this future is failed, then the resulting future
    // will be failed with that same failure, and cb will be destroyed without
    // being invoked.
    //
    // If you intend to discard the result, then you may want to use
    // finally() instead.
    template<typename Alloc, typename... Ts>
    template<typename CbT>
    [[nodiscard]] auto basic_future<Alloc, Ts...>::then(CbT &&cb) {
        // Kinda flying by the seats of our pants here...
        // We rely on the fact that `immediate_queue` handlers ignore the passed
        // queue.
        future_internal::immediate_queue queue;
        return this->then(queue, std::forward<CbT>(cb));
    }

    template<typename Alloc, typename... Ts>
    template<typename CbT>
    [[nodiscard]] auto basic_future<Alloc, Ts...>::then_expect(CbT &&cb) {
        // Kinda flying by the seats of our pants here...
        // We rely on the fact that `immediate_queue` handlers ignore the passed
        // queue.
        future_internal::immediate_queue queue;
        return this->then_expect(queue, std::forward<CbT>(cb));
    }

    template<typename Alloc, typename... Ts>
    template<typename CbT>
    void basic_future<Alloc, Ts...>::finally(CbT &&cb) {
        future_internal::immediate_queue queue;
        return this->finally(queue, std::forward<CbT>(cb));
    }

    // Queues cb in the target queue once the future has been fulfilled.
    //
    // It's expected that the queue itself will outlive the future.
    //
    // Returns a future of whichever type is returned by cb.
    // If this future is failed, then the resulting future
    // will be failed with that same failure, and cb will be destroyed without
    // being invoked.
    //
    // The assignment of the failure will be done synchronously in the fullfilling
    // thread.
    // TODO: Maybe we can add an option to change that behavior
    template<typename Alloc, typename... Ts>
    template<typename QueueT, typename CbT>
    [[nodiscard]] auto basic_future<Alloc, Ts...>::then(QueueT &queue, CbT &&cb) {
        using handler_t =
        future_internal::future_then_handler<Alloc, std::decay_t<CbT>, QueueT, Ts...>;
        using result_storage_t = typename handler_t::dst_storage_type;
        using result_fut_t = typename result_storage_t::future_type;

        future_internal::storage_ptr<result_storage_t> result;
        result.allocate(allocator());

        storage_->template set_handler<handler_t>(&queue, result, std::move(cb));
        storage_.reset();

        return result_fut_t(std::move(result));
    }

    template<typename Alloc, typename... Ts>
    template<typename QueueT, typename CbT>
    [[nodiscard]] auto basic_future<Alloc, Ts...>::then_expect(QueueT &queue,
                                                               CbT &&cb) {
        using handler_t = future_internal::future_then_expect_handler<Alloc, std::decay_t<CbT>,
                QueueT, Ts...>;
        using result_storage_t = typename handler_t::dst_storage_type;
        using result_fut_t = typename result_storage_t::future_type;

        future_internal::storage_ptr<result_storage_t> result;
        result.allocate(allocator());

        storage_->template set_handler<handler_t>(&queue, result, std::move(cb));
        storage_.reset();

        return result_fut_t(std::move(result));
    }

    template<typename Alloc, typename... Ts>
    template<typename QueueT, typename CbT>
    void basic_future<Alloc, Ts...>::finally(QueueT &queue, CbT &&cb) {
        assert(storage_);
        static_assert(std::is_invocable_v<CbT, expected<Ts, std::exception_ptr>...>,
                      "Finally should be accepting expected arguments");
        using handler_t =
        future_internal::future_finally_handler<std::decay_t<CbT>, QueueT, Ts...>;
        storage_->template set_handler<handler_t>(&queue, std::move(cb));

        storage_.reset();
    }

    template<typename Alloc, typename... Ts>
    auto basic_future<Alloc, Ts...>::std_future() {
        constexpr bool all_voids = std::conjunction_v<std::is_same<Ts, void>...>;
        if constexpr (all_voids) {
            std::promise<void> prom;
            auto fut = prom.get_future();
            this->finally([p = std::move(prom)](expected<Ts, std::exception_ptr>... vals) mutable {
                auto err = future_internal::get_first_error(vals...);
                if (err) {
                    p.set_exception(*err);
                } else {
                    p.set_value();
                }
            });
            return fut;
        } else if constexpr (sizeof...(Ts) == 1) {
            using T = std::tuple_element_t<0, std::tuple<Ts...>>;

            std::promise<T> prom;
            auto fut = prom.get_future();
            this->finally([p = std::move(prom)](expected<T, std::exception_ptr> v) mutable {
                if (v.has_value()) {
                    p.set_value(std::move(v.value()));
                } else {
                    p.set_exception(v.error());
                }
            });
            return fut;
        } else {
            using tuple_t = future_internal::full_fill_type_t<Ts...>;

            std::promise<tuple_t> p;
            auto fut = p.get_future();
            this->finally([p = std::move(p)](expected<Ts, std::exception_ptr>... v) mutable {
                auto err = future_internal::get_first_error(v...);
                if (err) {
                    p.set_exception(*err);
                } else {
                    p.set_value({v.value()...});
                }
            });
            return fut;
        }
    }

    template<typename Alloc, typename... Ts>
    typename basic_future<Alloc, Ts...>::value_type
    basic_future<Alloc, Ts...>::get() {
        return std_future().get();
    }

    template<typename Alloc, typename... Ts>
    Alloc &basic_future<Alloc, Ts...>::allocator() {
        assert(storage_);
        return storage_->allocator();
    }

    template<typename Alloc, typename... Ts>
    basic_future<Alloc, Ts...> flatten(
            basic_future<Alloc, std::tuple<Ts...>> &rhs) {
        using storage_type = typename basic_future<Alloc, Ts...>::storage_type;

        future_internal::storage_ptr<storage_type> storage;

        storage.allocate(rhs.allocator());

        rhs.finally([storage](expected<std::tuple<Ts...>, std::exception_ptr> e) mutable {
            if (e.has_value()) {
                storage->finish(std::move(*e));
            } else {
                storage->fail(std::move(e.error()));
            }
        });

        return basic_future<Alloc, Ts...>(std::move(storage));
    }

    /**
     * @brief Permits a callback to produce a higher-order future.
     *
     * @tparam Ts
     * @param args
     * @return auto
     */
    template<typename... Ts>
    auto segmented(Ts &&... args) {
        return future_internal::segmented_callback_result<std::decay_t<Ts>...>{
                std::make_tuple(std::forward<Ts>(args)...)};
    }

}  // namespace melon

#endif  // MELON_FUTURE_DETAIL_FUTURE_H_
