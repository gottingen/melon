//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/work_queue.h"

#include <mutex>
#include <utility>

#include "abel/log/logging.h"

namespace abel {

    work_queue::work_queue() {
        worker_ = fiber([this] { worker_proc(); });
    }

    void work_queue::push(abel::function<void()>&& cb) {
        std::scoped_lock _(lock_);
        DCHECK_MSG(!stopped_, "The work queue is leaving.");
        jobs_.push(std::move(cb));
        cv_.notify_one();
    }

    void work_queue::stop() {
        std::scoped_lock _(lock_);
        stopped_ = true;
        cv_.notify_one();
    }

    void work_queue::join() { worker_.join(); }

    void work_queue::worker_proc() {
        while (true) {
            std::unique_lock lk(lock_);
            cv_.wait(lk, [&] { return stopped_ || !jobs_.empty(); });

            // So long as there still are pending jobs, we keep running.
            if (jobs_.empty()) {
                DCHECK(stopped_);
                break;
            }

            // We move all pending jobs out to reduce lock contention.
            auto temp = std::move(jobs_);
            lk.unlock();
            while (!temp.empty()) {
                auto&& job = temp.front();
                job();
                temp.pop();
            }
        }
    }

}  // namespace abel
