//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_FUTURE_UTILS_H_
#define ABEL_FUTURE_INTERNAL_FUTURE_UTILS_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "abel/future/internal/basics.h"
#include "abel/future/internal/impls.h"

namespace abel {
    namespace future_internal {
        // Creates a "ready" future from values.
        template<class... Ts>
        future<std::decay_t<Ts>...> make_ready_future(Ts &&... values);

        // Creates a (possibly "ready") `future<>` by calling `future` with `args...`.
        template<class F, class... Args,
                class R = futurize_t<std::invoke_result_t<F, Args...>>>
        R make_future_with(F &&functor, Args &&... args);

        // As the `future<...>` itself does not support blocking, we provide this
        // helper for blocking on `future<...>`.
        //
        // The `future` must NOT have continuation chained, otherwise the behavior
        // is undefined.
        //
        // The `future` passed in is move away so it's no longer usable after
        // calling this method.
        //
        // Calling this method may lead to deadlock if the `future` we're blocking
        // on is scheduled to be running on the same thread later.
        //
        // future: `future` to block on. Invalidated on return.
        // Returns: Value with which the `future` is satisfied with.
        //
        //          - If the `Ts...` is empty, the return type is `void`;
        //          - If there's only one type in `Ts...`, that type is the return
        //            type.
        //          - Otherwise `std::tuple<Ts...>` is returned.
        //
        //          The value in `future` is *moved out* into the return value.
        template<class... Ts>
        auto blocking_get(future<Ts...> &&future) -> unboxed_type_t<Ts...>;

        // Same as `blocking_get` except that it allows specifying timeout, and returns
        // a `std::optional` instead (in case `Ts...` is empty, a `bool` is returned
        // instead.).
        template<class... Ts, class DurationOrTimePoint>
        auto blocking_try_get(future<Ts...> &&future, const DurationOrTimePoint &timeout)
        -> abel::future_internal::detail::optional_or_bool_t<unboxed_type_t<Ts...>>;

        // Same as `blocking_get`, except that this one returns a `boxed<...>`.
        template<class... Ts>
        auto blocking_get_preserving_errors(future<Ts...> &&future) -> boxed<Ts...>;

        template<class... Ts, class DurationOrTimePoint>
        auto blocking_try_get_preserving_errors(future<Ts...> &&future, const DurationOrTimePoint &timeout)
        -> std::optional<boxed<Ts...>>;

        // Returns a `future` that is satisfied once all of the `future`s passed
        // in are satisfied.
        //
        // Note that even if `std::vector<bool>` is special in that it's not guaranteed
        // to be thread safe to access different elements concurrently, calling
        // `when_all` on `std::vector<future<bool>>` (resulting in `std::vector<bool>`)
        // won't lead to data race.
        //
        // futures: `future`s to wait. All of them are invalidated on return.
        // Returns: `future` satisfied with all of the values that are used to satisfy
        //          `futures`, with order preserved. The return value is constructed
        //          as if `std::tuple(blocking_get(futures)...)` (with all `void`s
        //          removed) is used to construct a ready `future`.
        //
        //          It confused me for some time if we should group the values in
        //          return type or flatten them, i.e., given `when_all(future<>(),
        //          future<int>(), future<char, double>()>`, should the return type
        //          be:
        //
        //          - future<int, std::tuple<char, double>> (what we currently do), or
        //          - future<int, char, double>?
        //
        //          (I personally object to use `future<std::tuple<>, std::tuple<int>,
        //          std::tuple<char, double>>`, it's unnecessarily complicated.)
        template<class... Ts,  // `Ts` can't be ref types.
                class = std::enable_if_t<(sizeof...(Ts) > 0) && is_futures_v<Ts...> &&
                                         are_rvalue_refs_v<Ts &&...>>,

                class R

                = rebind_t<
                        types_erase_t<empty_type<decltype(blocking_get(std::declval<Ts>()))...>,
                                void>,
                        future>>

        auto when_all(Ts &&... futures) -> R;

