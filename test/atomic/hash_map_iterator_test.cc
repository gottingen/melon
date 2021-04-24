// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include "gtest/gtest.h"
#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"

TEST(iterator, iterator_types) {
    using Ltbl = IntIntTable::locked_table;
    using It = Ltbl::iterator;
    using ConstIt = Ltbl::const_iterator;

    const bool it_difference_type =
            std::is_same<Ltbl::difference_type, It::difference_type>::value;
    const bool it_value_type =
            std::is_same<Ltbl::value_type, It::value_type>::value;
    const bool it_pointer = std::is_same<Ltbl::pointer, It::pointer>::value;
    const bool it_reference = std::is_same<Ltbl::reference, It::reference>::value;
    const bool it_iterator_category =
            std::is_same<std::bidirectional_iterator_tag,
                    It::iterator_category>::value;

    const bool const_it_difference_type =
            std::is_same<Ltbl::difference_type, ConstIt::difference_type>::value;
    const bool const_it_value_type =
            std::is_same<Ltbl::value_type, ConstIt::value_type>::value;
    const bool const_it_reference =
            std::is_same<Ltbl::const_reference, ConstIt::reference>::value;
    const bool const_it_pointer =
            std::is_same<Ltbl::const_pointer, ConstIt::pointer>::value;
    const bool const_it_iterator_category =
            std::is_same<std::bidirectional_iterator_tag,
                    ConstIt::iterator_category>::value;

    EXPECT_TRUE(it_difference_type);
    EXPECT_TRUE(it_value_type);
    EXPECT_TRUE(it_pointer);
    EXPECT_TRUE(it_reference);
    EXPECT_TRUE(it_iterator_category);

    EXPECT_TRUE(const_it_difference_type);
    EXPECT_TRUE(const_it_value_type);
    EXPECT_TRUE(const_it_pointer);
    EXPECT_TRUE(const_it_reference);
    EXPECT_TRUE(const_it_iterator_category);
}

TEST(iterator, empty_table_iteration) {
    IntIntTable table;
    {
        auto lt = table.lock_table();
        EXPECT_TRUE(lt.begin() == lt.begin());
        EXPECT_TRUE(lt.begin() == lt.end());

        EXPECT_TRUE(lt.cbegin() == lt.begin());
        EXPECT_TRUE(lt.begin() == lt.end());

        EXPECT_TRUE(lt.cbegin() == lt.begin());
        EXPECT_TRUE(lt.cend() == lt.end());
    }
}

TEST(iterator, iterator_walkthrough) {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }

    {
        auto lt = table.lock_table();
        auto it = lt.cbegin();
        for (size_t i = 0; i < table.size(); ++i) {
            EXPECT_TRUE((*it).first == (*it).second);
            EXPECT_TRUE(it->first == it->second);
            auto old_it = it;
            EXPECT_TRUE(old_it == it++);
        }
        EXPECT_TRUE(it == lt.end());
    }

    {
        auto lt = table.lock_table();
        auto it = lt.cbegin();
        for (size_t i = 0; i < table.size(); ++i) {
            EXPECT_TRUE((*it).first == (*it).second);
            EXPECT_TRUE(it->first == it->second);
            ++it;
        }
        EXPECT_TRUE(it == lt.end());
    }

    {
        auto lt = table.lock_table();
        auto it = lt.cend();
        for (size_t i = 0; i < table.size(); ++i) {
            auto old_it = it;
            EXPECT_TRUE(old_it == it--);
            EXPECT_TRUE((*it).first == (*it).second);
            EXPECT_TRUE(it->first == it->second);
        }
        EXPECT_TRUE(it == lt.begin());
    }

    {
        auto lt = table.lock_table();
        auto it = lt.cend();
        for (size_t i = 0; i < table.size(); ++i) {
            --it;
            EXPECT_TRUE((*it).first == (*it).second);
            EXPECT_TRUE(it->first == it->second);
        }
        EXPECT_TRUE(it == lt.begin());
    }

    {
        auto lt = table.lock_table();
        auto it = lt.cend();
        auto lt2 = std::move(lt);
        for (size_t i = 0; i < table.size(); ++i) {
            --it;
            EXPECT_TRUE((*it).first == (*it).second);
            EXPECT_TRUE(it->first == it->second);
        }
        EXPECT_TRUE(it == lt2.begin());
    }
}

TEST(iterator, iterator_modification) {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }

    auto lt = table.lock_table();
    for (auto it = lt.begin(); it != lt.end(); ++it) {
        it->second = it->second + 1;
    }

    auto it = lt.cbegin();
    for (size_t i = 0; i < table.size(); ++i) {
        EXPECT_TRUE(it->first == it->second - 1);
        ++it;
    }
    EXPECT_TRUE(it == lt.end());
}

TEST(iterator, lock_table_blocks_inserts) {
    IntIntTable table;
    auto lt = table.lock_table();
    std::thread thread([&table]() {
        for (int i = 0; i < 10; ++i) {
            table.insert(i, i);
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(table.size() == 0);
    lt.unlock();
    thread.join();

    EXPECT_TRUE(table.size() == 10);
}

TEST(iterator, Cast_iterator_to_const_iterator) {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }
    auto lt = table.lock_table();
    for (IntIntTable::locked_table::iterator it = lt.begin(); it != lt.end();
         ++it) {
        EXPECT_TRUE(it->first == it->second);
        it->second++;
        IntIntTable::locked_table::const_iterator const_it = it;
        EXPECT_TRUE(it->first + 1 == it->second);
    }
}
