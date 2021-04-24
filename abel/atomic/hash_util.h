// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_ATOMIC_HASH_UTIL_H_
#define ABEL_ATOMIC_HASH_UTIL_H_

#include "abel/atomic/hash_config.h"
#include <exception>
#include <thread>
#include <utility>
#include <vector>

namespace abel {

#if ATOMIC_HASH_DEBUG
/// When \ref ATOMIC_HASH_DEBUG is 0, ATOMIC_HASH_DBG will printing out status
/// messages in various situations
#define ATOMIC_HASH_DBG(fmt, ...)                                                \
  fprintf(stderr, "\x1b[32m"                                                   \
                  "[atomic_hash:%s:%d:%lu] " fmt ""                              \
                  "\x1b[0m",                                                   \
          __FILE__, __LINE__,                                                  \
          std::hash<std::thread::id>()(std::this_thread::get_id()),            \
          __VA_ARGS__)
#else
/// When \ref ATOMIC_HASH_DEBUG is 0, ATOMIC_HASH_DBG does nothing
#define ATOMIC_HASH_DBG(fmt, ...)                                                \
  do {                                                                         \
  } while (0)
#endif

///
/// alignas() requires GCC >= 4.9, so we stick with the alignment attribute for
/// GCC.
///
#ifdef __GNUC__
#define ATOMIC_HASH_ALIGNAS(x) __attribute__((aligned(x)))
#else
#define ATOMIC_HASH_ALIGNAS(x) alignas(x)
#endif

///
/// At higher warning levels, MSVC produces an annoying warning that alignment
/// may cause wasted space: "structure was padded due to __declspec(align())".
///
#ifdef _MSC_VER
#define ATOMIC_HASH_SQUELCH_PADDING_WARNING __pragma(warning(suppress : 4324))
#else
#define ATOMIC_HASH_SQUELCH_PADDING_WARNING
#endif

///
/// At higher warning levels, MSVC may issue a deadcode warning which depends on
/// the template arguments given. For certain other template arguments, the code
/// is not really "dead".
///
#ifdef _MSC_VER
#define ATOMIC_HASH_SQUELCH_DEADCODE_WARNING_BEGIN                               \
  do {                                                                         \
    __pragma(warning(push));                                                   \
    __pragma(warning(disable : 4702))                                          \
  } while (0)
#define ATOMIC_HASH_SQUELCH_DEADCODE_WARNING_END __pragma(warning(pop))
#else
#define ATOMIC_HASH_SQUELCH_DEADCODE_WARNING_BEGIN
#define ATOMIC_HASH_SQUELCH_DEADCODE_WARNING_END
#endif

///
/// Thrown when an automatic expansion is triggered, but the load factor of the
/// table is below a minimum threshold, which can be set by the \ref
/// atomic_hash_map::minimum_load_factor method. This can happen if the hash
/// function does not properly distribute keys, or for certain adversarial
/// workloads.
///
class load_factor_too_low : public std::exception {
  public:
    ///
    /// Constructor
    ///
    /// @param lf the load factor of the table when the exception was thrown
    ///
    load_factor_too_low(const double lf) noexcept: load_factor_(lf) {}

    ///
    /// @return a descriptive error message
    ///
    virtual const char *what() const noexcept override {
        return "Automatic expansion triggered when load factor was below "
               "minimum threshold";
    }

    ///
    /// @return the load factor of the table when the exception was thrown
    ///
    double load_factor() const noexcept { return load_factor_; }

  private:
    const double load_factor_;
};

///
/// Thrown when an expansion is triggered, but the hash_power specified is greater
/// than the maximum, which can be set with the \ref
/// atomic_hash_map::maximum_hash_power method.
///
class maximum_hashpower_exceeded : public std::exception {
  public:
    /// Constructor
    ///
    /// @param hp the hash power we were trying to expand to
    ///
    maximum_hashpower_exceeded(const size_t hp) noexcept: hashpower_(hp) {}

    ///
    /// @return a descriptive error message
    ///
    virtual const char *what() const noexcept override {
        return "Expansion beyond maximum hash_power";
    }

    ///
    /// @return the hash_power we were trying to expand to
    ///
    size_t hash_power() const noexcept { return hashpower_; }

  private:
    const size_t hashpower_;
};

}  // namespace abel

#endif  // ABEL_ATOMIC_HASH_UTIL_H_
