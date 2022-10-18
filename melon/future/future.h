
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_FUTURE_H_
#define MELON_FUTURE_FUTURE_H_

#include "melon/future/detail/storage_fwd.h"

#include <memory>
#include <string>

namespace melon {

    template <typename Alloc, typename... Ts>
    class basic_promise;

    /**
     * @brief Values that will be eventually available
     *
     * @tparam Ts
     * fields set to `void` have special rules applied to them.
     * The following types may not be used:
     * - `expected<T>`
     * - `future<Ts...>`
     * - `promise<T>`
     *
     * @invariant A future is in one of two states:
     * - \b Uninitialized: The only legal operation is to assign another future to
     * it.
     * - \b Ready: All operations are legal.
     */
    template <typename Alloc, typename... Ts>
    class basic_future {
        static_assert(sizeof...(Ts) >= 1, "you probably meant future<void>");

    public:
        using promise_type = basic_promise<Alloc, Ts...>;

        /// The underlying storage type.
        using storage_type = future_internal::future_storage<Alloc, Ts...>;

        /// Allocator
        using allocator_type = Alloc;

        /// Ts...
        using value_type = future_internal::future_value_type_t<Ts...>;
        using full_fill_type = future_internal::full_fill_type_t<Ts...>;
        using finish_type = future_internal::finish_type_t<Ts...>;

        /**
         * @brief Construct an \b uninitialized future
         *
         * @post `this` will be \b uninitialized
         */
        basic_future() = default;

        /**
         * @brief Move constructor
         *
         * @param rhs
         *
         * @post `rhs` will be \b uninitialized
         */
        basic_future(basic_future&& rhs) = default;

        /**
         * @brief Move assignment
         *
         * @param rhs
         * @return future&
         *
         * @post `rhs` will be \b uninitialized
         */
        basic_future& operator=(basic_future&& rhs) = default;

        /**
         * @brief Creates a future that is finished by the invocation of cb when this
         *        is full_filled.
         *
         * @tparam CbT
         * @param callback
         * @return future<decltype(callback(Ts...))> a \ready future
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        template <typename CbT>
        [[nodiscard]] auto then(CbT&& callback);

        /**
         * @brief Creates a future that is finished by the invocation of cb when this
         *        is finished.
         *
         * @tparam QueueT
         * @tparam CbT
         * @param queue
         * @param callback
         * @return auto
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        template <typename QueueT, typename CbT>
        [[nodiscard]] auto then(QueueT& queue, CbT&& callback);

        /**
         * @brief Creates a future that is finished by the invocation of cb when this
         *        is finished.
         *
         * @tparam CbT
         * @param callback
         * @return auto
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        template <typename CbT>
        [[nodiscard]] auto then_expect(CbT&& callback);

        /**
         * @brief Creates a future that is finished by the invocation of cb from
         *        queue when this is finished.
         *
         * @tparam QueueT
         * @tparam CbT
         * @param queue
         * @param callback
         * @return auto
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        template <typename QueueT, typename CbT>
        [[nodiscard]] auto then_expect(QueueT& queue, CbT&& callback);

        /**
         * @brief Invokes cb when this is finished.
         *
         * @tparam CbT
         * @param callback
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        template <typename CbT>
        void finally(CbT&& callback);

        /**
         * @brief Invokes cb from queue when this is finished.
         *
         * @tparam QueueT
         * @tparam CbT
         * @param queue
         * @param callback
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        template <typename QueueT, typename CbT>
        void finally(QueueT& queue, CbT&& callback);

        /**
         * @brief Blocks until the future is finished, and then either return the
         *        value, or throw the error
         *
         * @return auto
         *
         * @pre the future must be \b ready
         * @post the future will be \b uninitialized
         */
        value_type get();

        /**
         * @brief Obtain a std::future bound to this future.
         *
         * @return auto
         */
        auto std_future();

        /**
         * @brief Get the allocator associated with this future.
         *
         * @pre The future must have shared storage associated with it.
         *
         * @return Alloc&
         */
        allocator_type& allocator();

