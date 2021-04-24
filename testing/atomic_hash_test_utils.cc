// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "testing/atomic_hash_test_utils.h"

std::atomic<int64_t> &get_unfreed_bytes() {
    static std::atomic<int64_t> unfreed_bytes(0L);
    return unfreed_bytes;
}
