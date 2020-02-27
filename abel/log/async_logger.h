//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//
#ifndef ABEL_LOG_ASYNC_LOGGER_H_
#define ABEL_LOG_ASYNC_LOGGER_H_

// Very fast asynchronous logger (millions of logs per second on an average
// desktop)
// Uses pre allocated lockfree queue for maximum throughput even under large
// number of threads.
// Creates a single back thread to pop messages from the queue and log them.
//
// Upon each log write the logger:
//    1. Checks if its log level is enough to log the message
//    2. Push a new copy of the message to a queue (or block the caller until
//    space is available in the queue)
//    3. will throw log_ex upon log exceptions
// Upon destruction, logs all remaining messages in the queue before
// destructing..

#include <abel/log/common.h>
#include <abel/log/logger.h>

#include <chrono>
#include <memory>
#include <string>

namespace abel {
    namespace log {

// Async overflow policy - block by default.
        enum class async_overflow_policy {
            block,         // Block until message can be enqueued
            overrun_oldest // Discard oldest message in the queue if full when trying to
            // add new item.
        };

        namespace details {
            class thread_pool;
        }

        class async_logger ABEL_INHERITANCE_FINAL : public std::enable_shared_from_this<async_logger>, public logger {
            friend class details::thread_pool;

        public:
            template<typename It>
            async_logger(std::string logger_name, const It &begin, const It &end,
                         std::weak_ptr<details::thread_pool> tp,
                         async_overflow_policy overflow_policy = async_overflow_policy::block);

            async_logger(std::string logger_name, sinks_init_list sinks, std::weak_ptr<details::thread_pool> tp,
                         async_overflow_policy overflow_policy = async_overflow_policy::block);

            async_logger(std::string logger_name, sink_ptr single_sink, std::weak_ptr<details::thread_pool> tp,
                         async_overflow_policy overflow_policy = async_overflow_policy::block);

        protected:
            void sink_it_(details::log_msg &msg) override;

            void flush_() override;

            void backend_log_(details::log_msg &incoming_log_msg);

            void backend_flush_();

        private:
            std::weak_ptr<details::thread_pool> thread_pool_;
            async_overflow_policy overflow_policy_;
        };
    } //namespace log
} // namespace abel

#include "details/async_logger_impl.h"

#endif //ABEL_LOG_ASYNC_LOGGER_H_