        // Returns a `future` that is satisfied once all of the `future`s passed
        // in are satisfied.
        //
        // futures: Same as `when_all`.
        // Returns: `future` satisfied with all of the boxed values that are used to
        //          satisfy `futures`, with order preserved. For each `future<Ts...>`
        //          in `futures`, `boxed<Ts...>` will be placed in the corresponding
        //          position in the return type.
        //
        //          e.g. `when_all(future<>(), future<int>(), future<double, char>())`
        //          will be type `future<boxed<>>, boxed<int>, boxed<double, char>>`.
        template<class... Ts,
                class = std::enable_if_t<(sizeof...(Ts) > 0) && is_futures_v<Ts...> &&
                                         are_rvalue_refs_v<Ts &&...>>>

        auto when_all_preserving_errors(Ts &&... futures) -> future<as_boxed_t<Ts>...>;

        // Overload for a collection of homogeneous objects in a container.
        //
        // `future`s in `futures` are invalidated on return.
        //
        // Returns:
        //   - `void` if `Ts...` is empty. (i.e., `C<future<>>` is passed in.)
        //   - A collection of values with the same type as is `blocking_get(future<
        //     Ts...>())`, with order preserved.
        template<
                template<class...> class C, class... Ts,
                class R = std::conditional_t<std::is_void_v<unboxed_type_t<Ts...>>,
                        future<>, future<C<unboxed_type_t<Ts...>>>>>

        auto when_all(C<future<Ts...>> &&futures) -> R;

        // Overload for a collection of homogeneous objects in a container.
        //
        // Returns: A collection of `boxed<Ts...>`s, with order preserved.
        template<template<class...> class C, class... Ts>
        auto when_all_preserving_errors(C<future<Ts...>> &&futures) -> future<C < boxed<Ts...>>

        >;

        // Returns a `future` that is satisfied when any of the `future`s passed in
        // is satisfied.
        //
        // futures: `future`s to wait. All of them are invalidated on return.
        //
        //          It's UNDEFINED to call `when_any` on an empty collection.
        //
        //          This is an implementation limitation: If we do allow an empty
        //          collection be passed in, we'd have to default construct `Ts...`
        //          on return. I'm not sure if handling this corner case justify the
        //          burden we put on the user by requiring `Ts...` to be
        //          DefaultConstructible.
        //
        //          But even if it's implementable without requiring `Ts...` to be
        //          DefaultConstructible, it's still hard to tell what does it mean
        //          to "wait for an object from nothing".
        //
        //          This is an inconsistency between `when_all` and `when_any`.
        //
        // Returns: `future` satisfied with both the index of the first satisfied
        //          `future` and its value (the type is the same type as returned
        //          by `blocking_get`). If there's no value in `futures` (i.e.,
        //          `Ts...` is empty), only the index will be saved in the `future`
        //          returned.
        template<template<class...> class C, class... Ts,
                class R = std::conditional_t<
                        std::is_void_v<unboxed_type_t<Ts...>>, future<std::size_t>,
                        future<std::size_t, unboxed_type_t<Ts...>>>>

        auto when_any(C<future<Ts...>> &&futures) -> R;

        // Same as above except for return value type, which holds a boxed<Ts...>
        // instead of values.
        //
        // `futures` may NOT be empty. (See comments above `when_any`'s declaration.)
        template<template<class...> class C, class... Ts>
        auto when_any_preserving_errors(C<future<Ts...>> &&futures) -> future<std::size_t, boxed<Ts...>>;

        // DEPRECATED: Use `split(...)` instead.
        //
        // Counterintuitively, `fork`ing a `future` not only gives back a
        // `future` that is satisfied with the same value as the original
        // one, but also mutates the `future` passed in, due to implementation
        // limitations. Generally, the users of `fork` need not to be aware of
        // this.
        //
        // `fork` requires `Ts...` to be CopyConstructible, naturally.
        //
        // future: pointer to `future` to fork. Overwritten with an equivalent one
        //         on return.
        // Returns: Another `future` that is satisfied with the same value
        //          as of `f`.
        // Usage:
        //        promise<int> p;
        //        auto f = p.get_future();
        //        auto f2 = fork(&f);
        //        p.set_value(1);
        //        // Now both `f` and `f2` are satisfied with `1`.
        template<class... Ts>
        future<Ts...> fork(future<Ts...> *future);

