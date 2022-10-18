
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_STREAM_FUTURE_H_
#define MELON_FUTURE_STREAM_FUTURE_H_


#include "melon/future/future.h"

#include "melon/future/detail/stream_storage_fwd.h"

#include <memory>

namespace melon {

    template<typename Alloc, typename... Ts>
    class basic_stream_promise;

    /**
     * @brief Represents a stream of values that will be eventually available.
     *
     * @tparam Alloc The rebindable allocator to use.
     * @tparam Ts The types making up the stream fields.
     */
    template<typename Alloc, typename... Ts>
    class basic_stream_future {
    public:
        /// The underlying storage type.
        using storage_type = future_internal::stream_storage<Alloc, Ts...>;
        using full_fill_type = typename storage_type::full_fill_type;

        /**
         * @brief Default construction.
         *
         */
        basic_stream_future() = default;

        /**
         * @brief Move construction.
         *
         */
        basic_stream_future(basic_stream_future &&) = default;

        /**
         * @brief Move assignment.
         *
         * @return basic_stream_future&
         */
        basic_stream_future &operator=(basic_stream_future &&) = default;

        /**
         * @brief Invokes a callback on each value in the stream wherever they are
         *        produced.
         *
         * @tparam CbT
         * @param cb The callback to invoke on each value.
         * @return Basic_future<Alloc, void> A future that will be completed at the
         *                                   end of the stream.
         */
        template<typename CbT>
        [[nodiscard]] basic_future<Alloc, void> for_each(CbT &&cb);

        /**
         * @brief Posts the execution of a callback to a queue when values are
         *        produced.
         *
         * @tparam QueueT
         * @tparam CbT
         * @param queue cb will be posted to that queue
         * @param cb the callback to invoke.
         * @return Basic_future<Alloc, void> A future that will be completed at the
         *                                   end of the stream.
         */
        template<typename QueueT, typename CbT>
        [[nodiscard]] basic_future<Alloc, void> for_each(QueueT &queue, CbT &&cb);

    private:
        template<typename SubAlloc, typename... Us>
        friend
        class basic_stream_promise;

        explicit basic_stream_future(future_internal::storage_ptr<storage_type> s);

        future_internal::storage_ptr<storage_type> storage_;
    };

    /**
     * @brief basic_stream_future with default allocator.
     *
     * @tparam Ts
     */
    template<typename... Ts>
    using stream_future = basic_stream_future<std::allocator<void>, Ts...>;

    /**
     * @brief
     *
     * @tparam Alloc
     * @tparam Ts
     */
    template<typename Alloc, typename... Ts>
    class basic_stream_promise {
    public:
        using future_type = basic_stream_future<Alloc, Ts...>;
        using storage_type = typename future_type::storage_type;
        using full_fill_type = typename storage_type::full_fill_type;
        using fail_type = std::exception_ptr;

        /**
         * @brief Construct a new basic_stream_promise object.
         *
         */
        basic_stream_promise();

        /**
         * @brief Construct a new basic_stream_promise object
         *
         */
        basic_stream_promise(basic_stream_promise &&) = default;

        /**
         * @brief
         *
         * @return basic_stream_promise&
         */
        basic_stream_promise &operator=(basic_stream_promise &&) = default;

        /**
         * @brief Destroy the basic_stream_promise object
         *
         */
        ~basic_stream_promise();

        /**
         * @brief Get the future object
         *
         * @param alloc
         * @return future_type
         */
        future_type get_future(const Alloc &alloc = Alloc());

        /**
         * @brief Add a datapoint to the stream.
         *
         * @tparam Us
         */
        template<typename... Us>
        void push(Us &&...);

        /**
         * @brief Closes the stream.
         *
         */
        void complete();

        /**
         * @brief Notify failure of the stream.
         *
         */
        void set_exception(fail_type);

        /**
         * @brief returns wether the promise still refers to an uncompleted future
         *
         * @return true
         * @return false
         */
        operator bool() const;

    private:
        future_internal::storage_ptr<storage_type> storage_;

        basic_stream_promise(const basic_stream_promise &) = delete;

        basic_stream_promise &operator=(const basic_stream_promise &) = delete;
    };

    /**
     * @brief
     *
     * @tparam Ts
     */
    template<typename... Ts>
    using stream_promise = basic_stream_promise<std::allocator<void>, Ts...>;
}  // namespace melon

#include "melon/future/detail/stream_future.h"
#include "melon/future/detail/stream_promise.h"
#include "melon/future/detail/stream_storage_impl.h"


#endif  // MELON_FUTURE_STREAM_FUTURE_H_
