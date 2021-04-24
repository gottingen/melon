// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/container/ring_buffer.h"
#include "gtest/gtest.h"
#include <vector>
#include "test_helper.h"
#include <list>
#include <deque>

using namespace abel;


// Template instantations.
// These tell the compiler to compile all the functions for the given class.
template
class abel::ring_buffer<int, std::vector<int> >;

template
class abel::ring_buffer<Align64, std::vector<Align64> >;

template
class abel::ring_buffer<TestObject, std::vector<TestObject> >;

template
class abel::ring_buffer<int, std::deque<int> >;

template
class abel::ring_buffer<Align64, std::deque<Align64> >;

template
class abel::ring_buffer<TestObject, std::deque<TestObject> >;

template
class abel::ring_buffer<int, std::list<int> >;

template
class abel::ring_buffer<Align64, std::list<Align64> >;

template
class abel::ring_buffer<TestObject, std::list<TestObject> >;

TEST(ring_buffer, ctor) {
    std::vector<int> emptyIntArray;
    ring_buffer<int, std::vector < int> > intRingBuffer(emptyIntArray);

    EXPECT_TRUE(intRingBuffer.validate());
    EXPECT_TRUE(intRingBuffer.capacity() == 0);

    intRingBuffer.resize(0);
    EXPECT_TRUE(intRingBuffer.validate());
    EXPECT_TRUE(intRingBuffer.size() == 0);

    intRingBuffer.resize(1);
    EXPECT_TRUE(intRingBuffer.validate());
    EXPECT_TRUE(intRingBuffer.size() == 1);
}