        // "Splitting" a `future` into two. This can be handy if the result of a future
        // is used in two code branches.
        //
        // `Ts...` need to be CopyConstructible, as obvious.
        //
        // Returns: Two `future`s, which are satisfied once `future` is satisfied.
        //          It's unspecified whether `Ts`... is moved or copied into the
        //          resulting future.
        //
        // Usage:
        //
        //   promise<int> p;
        //   auto f = p.get_future();
        //   auto&& [f1, f2] = split(&f);
        //   p.set_value(1);
        //   // Both `f1` and `f2` are satisfied with `1` now.
        template<class... Ts>
        std::pair<future<Ts...>, future<Ts...>>
        split(future<Ts...> &&future);

        template<class... Ts>
        std::pair<future<Ts...>, future<Ts...>>
        split(future<Ts...> *future);

        // Keep calling `action` passed in until it returns false.
        //
        // action: Action to call. The action should accept no arguments and return
        //         a type that is implicitly convertible to `bool` / `future<bool>` (
        //         i.e. the signature of `action` should be `bool ()` /
        //         `future<bool> ()`.). The loop stops when the value returned by
        //         `action` is `false`.
        template<class F,
                class = std::enable_if_t<std::is_invocable_r_v<bool, F> ||
                                         std::is_invocable_r_v<future<bool>, F>>>

        future<> repeat(F &&action);

        // Keep calling `action` until `pred` returns false.
        //
        // action: Action to call. The action should accept no arguments, and its
        //         return value is passed to `pred` to evaluate if the loop should
        //         proceed.
        //
        // pred: Predicates if the loop should proceed. It accepts what's returned
        //       by `action`, which could be zero or more values, and returns a `bool`
        //       (or `future<bool>`, as `repeat` expects) to indicate if the loop
        //       should continue.
        //
        //       In case `action` returns a `future<...>`, number of arguments to
        //       `pred` could by more than 1 (`action` returns `future<int, double>`,
        //       e.g.).
        //
        //       `pred` should not accept argument by value unless it's expecting a
        //       cheap-to-copy type (`int`, for example).
        //
        // Returns: The value return by `action` in the last iteration is returned.
        //
        // CAUTION: Due to implementation limitation, looping without useing a non--
        //          inline executor may lead to stack overflow.
        template<
                class F, class Pred,
                class R = futurize_t<std::invoke_result_t<F>>,  // Implies `F` invocable.
                class = std::enable_if_t<std::is_convertible_v<
                        decltype(std::declval<R>().then(std::declval<Pred>())),  // `Pred`
                        // accepts `R`.
                        future<bool>>>  // And the result of `Pred` is convertible to `bool`.
        >

        R repeat_if(F &&action, Pred &&pred);


        template<class... Ts>
        future<std::decay_t<Ts>...> make_ready_future(Ts &&... values) {
            return future(futurize_values, std::forward<Ts>(values)...);
        }

        template<class F, class... Args, class R>
        R make_future_with(F &&functor, Args &&... args) {
            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
                static_assert(std::is_same_v<R, future<>>);

                functor(std::forward<Args>(args)...);
                return R(futurize_values);
            } else {
                return R(functor(std::forward<Args>(args)...));
            }
        }

        template<class... Ts>
        auto blocking_get(future<Ts...> &&future) -> unboxed_type_t<Ts...> {
            return blocking_get_preserving_errors(std::move(future)).get();
        }

        template<class... Ts>
        auto blocking_get_preserving_errors(future<Ts...> &&future) -> boxed<Ts...> {
            std::condition_variable cv;
            std::mutex lock;
            std::optional<boxed<Ts...>> receiver;

            // Once the `future` is satisfied, our continuation will move the
            // result into `receiver` and notify `cv` to wake us up.
            std::move(future).then([&](boxed<Ts...> boxed) noexcept {
                std::lock_guard<std::mutex> lk(lock);
                receiver.emplace(std::move(boxed));
                cv.notify_one();
            });

            // Block until our continuation wakes us up.
            std::unique_lock<std::mutex> lk(lock);
            cv.wait(lk, [&] { return !!receiver; });

            return std::move(*receiver);
        }

        template<class... Ts, class DurationOrTimePoint>
        auto blocking_try_get(future<Ts...> &&future, const DurationOrTimePoint &timeout)
        -> abel::future_internal::detail::optional_or_bool_t<unboxed_type_t<Ts...>> {
            auto rc = blocking_try_get_preserving_errors(std::move(future), timeout);
            if constexpr (sizeof...(Ts) == 0) {
                return !!rc;
            } else {
                if (rc) {
                    return std::move(rc->get());
                }
                return std::nullopt;
            }
        }

