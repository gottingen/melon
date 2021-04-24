// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include "gtest/gtest.h"
#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"


TEST(locked_table, locked_table_typedefs) {
    using Tbl = IntIntTable;
    using Ltbl = Tbl::locked_table;
    const bool key_type = std::is_same<Tbl::key_type, Ltbl::key_type>::value;
    const bool mapped_type =
            std::is_same<Tbl::mapped_type, Ltbl::mapped_type>::value;
    const bool value_type =
            std::is_same<Tbl::value_type, Ltbl::value_type>::value;
    const bool size_type = std::is_same<Tbl::size_type, Ltbl::size_type>::value;
    const bool difference_type =
            std::is_same<Tbl::difference_type, Ltbl::difference_type>::value;
    const bool hasher = std::is_same<Tbl::hasher, Ltbl::hasher>::value;
    const bool key_equal = std::is_same<Tbl::key_equal, Ltbl::key_equal>::value;
    const bool allocator_type =
            std::is_same<Tbl::allocator_type, Ltbl::allocator_type>::value;
    const bool reference = std::is_same<Tbl::reference, Ltbl::reference>::value;
    const bool const_reference =
            std::is_same<Tbl::const_reference, Ltbl::const_reference>::value;
    const bool pointer = std::is_same<Tbl::pointer, Ltbl::pointer>::value;
    const bool const_pointer =
            std::is_same<Tbl::const_pointer, Ltbl::const_pointer>::value;
    EXPECT_TRUE(key_type);
    EXPECT_TRUE(mapped_type);
    EXPECT_TRUE(value_type);
    EXPECT_TRUE(size_type);
    EXPECT_TRUE(difference_type);
    EXPECT_TRUE(hasher);
    EXPECT_TRUE(key_equal);
    EXPECT_TRUE(allocator_type);
    EXPECT_TRUE(reference);
    EXPECT_TRUE(const_reference);
    EXPECT_TRUE(pointer);
    EXPECT_TRUE(const_pointer);
}

TEST(locked_table, locked_table_move) {
    IntIntTable tbl;

    {
        auto lt = tbl.lock_table();
        auto lt2(std::move(lt));
        EXPECT_TRUE(!lt.is_active());
        EXPECT_TRUE(lt2.is_active());
    }

    {
        auto lt = tbl.lock_table();
        auto lt2 = std::move(lt);
        EXPECT_TRUE(!lt.is_active());
        EXPECT_TRUE(lt2.is_active());
    }

    {
        auto lt1 = tbl.lock_table();
        auto it1 = lt1.begin();
        auto it2 = lt1.begin();
        EXPECT_TRUE(it1 == it2);
        auto lt2(std::move(lt1));
        EXPECT_TRUE(it1 == it2);
    }
}

TEST(locked_table, locked_table_unlock) {
    IntIntTable tbl;
    tbl.insert(10, 10);
    auto lt = tbl.lock_table();
    lt.unlock();
    EXPECT_TRUE(!lt.is_active());
}

TEST(locked_table, locked_table_info) {
    IntIntTable tbl;
    tbl.insert(10, 10);
    auto lt = tbl.lock_table();
    EXPECT_TRUE(lt.is_active());

// We should still be able to call table info operations on the
// atomic_hash_map instance, because they shouldn't take locks.

    EXPECT_TRUE(lt.slot_per_bucket() == tbl.slot_per_bucket());
    EXPECT_TRUE(lt.get_allocator() == tbl.get_allocator());
    EXPECT_TRUE(lt.hash_power() == tbl.hash_power());
    EXPECT_TRUE(lt.bucket_count() == tbl.bucket_count());
    EXPECT_TRUE(lt.empty() == tbl.empty());
    EXPECT_TRUE(lt.size() == tbl.size());
    EXPECT_TRUE(lt.capacity() == tbl.capacity());
    EXPECT_TRUE(lt.load_factor() == tbl.load_factor());
    EXPECT_THROW(lt.minimum_load_factor(1.01), std::invalid_argument);
    lt.minimum_load_factor(lt.minimum_load_factor() * 2);
    lt.rehash(5);
    EXPECT_THROW(lt.maximum_hash_power(lt.hash_power() - 1),
                 std::invalid_argument);
    lt.maximum_hash_power(lt.hash_power() + 1);
    EXPECT_TRUE(lt.maximum_hash_power() == tbl.maximum_hash_power());
}

