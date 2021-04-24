// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "testing/mocking_bit_gen.h"

#include <string>

namespace abel {

MockingBitGen::~MockingBitGen() {

    for (const auto &del : deleters_) {
        del();
    }
}

}  // namespace abel
