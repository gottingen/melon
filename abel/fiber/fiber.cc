//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/fiber.h"

#include <utility>
#include <vector>

#include "abel/base/profile.h"
#include "abel/base/random.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/internal/waitable.h"
#include "abel/fiber/fiber_context.h"
#include "abel/fiber/runtime.h"

namespace abel {

    namespace {

        fiber_internal::scheduling_group *get_scheduling_group(std::size_t id) {
            if (ABEL_LIKELY(id == fiber::kNearestSchedulingGroup)) {
                return fiber_internal::nearest_scheduling_group();
            } else if (id == fiber::kUnspecifiedSchedulingGroup) {
                return fiber_internal::routine_get_scheduling_group(
                        Random<std::size_t>(0, get_scheduling_group_count() - 1));
            } else {
                return fiber_internal::routine_get_scheduling_group(id);
            }
        }

    }  // namespace

    fiber::fiber() = default;

    fiber::~fiber() {
        DCHECK(!joinable(),
                   "You need to call either `join()` or `detach()` prior to destroy "
                   "a fiber.");
    }

    fiber::fiber(const attributes &attr, abel::function<void()> &&start) {
        // Choose a scheduling group for running this fiber.
        auto sg = get_scheduling_group(attr.scheduling_group);
        DCHECK(sg, "No scheduling group is available?");

        if (attr.execution_context) {
            // Caller specified an execution context, so we should wrap `start` to run
            // in the execution context.
            //
            // `ec` holds a reference to `attr.execution_context`, it's released when
            // `start` returns.
            start = [start = std::move(start),
                    ec = ref_ptr(ref_ptr_v, attr.execution_context)] {
                ec->execute(start);
            };
        }
        // `fiber` will cease to exist once `start` returns. We don't own it.
        //
        // FIXME: The fiber's stack is allocated on caller's NUMA node, it would be
        // beneficial if we can allocate if from the scheduling group it will run in.
        auto fiber =
                fiber_internal::create_fiber_entity(sg, attr.system_fiber, std::move(start));
        fiber->scheduling_group_local = attr.scheduling_group_local;

        // If `join()` is called, we'll sleep on this.
        fiber->ref_exit_barrier =
                get_ref_counted<fiber_internal::exit_barrier>();
        join_impl_ = fiber->ref_exit_barrier;

        // Schedule the fiber.
        if (attr.launch_policy == fiber_internal::Launch::Post) {
            sg->ready_fiber(fiber, {} /* Not needed. */);
        } else {
            sg->switch_to(fiber_internal::get_current_fiber_entity(), fiber);
        }
    }

    void fiber::detach() {
        DCHECK(joinable(), "The fiber is in detached state.");
        join_impl_ = nullptr;
    }

    void fiber::join() {
        DCHECK(joinable(), "The fiber is in detached state.");
        join_impl_->wait();
        join_impl_.reset();
    }

    bool fiber::joinable() const { return !!join_impl_; }

    fiber::fiber(fiber &&) noexcept = default;

    fiber &fiber::operator=(fiber &&) noexcept = default;

    void start_fiber_from_pthread(abel::function<void()> &&start_proc) {
        fiber_internal::start_fiber_detached(std::move(start_proc));
    }

    namespace fiber_internal {

        void start_fiber_detached(abel::function<void()> &&start_proc) {
            auto sg = fiber_internal::nearest_scheduling_group();
            auto fiber =
                    fiber_internal::create_fiber_entity(sg, false, std::move(start_proc));
            fiber->scheduling_group_local = false;
            sg->ready_fiber(fiber, {} /* Not needed. */);
        }

        void start_system_fiber_detached(abel::function<void()> &&start_proc) {
            auto sg = fiber_internal::nearest_scheduling_group();
            auto fiber =
                    fiber_internal::create_fiber_entity(sg, true, std::move(start_proc));
            fiber->scheduling_group_local = false;
            sg->ready_fiber(fiber, {} /* Not needed. */);
        }

        void start_fiber_detached(fiber::attributes &&attrs,
                                abel::function<void()> &&start_proc) {
            auto sg = get_scheduling_group(attrs.scheduling_group);

            if (attrs.execution_context) {
                start_proc = [start_proc = std::move(start_proc),
                        ec = ref_ptr(ref_ptr_v, std::move(attrs.execution_context))] {
                    ec->execute(start_proc);
                };
            }

            auto fiber = fiber_internal::create_fiber_entity(sg, attrs.system_fiber,
                                                             std::move(start_proc));
            fiber->scheduling_group_local = attrs.scheduling_group_local;

            if (attrs.launch_policy == fiber_internal::Launch::Post) {
                sg->ready_fiber(fiber, {} /* Not needed. */);
            } else {
                sg->switch_to(fiber_internal::get_current_fiber_entity(), fiber);
            }
        }

        void batch_start_fiber_detached(std::vector<abel::function<void()>> &&start_procs) {
            auto sg = fiber_internal::nearest_scheduling_group();

            std::vector<fiber_internal::fiber_entity *> fibers;
            for (auto &&e : start_procs) {
                auto fiber = fiber_internal::create_fiber_entity(sg, false, std::move(e));
                fiber->scheduling_group_local = false;
                fibers.push_back(fiber);
            }

            sg->start_fibers(fibers.data(), fibers.data() + fibers.size());
        }

    }  // namespace fiber_internal

}  // namespace abel