        template<class... Ts, class DurationOrTimePoint>
        auto blocking_try_get_preserving_errors(future<Ts...> &&future,
                                            const DurationOrTimePoint &timeout) -> std::optional<boxed<Ts...>> {
            struct State {
                std::condition_variable cv;
                std::mutex lock;
                std::optional<boxed<Ts...>> receiver;
            };
            auto state = std::make_shared<State>();

            // `state` must be copied here, in case of timeout, we'll leave the scope
            // before continuation is fired.
            std::move(future).then([state](boxed<Ts...> boxed) noexcept {
                std::lock_guard<std::mutex> lk(state->lock);
                state->receiver.emplace(std::move(boxed));
                state->cv.notify_one();
            });

            // Block until our continuation wakes us up.
            std::unique_lock<std::mutex> lk(state->lock);
            if constexpr (abel::future_internal::detail::is_duration_v<DurationOrTimePoint>) {
                state->cv.wait_for(lk, timeout, [&] {
                    return !!state->receiver;
                });
            } else {
                state->cv.wait_until(lk, timeout, [&] {
                    return !!state->receiver;
                });
            }

            return std::move(state->receiver);  // Safe. we're holding the lock.
        }

        namespace detail {

            struct Flatten {
                // This helper adds another `std::tuple` on top of `t` if there's at
                // least 2 elements in `t`.
                //
                // We'll use this below when we `std::tuple_cat`ing the tuples we get
                // from `boxes.get_raw()`.
                //
                // Because `std::tuple_cat` will remove the outer `std::tuple` from
                // `boxed<...>::get_raw()`, simply concatenating the tuples will flatten
                // the values even for the tuples with more than 1 elements.
                template<class T>
                auto rebox(T &&t) const {
                    using Reboxed = std::conditional_t<(std::tuple_size_v<T> >=
                                                        2), std::tuple<T &&>, T>;  // Forward by refs if we do rebox the value.
                    return Reboxed(std::forward<T>(t));
                }

                // Here we:
                //
                // - Iterate through each `boxed<...>`;
                // - get the `std::tuple<...>` inside and `rebox` it;
                // - Concatenate the tuples we got..
                //
                // No copy should occur, everything is moved.
                template<class T, std::size_t... Is>
                auto operator()(T &&t, std::index_sequence<Is...>) const {
                    return std::tuple_cat(rebox(std::move(std::get<Is>(t).get_raw()))...);
                }
            };

        }  // namespace detail

        template<class... Ts, class, class R>
        auto when_all(Ts &&... futures) -> R {
            return when_all_preserving_errors(std::move(futures)...).then([](boxed<as_boxed_t<Ts>...> boxes) -> R {
                // Here we get `std::tuple<boxed<...>, boxed<...>, boxed<...>>`.
                auto &&raw = boxes.get_raw();

                auto &&flattened =
                        detail::Flatten()(raw, std::index_sequence_for<Ts...>());
                // `raw` was moved and invalidated.

                return future(futurize_tuple, std::move(flattened));
            });
        }

        template<template<class...> class C, class... Ts, class R>
        auto when_all(C<future<Ts...>> &&futures) -> R {
            return when_all_preserving_errors(std::move(futures)).then([](C<boxed<Ts...>> boxed_values) {
                if constexpr (std::is_void_v<unboxed_type_t<Ts...>>) {
                    // sizeof...(Ts) == 0.
                    //
                    // Nothing to return.
                    (void) boxed_values;
                    return;
                } else {
                    C<unboxed_type_t<Ts...>> result;

                    result.reserve(boxed_values.size());
                    for (auto &&e: boxed_values) {
                        result.emplace_back(std::move(e).get());
                    }
                    return result;
                }
            });
        }

