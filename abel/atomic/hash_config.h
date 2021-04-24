// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_ATOMIC_HASH_CONFIG_H_
#define ABEL_ATOMIC_HASH_CONFIG_H_

#include <cstddef>
#include <limits>

namespace abel {

/// The default maximum number of keys per bucket
constexpr size_t DEFAULT_SLOT_PER_BUCKET = 4;

/// The default number of elements in an empty hash table
constexpr size_t DEFAULT_SIZE =
        (1U << 16) * DEFAULT_SLOT_PER_BUCKET;

/// The default minimum load factor that the table allows for automatic
/// expansion. It must be a number between 0.0 and 1.0. The table will throw
/// load_factor_too_low if the load factor falls below this value
/// during an automatic expansion.
constexpr double DEFAULT_MINIMUM_LOAD_FACTOR = 0.05;

/// An alias for the value that sets no limit on the maximum hash_power. If this
/// value is set as the maximum hash_power limit, there will be no limit. This
/// is also the default initial value for the maximum hash_power in a table.
constexpr size_t NO_MAXIMUM_HASHPOWER =
        std::numeric_limits<size_t>::max();

/// set ATOMIC_HASH_DEBUG to 1 to enable debug output
#define ATOMIC_HASH_DEBUG 0

}  // namespace abel

#endif  // ABEL_ATOMIC_HASH_CONFIG_H_
