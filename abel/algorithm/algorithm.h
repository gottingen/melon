// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_ALGORITHM_ALGORITHM_H_
#define ABEL_ALGORITHM_ALGORITHM_H_

#include <algorithm>
#include <iterator>
#include "abel/meta/type_traits.h"
#include "abel/base/profile.h"

namespace abel {


namespace algorithm_internal {

// Performs comparisons with operator==, similar to C++14's `std::equal_to<>`.
struct equal_to {
    template<typename T, typename U>
    bool operator()(const T &a, const U &b) const {
        return a == b;
    }
};

template<typename InputIter1, typename InputIter2, typename Pred>
bool equal_impl(InputIter1 first1, InputIter1 last1, InputIter2 first2,
               InputIter2 last2, Pred pred, std::input_iterator_tag,
               std::input_iterator_tag) {
    while (true) {
        if (first1 == last1) return first2 == last2;
        if (first2 == last2) return false;
        if (!pred(*first1, *first2)) return false;
        ++first1;
        ++first2;
    }
}

template<typename InputIter1, typename InputIter2, typename Pred>
bool equal_impl(InputIter1 first1, InputIter1 last1, InputIter2 first2,
               InputIter2 last2, Pred &&pred, std::random_access_iterator_tag,
               std::random_access_iterator_tag) {
    return (last1 - first1 == last2 - first2) &&
           std::equal(first1, last1, first2, std::forward<Pred>(pred));
}

// When we are using our own internal predicate that just applies operator==, we
// forward to the non-predicate form of std::equal. This enables an optimization
// in libstdc++ that can result in std::memcmp being used for integer types.
template<typename InputIter1, typename InputIter2>
bool equal_impl(InputIter1 first1, InputIter1 last1, InputIter2 first2,
               InputIter2 last2, algorithm_internal::equal_to /* unused */,
               std::random_access_iterator_tag,
               std::random_access_iterator_tag) {
    return (last1 - first1 == last2 - first2) &&
           std::equal(first1, last1, first2);
}

template<typename It>
It rotate_impl(It first, It middle, It last, std::true_type) {
    return std::rotate(first, middle, last);
}

template<typename It>
It rotate_impl(It first, It middle, It last, std::false_type) {
    std::rotate(first, middle, last);
    return std::next(first, std::distance(middle, last));
}

}  // namespace algorithm_internal

/**
 * @brief This is a C++11-compatible implementation of C++14 `std::equal`.See
 * https://en.cppreference.com/w/cpp/algorithm/equal for more information.
 * Compares the equality of two ranges specified by pairs of iterators, using
 * the given predicate, returning
 *
 * This comparison takes at most min(`last1` - `first1`, `last2` - `first2`)
 * invocations of the predicate. Additionally, if InputIter1 and InputIter2 are
 * both random-access iterators, and `last1` - `first1` != `last2` - `first2`,
 * then the predicate is never invoked and the function returns false.
 * @tparam InputIter1 paired iterator template
 * @tparam InputIter2 paired iterator template
 * @tparam Pred factor to compare two inputs
 * @param first1 star
 * @param last1 todo
 * @param first2 todo
 * @param last2 todo
 * @param pred todo
 * @return true iff for each corresponding iterator i1
 * and i2 in the first and second range respectively, pred(*i1, *i2) == true
 */
template<typename InputIter1, typename InputIter2, typename Pred>
bool equal(InputIter1 first1, InputIter1 last1, InputIter2 first2,
           InputIter2 last2, Pred &&pred) {
    return algorithm_internal::equal_impl(
            first1, last1, first2, last2, std::forward<Pred>(pred),
            typename std::iterator_traits<InputIter1>::iterator_category{},
            typename std::iterator_traits<InputIter2>::iterator_category{});
}

/**
 * @brief overload equal, using "==" operator for pred
 * @tparam InputIter1 todo
 * @tparam InputIter2 todo
 * @param first1 todo
 * @param last1 todo
 * @param first2 todo
 * @param last2 todo
 * @return todo
 */
template<typename InputIter1, typename InputIter2>
bool equal(InputIter1 first1, InputIter1 last1, InputIter2 first2,
           InputIter2 last2) {
    return abel::equal(first1, last1, first2, last2,
                       algorithm_internal::equal_to{});
}


/**
 * @brief Performs a linear search for `value` using the iterator `first` up to
 * but not including `last`, returning true if [`first`, `last`) contains an
 * element equal to `value`. complexity o(n), it may fast than binary search over
 * short containers
 * @tparam InputIterator todo
 * @tparam EqualityComparable todo
 * @param first todo
 * @param last todo
 * @param value todo
 * @return todo
 */
template<typename InputIterator, typename EqualityComparable>
bool linear_search(InputIterator first, InputIterator last,
                   const EqualityComparable &value) {
    return std::find(first, last, value) != last;
}

/**
 * @brief todo
 * @tparam ForwardIterator todo
 * @param first todo
 * @param middle todo
 * @param last todo
 * @return todo
 */
template<typename ForwardIterator>
ForwardIterator rotate(ForwardIterator first, ForwardIterator middle,
                       ForwardIterator last) {
    return algorithm_internal::rotate_impl(
            first, middle, last,
            std::is_same<decltype(std::rotate(first, middle, last)),
                    ForwardIterator>());
}


}  // namespace abel

#endif  // ABEL_ALGORITHM_ALGORITHM_H_
