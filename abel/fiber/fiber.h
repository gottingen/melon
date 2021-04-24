//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_FIBER_H_
#define ABEL_FIBER_FIBER_H_


#include <limits>
#include <utility>
#include <vector>

#include "abel/functional/function.h"
#include "abel/memory/ref_ptr.h"

namespace abel {

    namespace fiber_internal {

        class exit_barrier;

    }  // namespace fiber_internal

    namespace fiber_internal {

        enum class Launch {
            Post,
            Dispatch  // If possible, yield current pthread worker to user's code.
        };

    }  // namespace fiber
    class fiber_context;

    // Analogous to `std::thread`, but it's for fiber.
    //
    // Directly constructing `fiber` does NOT propagate execution context. Consider
    // using `fiber_async` instead.
    class fiber {
    public:
        // Hopefully you don't start 2**64 scheduling groups.
        static constexpr auto kNearestSchedulingGroup =
                std::numeric_limits<std::size_t>::max() - 1;
        static constexpr auto kUnspecifiedSchedulingGroup =
                std::numeric_limits<std::size_t>::max();

        struct attributes {
            // How the fiber is launched.
            fiber_internal::Launch launch_policy = fiber_internal::Launch::Post;

            // Which scheduling group should the fiber be *initially* placed in. Note
            // that unless you also have `scheduling_group_local` set, the fiber can be
            // stolen by workers belonging to other scheduling group.
            std::size_t scheduling_group = kNearestSchedulingGroup;

            // If set, fiber's start procedure is run in this execution context.
            //
            // `fiber` will take a reference to the execution context, so you're safe to
            // release your own ref. once `fiber` is constructed.
            fiber_context *execution_context = nullptr;

            // If set, this fiber is treated as system fiber. Certain restrictions may
            // apply to system fiber (e.g., stack size.).
            //
            // This flag is reserved for internal use only.
            bool system_fiber = false;

            // If set, `scheduling_group` is enforced. (i.e., work-stealing is disabled
            // on this fiber.)
            bool scheduling_group_local = false;

            // TODO(yinbinli): `bool start_in_detached_fashion`. If set, the fiber is
            // immediately detached once created. This provide us further optimization
            // possibility.

        };

        // Create an empty (invalid) fiber.
        fiber();

        // Create a fiber with attributes. It will run from `start`.
        fiber(const attributes &attr, abel::function<void()> &&start);

        // Create fiber by calling `f` with args.
        template<class F, class... Args,
                class = std::enable_if_t<std::is_invocable_v<F &&, Args &&...>>>
        explicit fiber(F &&f, Args &&... args)
                : fiber(attributes(), std::forward<F>(f), std::forward<Args>(args)...) {}

        // Create fiber by calling `f` with args, using policy `policy`.
        template<class F, class... Args,
                class = std::enable_if_t<std::is_invocable_v<F &&, Args &&...>>>
        fiber(fiber_internal::Launch policy, F &&f, Args &&... args)
                : fiber(attributes{.launch_policy = policy}, std::forward<F>(f),
                        std::forward<Args>(args)...) {}

        // Create fiber by calling `f` with args, using attributes `attr`.
        template<class F, class... Args,
                class = std::enable_if_t<std::is_invocable_v<F &&, Args &&...>>>
        fiber(const attributes &attr, F &&f, Args &&... args)
                : fiber(attr, [f = std::forward<F>(f),
                // P0780R2 is not implemented as of now (GCC 8.2).
                t = std::tuple(std::forward<Args>(args)...)] {
            std::apply(std::forward<F>(f)(std::move(t)));
        }) {}

        // Special case if no parameter is passed to `F`, in this case we don't need
        // an indirection (the extra lambda).
        template<class F, class = std::enable_if_t<std::is_invocable_v<F &&>>>
        fiber(const attributes &attr, F &&f)
                : fiber(attr, abel::function<void()>(std::forward<F>(f))) {}

        // If a `fiber` object which owns a fiber is destructed with no prior call to
        // `join()` or `detach()`, it leads to abort.
        ~fiber();

        // Detach the fiber.
        void detach();

        // Wait for the fiber to exit.
        void join();

        // Test if we can call `join()` on this object.
        bool joinable() const;

        // Movable but not copyable
        fiber(fiber &&) noexcept;

        fiber &operator=(fiber &&) noexcept;

    private:
        ref_ptr <fiber_internal::exit_barrier> join_impl_;
    };

    // In certain cases you may want to start a fiber from pthread environment, so
    // that your code can use fiber primitives. This method helps you accomplish
    // this.
    void start_fiber_from_pthread(abel::function<void()> &&start_proc);

    namespace fiber_internal {

        // Start a new fiber in "detached" state. This method performs better than
        // `fiber(...).detach()` in trade of simple interface.
        //
        // Introduced for perf. reasons, for internal use only.
        void start_fiber_detached(abel::function<void()> &&start_proc);

        void start_system_fiber_detached(abel::function<void()> &&start_proc);

        void start_fiber_detached(fiber::attributes &&attrs,
                                abel::function<void()> &&start_proc);

        // Start fibers in batch, in "detached" state.
        void batch_start_fiber_detached(std::vector<abel::function<void()>> &&start_procs);

    }  // namespace fiber_internal

}  // namespace abel


#endif  // ABEL_FIBER_FIBER_H_
