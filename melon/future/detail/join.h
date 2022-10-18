
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_JOIN_H_
#define MELON_FUTURE_DETAIL_JOIN_H_


#include "melon/future/detail/utility.h"

namespace melon::future_internal {

    template<typename Alloc, typename... FutTs>
    struct landing {
        std::tuple<expected < decay_future_t<FutTs>, std::exception_ptr>...>
        landing_;
        std::atomic<int> full_filled_ = 0;

        using storage_type = future_storage<Alloc, decay_future_t<FutTs>...>;
        storage_ptr <storage_type> dst_;

        void ping() {
            if (++full_filled_ == sizeof...(FutTs)) {
                dst_->finish(std::move(landing_));
            }
        }
    };

// Used by join to listen to the individual futures.
    template<std::size_t id, typename LandingT, typename... FutTs>
    void bind_landing(const std::shared_ptr<LandingT> &, FutTs &&...) {
        static_assert(sizeof...(FutTs) == 0);
    }

    template<std::size_t id, typename LandingT, typename Front, typename... FutTs>
    void bind_landing(const std::shared_ptr<LandingT> &l, Front &&front,
                      FutTs &&... futs) {
        auto value_landing = &std::get<id>(l->landing_);
        front.finally([=](expected<typename std::decay_t<Front>::value_type, std::exception_ptr> &&e) {
            *value_landing = std::move(e);
            l->ping();
        });

        bind_landing<id + 1>(l, std::forward<FutTs>(futs)...);
    }

}  // namespace melon::future_internal

namespace melon {

    template<typename FirstT, typename... FutTs>
    auto join(FirstT &&first, FutTs &&... futs) {
        static_assert(sizeof...(FutTs) >= 1, "Trying to join less than two futures?");
        static_assert(is_future_v < std::decay_t<FirstT>>);
        static_assert(std::conjunction_v<is_future < std::decay_t<FutTs>>...>,
        "trying to join a non-future");

        using landing_type =
        future_internal::landing<typename std::decay_t<FirstT>::allocator_type,
                std::decay_t<FirstT>, std::decay_t<FutTs>...>;
        using fut_type = typename landing_type::storage_type::future_type;

        auto landing = std::make_shared<landing_type>();
        landing->dst_.allocate(first.allocator());

        future_internal::bind_landing<0>(landing, std::forward<FirstT>(first),
                                         std::forward<FutTs>(futs)...);

        return fut_type{landing->dst_};
    }
}  // namespace melon

#endif  // MELON_FUTURE_DETAIL_JOIN_H_
