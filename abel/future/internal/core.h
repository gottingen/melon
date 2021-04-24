//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_CORE_H_
#define ABEL_FUTURE_INTERNAL_CORE_H_


#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "abel/functional/function.h"
#include "abel/future/internal/boxed.h"
#include "abel/future/internal/executor.h"
#include "abel/log/logging.h"

namespace abel {
    namespace future_internal {

        // This is the shared state between `Promise<...>` and `Future<...>`.
        //
        // `future_core` itself will do necessary synchronizations to be multithread safe.
        //
        // Unfortunately `intrusive_ptr` is not in STL yet (@sa P0468R0), thus we're
        // using `std::shared_ptr<...>` to hold it.
        //
        // Seastar rolled their own `lw_shared_ptr`, which I believe removed some
        // abilities provided by STL such as custom deleter, and thus more lightweight.
        //
        // We don't need those abilities neither, so it might be better for us to
        // roll our own `lw_shared_ptr` as well once the overhead of these features
        // become a bottleneck;
        template<class... Ts>
        class future_core {
        public:
            using value_type = boxed<Ts...>;
            using action_type = abel::function<void(value_type &&) noexcept>;

            // Construct a `future_core` using `executor`.
            explicit future_core(executor executor) : executor_(std::move(executor)) {}

            // Satisfy the `future_core` with the boxed value.
            //
            // Precondition: The `future_core` must have not been satisfied.
            void set_boxed(value_type &&value) noexcept;

            // Chain an action. It might be immediately invoked if the `future_core` has
            // already been satisfied.
            //
            // At most 1 action may ever be chained for a given `future_core`.
            void chain_action(action_type &&action) noexcept;

            // Get executor used when invoking the continuation.
            executor get_executor() const noexcept;

        private:
            // `future_core` is not satisfied, the `continuation` (if any) is stored in the
            // state.
            struct waiting_for_single_object {
                action_type on_satisfied;
            };
            // `future_core` is satisfied, the value satisfied it is stored.
            struct satisfied {
                value_type value;
                bool ever_called_continuation{false};
            };

            // I personally **think** a spin lock would work better here as there's
            // hardly any contention on a given `future_core` likely to occur.
            //
            // But pthread's mutex does spin for some rounds before trapping, so I
            // *think* it should work just equally well from performance perspective.
            //
            // Not sure if HLE (Hardware Lock Elision) helps here, but it should.
            std::mutex lock_;
            std::variant<waiting_for_single_object, satisfied> state_;
            executor executor_;
        };

// Implementation goes below.

        template<class... Ts>
        void future_core<Ts...>::set_boxed(value_type &&value) noexcept {
            std::unique_lock<std::mutex> lk(lock_);

            // Transit to `satisfied`.
            action_type action =
                    std::move(std::get<waiting_for_single_object>(state_).on_satisfied);
            state_.template emplace<satisfied>(satisfied{std::move(value), !!action});

            if (action) {
                // The `future_core` has reached the final state, and won't be mutate any more
                // (except for its destruction), thus unlocking here is safe.
                lk.unlock();
                executor_.execute(
                        [a = std::move(action),
                                v = std::move(std::get<satisfied>(state_).value)]() mutable noexcept {
                            a(std::move(v));
                        });
            }  // Otherwise the lock will be release automatically.
        }

        template<class... Ts>
        void future_core<Ts...>::chain_action(action_type &&action) noexcept {
            std::unique_lock<std::mutex> lk(lock_);

            if (state_.index() == 0) {  // We're still waiting for value.
                state_.template emplace<waiting_for_single_object>(
                        waiting_for_single_object{std::move(action)});
            } else {  // Already satisfied, call the `action` immediately.
                DCHECK_EQ(state_.index(), 1);
                auto &&s = std::get<satisfied>(state_);

                DCHECK_MSG(!s.ever_called_continuation,
                            "Action may not be chained for multiple times.");

                s.ever_called_continuation = true;
                lk.unlock();  // Final state reached.

                executor_.execute(
                        [a = std::move(action), v = std::move(s.value)]() mutable noexcept {
                            a(std::move(v));
                        });
            }
        }

        template<class... Ts>
        inline executor future_core<Ts...>::get_executor() const noexcept {
            return executor_;
        }

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_CORE_H_
