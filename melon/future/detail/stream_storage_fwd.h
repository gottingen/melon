
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_STREAM_STORAGE_FWD_H_
#define MELON_FUTURE_DETAIL_STREAM_STORAGE_FWD_H_

#include "melon/future/expected.h"

namespace melon::future_internal {

        template <typename... Ts>
        class stream_handler_interface {
        public:
            using full_fill_type = full_fill_type_t<Ts...>;
            using fail_type = fail_type_t<Ts...>;
            using finish_type = finish_type_t<Ts...>;

            stream_handler_interface() = default;
            stream_handler_interface(const stream_handler_interface&) = delete;
            stream_handler_interface(stream_handler_interface&&) = delete;
            stream_handler_interface& operator=(const stream_handler_interface&) = delete;
            stream_handler_interface& operator=(stream_handler_interface&&) = delete;
            virtual ~stream_handler_interface() {}

            // The future has been completed
            virtual void push(Ts...) = 0;
            virtual void complete() = 0;
            virtual void fail(fail_type) = 0;
        };

        template <typename QueueT, typename Enable = void, typename... Ts>
        class stream_handler_base : public stream_handler_interface<Ts...> {
        public:
            stream_handler_base(QueueT* q) : queue_(q) {}

        protected:
            QueueT* get_queue() { return queue_; }

        private:
            QueueT* queue_;
        };

// If the queue's push() method is static, do not bother storing the pointer.
        template <typename QueueT, typename... Ts>
        class stream_handler_base<QueueT, std::enable_if_t<has_static_push_v<QueueT>>,
        Ts...> : public stream_handler_interface<Ts...> {
        public:
        stream_handler_base(QueueT*) {}

        protected:
        constexpr static QueueT* get_queue() { return nullptr; }
    };

    constexpr std::uint8_t Stream_storage_state_ready_bit = 1;
    constexpr std::uint8_t Stream_storage_state_fail_bit = 2;
    constexpr std::uint8_t Stream_storage_state_complete_bit = 4;

    template <typename Alloc, typename... Ts>
    class stream_storage : public Alloc {
        stream_storage(const Alloc& alloc);

    public:
        ~stream_storage();
        using fail_type = std::exception_ptr;
        using full_fill_type = std::tuple<Ts...>;
        using full_fill_buffer_type = std::vector<full_fill_type>;
        using allocator_type = Alloc;

        void fail(fail_type&& e);

        template <typename... Us>
        void push(Us&&...);
        void complete();

        template <typename Handler_t, typename QueueT, typename... Args_t>
        void set_handler(QueueT* queue, Args_t&&... args);

        Alloc& allocator() { return *static_cast<Alloc*>(this); }
        const Alloc& allocator() const { return *static_cast<const Alloc*>(this); }

        basic_future<Alloc, void> get_final_future() {
            return basic_future<Alloc, void>{final_promise_};
        }

    private:
        struct Callback_data {
            // This will either point to sbo_buffer_, or heap-allocated data, depending
            // on state_.
            stream_handler_interface<Ts...>* callback_;
        };

        Callback_data cb_data_;

        std::mutex mtx_;
        full_fill_buffer_type full_filled_;
        std::exception_ptr error_;

        template <typename T>
        friend struct storage_ptr;

        storage_ptr<future_storage<Alloc, void>> final_promise_;

        std::atomic<std::uint8_t> state_ = 0;
        std::atomic<std::uint8_t> ref_count_ = 0;
    };
}  // namespace melon

#endif  // MELON_FUTURE_DETAIL_STREAM_STORAGE_FWD_H_
