//

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <abel/thread/thread_annotations.h>
#include <abel/thread/mutex.h>

namespace abel {

    namespace thread_internal {

// A simple thread_pool implementation for tests.
        class thread_pool {
        public:
            explicit thread_pool(int num_threads) {
                for (int i = 0; i < num_threads; ++i) {
                    threads_.push_back(std::thread(&thread_pool::work_loop, this));
                }
            }

            thread_pool(const thread_pool &) = delete;

            thread_pool &operator=(const thread_pool &) = delete;

            ~thread_pool() {
                {
                    abel::mutex_lock l(&mu_);
                    for (size_t i = 0; i < threads_.size(); i++) {
                        queue_.push(nullptr);  // Shutdown signal.
                    }
                }
                for (auto &t : threads_) {
                    t.join();
                }
            }

            // schedule a function to be run on a thread_pool thread immediately.
            void schedule(std::function<void()> func) {
                assert(func != nullptr);
                abel::mutex_lock l(&mu_);
                queue_.push(std::move(func));
            }

        private:
            bool work_available() const ABEL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
                return !queue_.empty();
            }

            void work_loop() {
                while (true) {
                    std::function<void()> func;
                    {
                        abel::mutex_lock l(&mu_);
                        mu_.await(abel::condition(this, &thread_pool::work_available));
                        func = std::move(queue_.front());
                        queue_.pop();
                    }
                    if (func == nullptr) {  // Shutdown signal.
                        break;
                    }
                    func();
                }
            }

            abel::mutex mu_;
            std::queue<std::function<void()>> queue_ ABEL_GUARDED_BY(mu_);
            std::vector<std::thread> threads_;
        };

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_
