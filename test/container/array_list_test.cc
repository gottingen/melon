//
// Created by liyinbin on 2020/1/31.
//


#include <gtest/gtest.h>
#include <abel/container/circular_buffer.h>
#include <abel/container/array_list.h>
#include <abel/algorithm/algorithm.h>
#include <deque>

using namespace abel;

TEST(array_list, chunked_fifo_small) {
    // Check all the methods of array_list but with a trivial type (int) and
    // only a few elements - and in particular a single chunk is enough.
    array_list<int> fifo;
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
    fifo.push_back(3);
    EXPECT_EQ(fifo.size(), 1u);
    EXPECT_EQ(fifo.empty(), false);
    EXPECT_EQ(fifo.front(), 3);
    fifo.push_back(17);
    EXPECT_EQ(fifo.size(), 2u);
    EXPECT_EQ(fifo.empty(), false);
    EXPECT_EQ(fifo.front(), 3);
    fifo.pop_front();
    EXPECT_EQ(fifo.size(), 1u);
    EXPECT_EQ(fifo.empty(), false);
    EXPECT_EQ(fifo.front(), 17);
    fifo.pop_front();
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
    // The previously allocated chunk should have been freed, and now
    // a new one will need to be allocated:
    fifo.push_back(57);
    EXPECT_EQ(fifo.size(), 1u);
    EXPECT_EQ(fifo.empty(), false);
    EXPECT_EQ(fifo.front(), 57);
    // check miscelleneous methods (at least they shouldn't crash)
    fifo.clear();
    fifo.shrink_to_fit();
    fifo.reserve(1);
    fifo.reserve(100);
    fifo.reserve(1280);
    fifo.shrink_to_fit();
    fifo.reserve(1280);
}

TEST(array_list, chunked_fifo_fullchunk) {
    // Grow a array_list to exactly fill a chunk, and see what happens when
    // we cross that chunk.
    constexpr size_t N = 128;
    array_list<int, N> fifo;
    for (int i = 0; i < static_cast<int>(N); i++) {
        fifo.push_back(i);
    }
    EXPECT_EQ(fifo.size(), N);
    fifo.push_back(N);
    EXPECT_EQ(fifo.size(), N + 1);
    for (int i = 0; i < static_cast<int>(N + 1); i++) {
        EXPECT_EQ(fifo.front(), i);
        EXPECT_EQ(fifo.size(), N + 1 - i);
        fifo.pop_front();
    }
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
}

TEST(array_list, chunked_fifo_big) {
    // Grow a array_list to many elements, and see things are working as
    // expected
    array_list<int> fifo;
    constexpr size_t N = 100000;
    for (int i = 0; i < static_cast<int>(N); i++) {
        fifo.push_back(i);
    }
    EXPECT_EQ(fifo.size(), N);
    EXPECT_EQ(fifo.empty(), false);
    for (int i = 0; i < static_cast<int>(N); i++) {
        EXPECT_EQ(fifo.front(), i);
        EXPECT_EQ(fifo.size(), N - i);
        fifo.pop_front();
    }
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
}

TEST(array_list, chunked_fifo_constructor) {
    // Check that array_list appropriately calls the type's constructor
    // and destructor, and doesn't need anything else.
    struct typ {
        int val;
        unsigned *constructed;
        unsigned *destructed;
        typ (int val, unsigned *constructed, unsigned *destructed)
            : val(val), constructed(constructed), destructed(destructed) {
            ++*constructed;
        }
        ~typ () { ++*destructed; }
    };
    array_list<typ> fifo;
    unsigned constructed = 0, destructed = 0;
    constexpr unsigned N = 1000;
    for (unsigned i = 0; i < N; i++) {
        fifo.emplace_back(i, &constructed, &destructed);
    }
    EXPECT_EQ(fifo.size(), N);
    EXPECT_EQ(constructed, N);
    EXPECT_EQ(destructed, 0u);
    for (unsigned i = 0; i < N; i++) {
        EXPECT_EQ(fifo.front().val, static_cast<int>(i));
        EXPECT_EQ(fifo.size(), N - i);
        fifo.pop_front();
        EXPECT_EQ(destructed, i + 1);
    }
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
    // Check that destructing a fifo also destructs the objects it still
    // contains
    constructed = destructed = 0;
    {
        array_list<typ> fifo;
        for (unsigned i = 0; i < N; i++) {
            fifo.emplace_back(i, &constructed, &destructed);
            EXPECT_EQ(fifo.front().val, 0);
            EXPECT_EQ(fifo.size(), i + 1);
            EXPECT_EQ(fifo.empty(), false);
            EXPECT_EQ(constructed, i + 1);
            EXPECT_EQ(destructed, 0u);
        }
    }
    EXPECT_EQ(constructed, N);
    EXPECT_EQ(destructed, N);
}

TEST(array_list, chunked_fifo_construct_fail) {
    // Check that if we fail to construct the item pushed, the queue remains
    // empty.
    class my_exception { };
    struct typ {
        typ () {
            throw my_exception();
        }
    };
    array_list<typ> fifo;
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
    try {
        fifo.emplace_back();
    } catch (my_exception) {
        // expected, ignore
    }
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
}

TEST(array_list, chunked_fifo_construct_fail2) {
    // A slightly more elaborate test, with a chunk size of 2
    // items, and the third addition failing, so the question is
    // not whether empty() is wrong immediately, but whether after
    // we pop the two items, it will become true or we'll be left
    // with an empty chunk.
    class my_exception { };
    struct typ {
        typ (bool fail) {
            if (fail) {
                throw my_exception();
            }
        }
    };
    array_list<typ, 2> fifo;
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
    fifo.emplace_back(false);
    fifo.emplace_back(false);
    try {
        fifo.emplace_back(true);
    } catch (my_exception) {
        // expected, ignore
    }
    EXPECT_EQ(fifo.size(), 2u);
    EXPECT_EQ(fifo.empty(), false);
    fifo.pop_front();
    EXPECT_EQ(fifo.size(), 1u);
    EXPECT_EQ(fifo.empty(), false);
    fifo.pop_front();
    EXPECT_EQ(fifo.size(), 0u);
    EXPECT_EQ(fifo.empty(), true);
}

TEST(array_list, chunked_fifo_iterator) {
    constexpr auto items_per_chunk = 8;
    auto fifo = array_list<int, items_per_chunk> {};
    auto reference = std::deque<int> {};

    EXPECT_TRUE(fifo.begin() == fifo.end());

    for (int i = 0; i < items_per_chunk * 4; ++i) {
        fifo.push_back(i);
        reference.push_back(i);
        EXPECT_TRUE(abel::equal(fifo.begin(), fifo.end(), reference.begin(), reference.end()));
    }

    for (int i = 0; i < items_per_chunk * 2; ++i) {
        fifo.pop_front();
        reference.pop_front();
        EXPECT_TRUE(abel::equal(fifo.begin(), fifo.end(), reference.begin(), reference.end()));
    }
}