TEST(locked_table, locked_table_clear) {
    IntIntTable tbl;
    tbl.insert(10, 10);
    auto lt = tbl.lock_table();
    EXPECT_TRUE(lt.size() == 1);
    lt.clear();
    EXPECT_TRUE(lt.size() == 0);
    lt.clear();
    EXPECT_TRUE(lt.size() == 0);
}


TEST(locked_table, locked_table_insert_duplicate) {
    IntIntTable tbl;
    tbl.insert(10, 10);
    {
        auto lt = tbl.lock_table();
        auto result = lt.insert(10, 20);
        EXPECT_TRUE(result.first->first == 10);
        EXPECT_TRUE(result.first->second == 10);
        EXPECT_FALSE(result.second);
        result.first->second = 50;
    }
    EXPECT_TRUE(tbl.find(10) == 50);
}

TEST(locked_table, locked_table_insert_new_key) {
    IntIntTable tbl;
    tbl.insert(10, 10);
    {
        auto lt = tbl.lock_table();
        auto result = lt.insert(20, 20);
        EXPECT_TRUE(result.first->first == 20);
        EXPECT_TRUE(result.first->second == 20);
        EXPECT_TRUE(result.second);
        result.first->second = 50;
    }
    EXPECT_TRUE(tbl.find(10) == 10);
    EXPECT_TRUE(tbl.find(20) == 50);
}


TEST(locked_table, locked_table_insert_lifetime) {
    UniquePtrTable<int> tbl;

    {
        auto lt = tbl.lock_table();
        std::unique_ptr<int> key(new int(20));
        std::unique_ptr<int> value(new int(20));
        auto result = lt.insert(std::move(key), std::move(value));
        EXPECT_TRUE(*result.first->first == 20);
        EXPECT_TRUE(*result.first->second == 20);
        EXPECT_TRUE(result.second);
        EXPECT_TRUE(!static_cast<bool>(key));
        EXPECT_TRUE(!static_cast<bool>(value));
    }

    {
        tbl.insert(new int(20), new int(20));
        auto lt = tbl.lock_table();
        std::unique_ptr<int> key(new int(20));
        std::unique_ptr<int> value(new int(30));
        auto result = lt.insert(std::move(key), std::move(value));
        EXPECT_TRUE(*result.first->first == 20);
        EXPECT_TRUE(*result.first->second == 20);
        EXPECT_TRUE(!result.second);
        EXPECT_TRUE(static_cast<bool>(key));
        EXPECT_TRUE(static_cast<bool>(value));
    }
}

TEST(locked_table, locked_table_erase_sample) {
    IntIntTable tbl;
    for (int i = 0; i < 5; ++i) {
        tbl.insert(i, i);
    }
    using lt_t = IntIntTable::locked_table;

    {
        auto lt = tbl.lock_table();
        lt_t::const_iterator const_it;
        const_it = lt.find(0);
        EXPECT_TRUE(const_it != lt.end());
        lt_t::const_iterator const_next = const_it;
        ++const_next;
        EXPECT_TRUE(static_cast<lt_t::const_iterator>(lt.erase(const_it)) ==
                    const_next);
        EXPECT_TRUE(lt.size() == 4)<<lt.size();

        lt_t::iterator it;
        it = lt.find(1);
        lt_t::iterator next = it;
        ++next;
        EXPECT_TRUE(lt.erase(static_cast<lt_t::const_iterator>(it)) == next);
        EXPECT_TRUE(lt.size() == 3)<<lt.size();

        EXPECT_TRUE(lt.erase(2) == 1);
        EXPECT_TRUE(lt.size() == 2)<<lt.size();
    }
}

TEST(locked_table, locked_table_erase_this) {
    IntIntTable tbl;
    for (int i = 0; i < 5; ++i) {
        tbl.insert(i, i);
    }
    using lt_t = IntIntTable::locked_table;

    {
        auto lt = tbl.lock_table();
        auto it = lt.begin();
        auto next = it;
        ++next;
        EXPECT_TRUE(lt.erase(it) == next);
        ++it;
        EXPECT_TRUE(it->first > 0);
        EXPECT_TRUE(it->first < 5);
        EXPECT_TRUE(it->second > 0);
        EXPECT_TRUE(it->second < 5)<<lt.size();
    }
}