        template<class... Ts, class>
        auto when_all_preserving_errors(Ts &&... futures) -> future<as_boxed_t<Ts>...> {
            static_assert(sizeof...(Ts) != 0,
                          "There's no point in waiting on an empty future pack..");

            struct Context {
                promise<as_boxed_t<Ts>...> m_promise;
                std::tuple<as_boxed_t<Ts>...> m_receivers{
                        abel::future_internal::detail::retrieve_boxed<as_boxed_t<Ts >>
                                ()...
                };
                std::atomic<std::size_t> m_left{sizeof...(Ts)};
            };
            auto context = std::make_shared<Context>();

            for_each_indexed([&](auto &&future, auto index) {
                // We chain a continuation for each future. The continuation will
                // satisfy the promise we made once all of the futures are satisfied.
                std::move(future).then(
                        [context](
                                as_boxed_t<std::remove_reference_t<decltype(future)>> boxed) {
                            std::get<decltype(index)::value>(context->m_receivers) =
                                    std::move(boxed);
                            if (!--context->m_left) {
                                context->m_promise.set_value(std::move(context->m_receivers)
                                );
                            }
                        });
            }, futures...);

            return context->m_promise.get_future();

        }

    template<template<class...> class C, class... Ts>
    auto when_all_preserving_errors(C<future<Ts...>> &&futures) -> future<C < boxed<Ts...>>> {
        if (futures.empty()) {
            return future(futurize_values, C<boxed<Ts...>>());
        }

        struct Context {
            promise <C<boxed < Ts...>>> m_promise;
            C <boxed<Ts...>> m_values;
            std::atomic<std::size_t> m_left;
        };
        auto context = std::make_shared<Context>();

        // We cannot inline the initialization in `Context` as `futures.size()`
        // is not constant.
        context->m_values.reserve(futures.size());
        context->m_left = futures.size();
        for (std::size_t index = 0;index != futures.size();++index) {
            context->m_values.emplace_back (abel::future_internal::detail::retrieve_boxed<boxed < Ts...>>());

        }

        for (std::size_t index = 0;index != futures.size();++index) {
            std::move(futures[index]).then([index, context](boxed<Ts...> boxed) mutable {
                context->m_values[index] = std::move(boxed);

            if (!--context->m_left) {
                context->m_promise.set_value(std::move(context->m_values));
            }
            });
        }

        return context->m_promise.get_future();
    }

    template<template<class...> class C, class... Ts, class R>
    auto when_any(C<future < Ts...>> &&futures) -> R {
        return when_any_preserving_errors(std::move(futures)).then([](std::size_t index, boxed<Ts...>values) {
            if constexpr (sizeof...(Ts) == 0) {
                (void)values;
                return index;
            } else {
                return future(futurize_values, index, values.get());
            }
        });
    }

    template<template<class...> class C, class... Ts>
    auto when_any_preserving_errors(C<future < Ts...>>&&futures) -> future <std::size_t, boxed<Ts...>> {
        // I do want to return a ready future on empty `futures`, but this
        // additionally requires `Ts...` to be DefaultConstructible, which
        // IMO is an overkill.
        DCHECK(!futures.empty(),

               "Calling `when_any(PreservingErrors)` on an empty "
               "collection is undefined. We simply couldn't "
               "define what does 'wait for a single object in an "
               "empty collection' mean.");
        struct Context {
            promise <std::size_t, boxed<Ts...>> m_promise;
            std::atomic<bool> m_ever_satisfied{};
        };
        auto context = std::make_shared<Context>();

        for (std::size_t index = 0; index != futures.size(); ++index) {
            std::move(futures[index]).then([index, context](boxed<Ts...> boxed) {
                if (!context->m_ever_satisfied.exchange(true)) {
                    // We're the first `future` satisfied.
                    context->m_promise.set_value(index, std::move(boxed));
                }
            });
        }

        return context->m_promise.get_future();

    }

    template<class... Ts>
    future<Ts...> fork(future<Ts...> *future) {
        promise < Ts...> p;  // FIXME: The default executor (instead of `future`'s)
        //        is used here.
        auto f = p.get_future();

        *future =
                std::move(*future).then([p = std::move(p)](boxed<Ts...> boxed) mutable {
                    p.set_boxed(boxed);  // Requires `boxed<Ts...>` to be CopyConstructible.
                    if constexpr (sizeof...(Ts) != 0) {
                        return std::move(boxed.get());
                    } else {
                        // Nothing to do for `void`.
                    }
                });

        return f;
    }

    template<class... Ts>
    std::pair<future < Ts...>, future<Ts...>> split(future<Ts...>&& future) {
        return split(&future);
    }

