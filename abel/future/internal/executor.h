//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_EXECUTOR_H_
#define ABEL_FUTURE_INTERNAL_EXECUTOR_H_


#include <atomic>
#include <memory>
#include <utility>

#include "abel/functional/function.h"

namespace abel {

    namespace future_internal {

        // This class is a lightweight polymorphic wrapper for execution contexts,
        // which are responsible for executing jobs posted to them.
        //
        // Both this wrapper and the wrappee is (required) to be CopyConstructible.
        //
        // Our design follows how p0443r7 declares `std::executor`: Exactly the same
        // way as how `std::function` wraps `Callable`.
        //
        // This design suffers from slightly bad copy performance (if it's critical
        // at all), but provide us (as well as the users) the advantages that all
        // type erasure designs enjoy over inheritance: Non-intrusive, better lifetime
        // management, easy-to-reason-about interface, etc.
        //
        // @sa: https://github.com/executors/executors/blob/master/explanatory.md
        class executor {
        public:
            // DefaultConstructible. May be used as placeholder.
            executor() = default;

            // Copy & move.
            executor(const executor &e) : impl_(e.impl_->clone()) {}

            executor(executor &&) = default;

            executor &operator=(const executor &e) {
                impl_ = e.impl_->clone();
                return *this;
            }

            executor &operator=(executor &&) = default;

            // Wraps a real executor in `this`.
            //
            // `T::execute([]{})` must be a valid expression for this overload
            // to participate in resolution.
            template<
                    class T,
                    class = std::enable_if_t<!std::is_same_v<executor, std::decay_t<T>>>,
                    class = decltype(std::declval<T &&>().execute(abel::function<void()>()))>
            /* implicit */ executor(T &&executor) {
                impl_ = erase_executor_type(std::forward<T>(executor));
            }

            // It's allowed (but not required, and generally discouraged) to invoke
            // `job` immediately, before returning to the caller.
            template<class T>
            void execute(T &&job) {
                impl_->execute(std::forward<T>(job));
            }

        private:
            // Type erased `executor`s (concept here).
            class concrete_executor {
            public:
                virtual ~concrete_executor() = default;

                // It should be possible to declare the argument as a customized
                // non-moving wrapper behaves like `FunctionView` but provide the
                // following in addition:
                //
                //  - It requires the functor to be MoveConstructible.
                //  - It's possible to move-construct a `abel::function<void()>` from the
                //    wrapper when needed.
                //
                // By doing this, `concrete_executor` itself does not have to require
                // a move. This can be helpful for things like `inline_executor`.
                //
                // In case it's indeed desired, the derived class could move construct
                // a `abel::function<...>` itself.
                //
                // For now we don't do this, as there's little sense in using an
                // inline_executor too much. For the asynchronous executors, a move is
                // inevitable.
                virtual void execute(abel::function<void()> job) = 0;

                // Clone the executor.
                //
                // Here we requires the `executor` to be CopyConstructible.
                virtual std::unique_ptr<concrete_executor> clone() = 0;
            };

            // This class holds all the information we need to call a concrete
            // `executor` (concept here).
            //
            // `executor` is copy / moved into `*this`. It's should be relatively
            // cheap anyway.
            template<class T>
            class concrete_executor_impl : public concrete_executor {
            public:
                template<class U>
                explicit concrete_executor_impl(U &&e) : impl_(std::forward<U>(e)) {}

                void execute(abel::function<void()> job) override {
                    return impl_.execute(std::move(job));
                }

                std::unique_ptr<concrete_executor> clone() override {
                    return std::make_unique<concrete_executor_impl>(impl_);  // Copied here.
                }

            private:
                T impl_;
            };

            // Erase type of `executor` and copy it to some class inherited from
            // `concrete_executor`.
            template<class T>
            std::unique_ptr<concrete_executor> erase_executor_type(T &&executor) {
                return std::make_unique<concrete_executor_impl<std::decay_t<T>>>(
                        std::forward<T>(executor));
            }

            // Type-erased wrapper for `executor`s.
            std::unique_ptr<concrete_executor> impl_;
        };

        // An "inline" executor just invokes the jobs posted to it immediately.
        //
        // Be careful not to overflow the stack if you're calling `execute` in `job`.
        class inline_executor {
        public:
            void execute(abel::function<void()> job) { job(); }  // Too simple, sometimes naive.
        };

        namespace detail {

            // Default executor to use.
            //
            // Setting a new default executor won't affect `Future`s already
            // constructed, nor will it affects the `Future`s from `Future.Then`.
            //
            // Only `Future`s returned by newly constructed `Promise`'s `get_future`
            // will respect the new settings.
            //
            // Declared as a standalone method rather than a global variable to
            // mitigate issues with initialization order.
            inline std::shared_ptr<executor> &default_executor_ptr() {
                static std::shared_ptr<executor> ptr = std::make_shared<executor>(
                        inline_executor());  // The default one is a bad one, indeed.
                return ptr;
            }

        }  // namespace detail

        // Get the current default executor.
        inline executor get_default_executor() {
            // I haven't check the implementation yet but even if this is lock-free,
            // copying a `std::shared_ptr<...>` is effectively a global ref-counter,
            // which scales bad.
            //
            // This shouldn't hurt, though. We're only called when a new `Promise` is
            // made, which is relatively rare. Calling `Then` on `Future` won't result
            // in a call to us.
            return *std::atomic_load_explicit(&detail::default_executor_ptr(),
                                              std::memory_order_acquire);  // Copied here.
        }

        // Set the default executor to use.
        //
        // The old executor is returned.
        inline executor set_default_executor(executor exec) {
            auto ptr = std::make_shared<executor>(std::move(exec));

            // Do NOT `std::move` here, even if the pointer is going to be destroyed.
            // The pointee (not just the pointer) may well be possibly used by others
            // concurrently.
            //
            // Copying the executor should be thread-safe, anyway.
            return *std::atomic_exchange_explicit(
                    &detail::default_executor_ptr(), std::move(ptr), std::memory_order_acq_rel);
        }

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_EXECUTOR_H_
