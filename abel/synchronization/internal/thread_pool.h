//

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <abel/base/thread_annotations.h>
#include <abel/synchronization/mutex.h>

namespace abel {

namespace synchronization_internal {

// A simple ThreadPool implementation for tests.
class ThreadPool {
 public:
  explicit ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
      threads_.push_back(std::thread(&ThreadPool::WorkLoop, this));
    }
  }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  ~ThreadPool() {
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

  // Schedule a function to be run on a ThreadPool thread immediately.
  void Schedule(std::function<void()> func) {
    assert(func != nullptr);
    abel::mutex_lock l(&mu_);
    queue_.push(std::move(func));
  }

 private:
  bool WorkAvailable() const ABEL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
    return !queue_.empty();
  }

  void WorkLoop() {
    while (true) {
      std::function<void()> func;
      {
        abel::mutex_lock l(&mu_);
        mu_.Await(abel::condition(this, &ThreadPool::WorkAvailable));
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

}  // namespace synchronization_internal

}  // namespace abel

#endif  // ABEL_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_
