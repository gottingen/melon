//
// Copyright(c) 2018 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <abel/log/details/null_mutex.h>
#include <abel/log/sinks/base_sink.h>
#include <mutex>
#include <thread>

namespace abel {
    namespace sinks {

        template<class Mutex>
        class test_sink : public abel::log::sinks::base_sink<Mutex> {
        public:
            size_t msg_counter() {
                return msg_counter_;
            }

            size_t flush_counter() {
                return flush_counter_;
            }

            void set_delay(std::chrono::milliseconds delay) {
                delay_ = delay;
            }

        protected:
            void sink_it_(const abel::log::details::log_msg &) override {
                msg_counter_++;
                std::this_thread::sleep_for(delay_);
            }

            void flush_() override {
                flush_counter_++;
            }

            size_t msg_counter_{0};
            size_t flush_counter_{0};
            std::chrono::milliseconds delay_{std::chrono::milliseconds::zero()};
        };

        using test_sink_mt = test_sink<std::mutex>;
        using test_sink_st = test_sink<abel::log::details::null_mutex>;

    } // namespace sinks
} // namespace abel