TEST(locked_table, locked_table_erase_other) {
    IntIntTable tbl;
    for (int i = 0; i < 5; ++i) {
        tbl.insert(i, i);
    }
    using lt_t = IntIntTable::locked_table;

    {
        auto lt = tbl.lock_table();
        auto it0 = lt.find(0);
        auto it1 = lt.find(1);
        auto it2 = lt.find(2);
        auto it3 = lt.find(3);
        auto it4 = lt.find(4);
        auto next = it2;
        ++next;
        EXPECT_TRUE(lt.erase(it2) == next)<<lt.size();
        EXPECT_EQ(it0->first , 0);
        EXPECT_EQ(it0->second , 0);
        EXPECT_EQ(it1->first , 1);
        EXPECT_EQ(it1->second , 1);
        EXPECT_EQ(it3->first , 3);
        EXPECT_EQ(it3->second , 3);
        EXPECT_EQ(it4->first , 4);
        EXPECT_EQ(it4->second , 4);
    }
}


TEST(locked_table, locked_table_find) {
    IntIntTable tbl;
    using lt_t = IntIntTable::locked_table;
    auto lt = tbl.lock_table();
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(lt.insert(i, i).second);
    }
    bool found_begin_elem = false;
    bool found_last_elem = false;
    for (int i = 0; i < 10; ++i) {
        lt_t::iterator it = lt.find(i);
        lt_t::const_iterator const_it = lt.find(i);
        EXPECT_TRUE(it != lt.end());
        EXPECT_TRUE(it->first == i);
        EXPECT_TRUE(it->second == i);
        EXPECT_TRUE(const_it != lt.end());
        EXPECT_TRUE(const_it->first == i);
        EXPECT_TRUE(const_it->second == i);
        it->second++;
        if (it == lt.begin()) {
            found_begin_elem = true;
        }
        if (++it == lt.end()) {
            found_last_elem = true;
        }
    }
    EXPECT_TRUE(found_begin_elem);
    EXPECT_TRUE(found_last_elem);
    for (int i = 0; i < 10; ++i) {
        lt_t::iterator it = lt.find(i);
        EXPECT_TRUE(it->first == i);
        EXPECT_TRUE(it->second == i + 1);
    }
}

TEST(locked_table, locked_table_at) {
    IntIntTable tbl;
    auto lt = tbl.lock_table();
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(lt.insert(i, i).second);
    }
    for (int i = 0; i < 10; ++i) {
        int &val = lt.at(i);
        const int &const_val =
                const_cast<const IntIntTable::locked_table &>(lt).at(i);
        EXPECT_TRUE(val == i);
        EXPECT_TRUE(const_val == i);
        ++val;
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(lt.at(i) == i + 1);
    }
    EXPECT_THROW(lt.at(11), std::out_of_range);
}

TEST(locked_table, locked_table_operator) {
    IntIntTable tbl;
    auto lt = tbl.lock_table();
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(lt.insert(i, i).second);
    }
    for (int i = 0; i < 10; ++i) {
        int &val = lt[i];
        EXPECT_TRUE(val == i);
        ++val;
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(lt[i] == i + 1);
    }
    EXPECT_TRUE(lt[11] == 0);
    EXPECT_TRUE(lt.at(11) == 0);
}

TEST(locked_table, locked_table_count) {
    IntIntTable tbl;
    auto lt = tbl.lock_table();
    for (int i = 0;i < 10; ++i) {
        EXPECT_TRUE(lt.insert(i, i).second);
    }
    for (int i = 0;i < 10; ++i) {
        EXPECT_TRUE(lt.count(i) == 1);
    }
    EXPECT_TRUE(lt.count(11) == 0);
}

TEST(locked_table, locked_table_equal_range) {
    IntIntTable tbl;
    using lt_t = IntIntTable::locked_table;
    auto lt = tbl.lock_table();
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(lt.insert(i, i).second);
    }
    for (int i = 0; i < 10; ++i) {
        std::pair<lt_t::iterator, lt_t::iterator> it_range = lt.equal_range(i);
        EXPECT_TRUE(it_range.first->first == i);
        EXPECT_TRUE(++it_range.first == it_range.second);
        std::pair<lt_t::const_iterator, lt_t::const_iterator> const_it_range =
                lt.equal_range(i);
        EXPECT_TRUE(const_it_range.first->first == i);
        EXPECT_TRUE(++const_it_range.first == const_it_range.second);
    }
    auto it_range = lt.equal_range(11);
    EXPECT_TRUE(it_range.first == lt.end());
    EXPECT_TRUE(it_range.second == lt.end());
}

