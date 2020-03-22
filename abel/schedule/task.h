//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_SCHEDULE_TASK_H_
#define ABEL_SCHEDULE_TASK_H_

#include <memory>
#include <schedule_group.h>

namespace abel {

    class task {
    public:
        explicit task(schedule_group sg = current_schedule_group()) : _sg(sg) {}

        virtual ~task() noexcept {}

        virtual void run_and_dispose() noexcept = 0;

        schedule_group group() const { return _sg; }

    private:
        schedule_group _sg;
    };

    void schedule(std::unique_ptr<task> t);

    void schedule_urgent(std::unique_ptr<task> t);


    template<typename Func>
    class lambda_task final : public task {
        Func _func;
    public:
        lambda_task(schedule_group sg, const Func &func) : task(sg), _func(func) {}

        lambda_task(schedule_group sg, Func &&func) : task(sg), _func(std::move(func)) {}

        virtual void run_and_dispose() noexcept override {
            _func();
            delete this;
        }
    };

    template<typename Func>
    ABEL_FORCE_INLINE
    std::unique_ptr<task>
    make_task(Func &&func) {
        return std::make_unique<lambda_task<Func>>(current_schedule_group(), std::forward<Func>(func));
    }

    template<typename Func>
    ABEL_FORCE_INLINE
    std::unique_ptr<task>
    make_task(schedule_group sg, Func &&func) {
        return std::make_unique<lambda_task<Func>>(sg, std::forward<Func>(func));
    }

} //namespace abel

#endif //ABEL_SCHEDULE_TASK_H_
