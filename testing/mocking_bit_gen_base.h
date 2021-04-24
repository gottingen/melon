// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
#ifndef TEST_TESTING_MOCKING_BIT_GEN_BASE_H_
#define TEST_TESTING_MOCKING_BIT_GEN_BASE_H_

#include <atomic>
#include <deque>
#include <string>
#include <typeinfo>

#include "abel/random/random.h"
#include "abel/strings/str_cat.h"

namespace abel {

namespace random_internal {

// MockingBitGenExpectationFormatter is invoked to format unsatisfied mocks
// and remaining results into a description string.
template<typename DistrT, typename FormatT>
struct MockingBitGenExpectationFormatter {
    std::string operator()(std::string_view args) {
        return abel::string_cat(FormatT::FunctionName(), "(", args, ")");
    }
};

// MockingBitGenCallFormatter is invoked to format each distribution call
// into a description string for the mock log.
template<typename DistrT, typename FormatT>
struct MockingBitGenCallFormatter {
    std::string operator()(const DistrT &dist,
                           const typename DistrT::result_type &result) {
        return abel::string_cat(
                FormatT::FunctionName(), "(", FormatT::FormatArgs(dist), ") => {",
                FormatT::FormatResults(abel::make_span(&result, 1)), "}");
    }
};

class MockingBitGenBase {
    template<typename>
    friend
    struct distribution_caller;
    using generator_type = abel::bit_gen;

  public:
    // URBG interface
    using result_type = generator_type::result_type;

    static constexpr result_type (min)() { return (generator_type::min)(); }

    static constexpr result_type (max)() { return (generator_type::max)(); }

    result_type operator()() { return gen_(); }

    MockingBitGenBase() : gen_(), observed_call_log_() {}

    virtual ~MockingBitGenBase() = default;

  protected:
    const std::deque<std::string> &observed_call_log() {
        return observed_call_log_;
    }

    // CallImpl is the type-erased virtual dispatch.
    // The type of dist is always distribution<T>,
    // The type of result is always distribution<T>::result_type.
    virtual bool CallImpl(const std::type_info &distr_type, void *dist_args,
                          void *result) = 0;

    template<typename DistrT, typename ArgTupleT>
    static const std::type_info &GetTypeId() {
        return typeid(std::pair<abel::decay_t<DistrT>, abel::decay_t<ArgTupleT>>);
    }

    // Call the generating distribution function.
    // Invoked by distribution_caller<>::Call<DistT, FormatT>.
    // DistT is the distribution type.
    // FormatT is the distribution formatter traits type.
    template<typename DistrT, typename FormatT, typename... Args>
    typename DistrT::result_type call(Args &&... args) {
        using distr_result_type = typename DistrT::result_type;
        using ArgTupleT = std::tuple<abel::decay_t<Args>...>;

        ArgTupleT arg_tuple(std::forward<Args>(args)...);
        auto dist = abel::make_from_tuple<DistrT>(arg_tuple);

        distr_result_type result{};
        bool found_match =
                CallImpl(GetTypeId<DistrT, ArgTupleT>(), &arg_tuple, &result);

        if (!found_match) {
            result = dist(gen_);
        }

        // TODO(asoffer): Forwarding the args through means we no longer need to
        // extract them from the from the distribution in formatter traits. We can
        // just string_join them.
        observed_call_log_.push_back(
                MockingBitGenCallFormatter<DistrT, FormatT>{}(dist, result));
        return result;
    }

  private:
    generator_type gen_;
    std::deque<std::string> observed_call_log_;
};  // namespace random_internal

}  // namespace random_internal

}  // namespace abel

#endif  // TEST_TESTING_MOCKING_BIT_GEN_BASE_H_