TEST(locked_table, locked_table_rehash) {
    IntIntTable tbl(10);
    auto lt = tbl.lock_table();
    EXPECT_TRUE(lt.hash_power() == 2);
    lt.rehash(1);
    EXPECT_TRUE(lt.hash_power() == 1);
    lt.rehash(10);
    EXPECT_TRUE(lt.hash_power() == 10);
}

TEST(locked_table, locked_table_reserve) {
    IntIntTable tbl(10);
    auto lt = tbl.lock_table();
    EXPECT_TRUE(lt.hash_power() == 2);
    lt.reserve(1);
    EXPECT_TRUE(lt.hash_power() == 0);
    lt.reserve(4096);
    EXPECT_TRUE(lt.hash_power() == 10);
}

TEST(locked_table, locked_table_equality) {
    IntIntTable tbl1(40);
    auto lt1 = tbl1.lock_table();
    for (int i = 0; i < 10; ++i) {
        lt1.insert(i, i);
    }

    IntIntTable tbl2(30);
    auto lt2 = tbl2.lock_table();
    for (int i = 0; i < 10; ++i) {
        lt2.insert(i, i);
    }

    IntIntTable tbl3(30);
    auto lt3 = tbl3.lock_table();
    for (int i = 0; i < 10; ++i) {
        lt3.insert(i, i + 1);
    }

    IntIntTable tbl4(40);
    auto lt4 = tbl4.lock_table();
    for (int i = 0; i < 10; ++i) {
        lt4.insert(i + 1, i);
    }

    EXPECT_TRUE(lt1 == lt2);
    EXPECT_FALSE(lt2 != lt1);

    EXPECT_TRUE(lt1 != lt3);
    EXPECT_FALSE(lt3 == lt1);
    EXPECT_FALSE(lt2 == lt3);
    EXPECT_TRUE(lt3 != lt2);

    EXPECT_TRUE(lt1 != lt4);
    EXPECT_TRUE(lt4 != lt1);
    EXPECT_FALSE(lt3 == lt4);
    EXPECT_FALSE(lt4 == lt3);
}

template<typename Table>
void check_all_locks_taken(Table &tbl) {
    auto &locks = abel::UnitTestInternalAccess::get_current_locks(tbl);
    for (auto &lock : locks) {
        EXPECT_FALSE(lock.try_lock());
    }
}

TEST(locked_table, locked_table_holds_locks_after_resize) {
    IntIntTable tbl(4);
    auto lt = tbl.lock_table();
    check_all_locks_taken(tbl);

// After a cuckoo_fast_double, all locks are still taken
    for (int i = 0; i < 5; ++i) {
        lt.insert(i, i);
    }
    check_all_locks_taken(tbl);

// After a cuckoo_simple_expand, all locks are still taken
    lt.rehash(10);
    check_all_locks_taken(tbl);
}

TEST(locked_table, locked_table_IO) {
    IntIntTable tbl(0);
    auto lt = tbl.lock_table();
    for (int i = 0; i < 100; ++i) {
        lt.insert(i, i);
    }

    std::stringstream sstream;
    sstream << lt;

    IntIntTable tbl2;
    auto lt2 = tbl2.lock_table();
    sstream.seekg(0);
    sstream >> lt2;

    EXPECT_TRUE(100 == lt.size());
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(i == lt.at(i));
    }

    EXPECT_TRUE(100 == lt2.size());
    for (int i = 100; i < 1000; ++i) {
        lt2.insert(i, i);
    }
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(i == lt2.at(i));
    }
}

TEST(locked_table, empty_locked_table_IO) {
    IntIntTable tbl(0);
    auto lt = tbl.lock_table();
    lt.minimum_load_factor(0.5);
    lt.maximum_hash_power(10);

    std::stringstream sstream;
    sstream << lt;

    IntIntTable tbl2(0);
    auto lt2 = tbl2.lock_table();
    sstream.seekg(0);
    sstream >> lt2;

    EXPECT_TRUE(0 == lt.size());
    EXPECT_TRUE(0 == lt2.size());
    EXPECT_TRUE(0.5 == lt.minimum_load_factor());
    EXPECT_TRUE(10 == lt.maximum_hash_power());
}