        /**
         * @brief Create a future directly from its underlying storage.
         */
        explicit basic_future(future_internal::storage_ptr<storage_type> s);

    private:
        future_internal::storage_ptr<storage_type> storage_;
    };

    template <typename... Ts>
    using future = basic_future<std::allocator<void>, Ts...>;

    /**
     * @brief Error assigned to a future who's promise is destroyed before being
     *        finished.
     *
     */
    struct unfull_filled_promise : public std::logic_error {
        unfull_filled_promise() : std::logic_error("Unfull_filled_promise") {}
    };

    /**
     * @brief Landing for a value that finishes a future.
     *
     * @tparam Ts
     */
    template <typename Alloc, typename... Ts>
    class basic_promise {
        static_assert(sizeof...(Ts) >= 1, "you probably meant Promise<void>");

    public:
        using future_type = basic_future<Alloc, Ts...>;
        using storage_type = typename future_type::storage_type;

        using value_type = future_internal::future_value_type_t<Ts...>;

        using full_fill_type = future_internal::full_fill_type_t<Ts...>;
        using finish_type = future_internal::finish_type_t<Ts...>;
        using fail_type = future_internal::fail_type_t<Ts...>;

        basic_promise(const Alloc& alloc = Alloc());
        basic_promise(basic_promise&&) = default;
        basic_promise& operator=(basic_promise&&) = default;
        ~basic_promise();

        /**
         * @brief Get the future object
         *
         * @param alloc The allocator to use when creating the interal state
         * @return future_type
         */
        future_type get_future();

        /**
         * @brief Fullfills the promise
         *
         * @tparam Us
         * @param values
         */
        template <typename... Us>
        void set_value(Us&&... values);

        /**
         * @brief Finishes the promise
         *
         * @tparam Us
         * @param expecteds
         */
        template <typename... Us>
        void finish(Us&&... expecteds);

        /**
         * @brief Fails the promise
         *
         * @param error
         */
        void set_exception(fail_type error);

        /**
         * @brief returns wether the promise still refers to an uncompleted future
         *
         * @return true
         * @return false
         */
        operator bool() const;

    private:
        bool future_created_ = false;
        bool value_assigned_ = false;

        future_internal::storage_ptr<storage_type> storage_;

        basic_promise(const basic_promise&) = delete;
        basic_promise& operator=(const basic_promise&) = delete;
    };

    template <typename... Ts>
    using promise = basic_promise<std::allocator<void>, Ts...>;
    /**
     * @brief Ties a set of future<> into a single future<> that is finished once
     *        all child futures are finished.
     *
     * @tparam FutTs
     * @param futures
     * @return auto
     */
    template <typename... FutTs>
    auto join(FutTs&&... futures);

    // Convenience function that creates a promise for the result of the cb, pushes
    // cb in q, and returns a future to that promise.

    /**
     * @brief Posts a callback into to queue, and return a future that will be
     *        finished upon executaiton of the callback.
     *
     * @tparam QueueT
     * @tparam CbT
     * @param q
     * @param callback
     * @return auto
     */
    template <typename QueueT, typename CbT>
    auto async(QueueT& q, CbT&& callback);

    /**
     * @brief Create a higher-order future from a `future<tuple>`
     *
     * @param rhs
     *
     * @post `rhs` will be \b uninitialized.
     * @post this will inherit the state `rhs` was in.
     */
    template <typename Alloc, typename... Ts>
    basic_future<Alloc, Ts...> flatten(basic_future<Alloc, std::tuple<Ts...>>& rhs);

    /**
     * @brief Used to let a callback return a higher-order future.
     *
     * @tparam Ts
     * @param args
     * @return auto
     */
    template <typename... Ts>
    auto segmented(Ts&&... args);

}  // namespace melon

#include "melon/future/detail/async.h"
#include "melon/future/detail/future.h"
#include "melon/future/detail/join.h"
#include "melon/future/detail/promise.h"
#include "melon/future/detail/storage_impl.h"

#endif  // MELON_FUTURE_FUTURE_H_
