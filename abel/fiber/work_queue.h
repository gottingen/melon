//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_WORK_QUEUE_H
#define ABEL_FIBER_WORK_QUEUE_H


#include <queue>

#include "abel/functional/function.h"
#include "abel/fiber/fiber_cond.h"
#include "abel/fiber/fiber.h"
#include "abel/fiber/fiber_mutex.h"

namespace abel {

    // Each work queue consists of a dedicated fiber for running jobs.
    //
    // Work posted to this queue is run in a FIFO fashion.
    class work_queue {
    public:
        work_queue();

        // Scheduling `cb` for execution.
        void Push(abel::function<void()>&& cb);

        // Stop the queue.
        void Stop();

        // Wait until all pending works has run.
        void Join();

    private:
        void WorkerProc();

    private:
        fiber worker_;
        fiber_mutex lock_;
        fiber_cond cv_;
        std::queue<abel::function<void()>> jobs_;
        bool stopped_ = false;
    };

}  // namespace abel

#endif  // ABEL_FIBER_WORK_QUEUE_H