    template<class... Ts>
    std::pair<future < Ts...>, future<Ts...>> split(future<Ts...> * future) {
        auto rf = fork(future);
        return std::pair(std::move(rf), std::move(*future));
    }

    template<class F, class>
    future <> repeat(F &&action) {
        return repeat_if(std::forward<F>(action), [](bool f) { return f; }).then([](auto &&) {});
    }

namespace detail {

    template<class R, class APtr, class PPtr>
    R repeat_if_impl(APtr act, PPtr pred) {
        // Note that here we should not introduce races as we'll NOT call `action`
        // concurrently, i.e., we're always in a "single threaded" environment --
        // The second iteration won't start until the first one is finished.

        // Optimization potentials: `make_future_with` is not strictly needed. We may
        // eliminate the extra `then`s when `act` / `pred` returns a immediately
        // value (instead of `future<...>`).
        //
        //   But keep an eye not to starve other jobs waiting for execution in our
        //   thread (once we use an executor).
        auto value = make_future_with(*act);

        return std::move(value).then([act = std::move(act),
                                             pred = std::move(pred)](auto &&... v) mutable {
            auto keep_going = make_future_with(*pred, v...);  // `v...` is not moved away.

            return std::move(keep_going).then([act = std::move(act), pred = std::move(pred),
                                  vp = std::make_tuple(std::move(v)...)](bool k) mutable {
                        if (k) {
                            return repeat_if_impl<R>(std::move(act),
                                                   std::move(pred));  // Recursion.
                        } else {
                            return R(futurize_tuple, std::move(vp));
                        }
                    });
        });
    }

}  // namespace detail

    template<class F, class Pred, class R, class>
    R repeat_if(F &&action, Pred &&pred) {
        // We'll need `action` and `pred` every iteration.
        //
        // Instead of moving it all over around (as Folly do), we move it into
        // heap once and keep a reference to it until the iteration is done.
        //
        // This way we should be able to save a lot of move (which is cheap but
        // nonetheless not free.).
        //
        // Indeed we still have to move the references (`std::shared_ptr`) around
        // (inside `repeat_if_impl`) but that's much cheaper generally.
        //
        // Whether this is beneficial depends on how many times we loop, indeed.
        return detail::repeat_if_impl<R>(
                std::make_shared<std::decay_t<F>>(std::forward<F>(action)),
                std::make_shared<std::decay_t<Pred>>(std::forward<Pred>(pred)));
    }

    template<class... Ts>
    auto blocking_get(future < Ts...> * future) {
        return blocking_get(std::move(*future));
    }

    template<class... Ts>
    auto blocking_get_preserving_errors(future < Ts...> * future) {
    return blocking_get_preserving_errors(std::move(*future));
    }

    template<class... Ts, class DurationOrTimePoint>
    auto blocking_try_get(future < Ts...> * future, const DurationOrTimePoint &tp) {
        return blocking_try_get(std::move(*future), tp);
    }

    template<class... Ts, class DurationOrTimePoint>
    auto blocking_try_get_preserving_errors(future < Ts...> * future, const DurationOrTimePoint &tp) {
        return blocking_get_preserving_errors(std::move(*future), tp);
    }

    template<class... Ts, class = std::enable_if_t<is_futures_v < Ts...>>>

    auto when_all(Ts *... futures) {
        return when_all(std::move(*futures)...);
    }

    template<class... Ts, class = std::enable_if_t<is_futures_v < Ts...>>>

    auto when_all_preserving_errors(Ts *... futures) {
        return when_all_preserving_errors(std::move(*futures)...);
    }

    template<template<class...> class C, class... Ts>
    auto when_all(C<future < Ts...>> *futures) {
        return when_all(std::move(*futures));
    }

    template<template<class...> class C, class... Ts>
    auto when_all_preserving_errors(C<future < Ts...>> *futures) {
        return when_all_preserving_errors(std::move(*futures));
    }

    template<template<class...> class C, class... Ts>
    auto when_any(C<future < Ts...>> *futures) {
        return when_any(std::move(*futures));
    }

    template<template<class...> class C, class... Ts>
    auto when_any_preserving_errors(C<future < Ts...>>*futures) {
        return when_any_preserving_errors(std::move(*futures));
    }

}  // namespace future_internal

}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_FUTURE_UTILS_H_
