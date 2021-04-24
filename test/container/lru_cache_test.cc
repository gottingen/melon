// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/container/lru_cache.h"
#include "gtest/gtest.h"


TEST(LRUCacheTest, TestSetCapacityCase1) {
    abel::status s;
    std::string value;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(15);

    // ***************** Step 1 *****************
    // (k5, v5) -> (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1);
    lru_cache.insert("k1", "v1", 1);
    lru_cache.insert("k2", "v2", 2);
    lru_cache.insert("k3", "v3", 3);
    lru_cache.insert("k4", "v4", 4);
    lru_cache.insert("k5", "v5", 5);
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 15);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 2 *****************
    // (k5, v5) -> (k4, v4) -> (k3, v3)
    lru_cache.set_capacity(12);
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 12);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k3", "v3"}}));

    // ***************** Step 3 *****************
    // (k5, v5)
    lru_cache.set_capacity(5);
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}}));

    // ***************** Step 4 *****************
    // (k5, v5)
    lru_cache.set_capacity(15);
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}}));

    // ***************** Step 5 *****************
    // empty
    lru_cache.set_capacity(1);
    ASSERT_EQ(lru_cache.size(), 0);
    ASSERT_EQ(lru_cache.total_charge(), 0);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({}));
}

TEST(LRUCacheTest, TestLookupCase1) {
    abel::status s;
    std::string value;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(5);

    // ***************** Step 1 *****************
    // (k5, v5) -> (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1);
    lru_cache.insert("k1", "v1");
    lru_cache.insert("k2", "v2");
    lru_cache.insert("k3", "v3");
    lru_cache.insert("k4", "v4");
    lru_cache.insert("k5", "v5");
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 2 *****************
    // (k3, v3) -> (k5, v5) -> (k4, v4) -> (k2, v2) -> (k1, v1);
    s = lru_cache.lookup("k3", &value);
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k3", "v3"}, {"k5", "v5"}, {"k4", "v4"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 3 *****************
    // (k1, v1) -> (k3, v3) -> (k5, v5) -> (k4, v4) -> (k2, v2);
    s = lru_cache.lookup("k1", &value);
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k1", "v1"}, {"k3", "v3"}, {"k5", "v5"}, {"k4", "v4"}, {"k2", "v2"}}));

    // ***************** Step 4 *****************
    // (k4, v4) -> (k1, v1) -> (k3, v3) -> (k5, v5) -> (k2, v2);
    s = lru_cache.lookup("k4", &value);
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k1", "v1"}, {"k3", "v3"}, {"k5", "v5"}, {"k2", "v2"}}));

    // ***************** Step 5 *****************
    // (k5, v5) -> (k4, v4) -> (k1, v1) -> (k3, v3) -> (k2, v2);
    s = lru_cache.lookup("k5", &value);
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k1", "v1"}, {"k3", "v3"}, {"k2", "v2"}}));

    // ***************** Step 6 *****************
    // (k5, v5) -> (k4, v4) -> (k1, v1) -> (k3, v3) -> (k2, v2);
    s = lru_cache.lookup("k5", &value);
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k1", "v1"}, {"k3", "v3"}, {"k2", "v2"}}));
}


TEST(LRUCacheTest, TestInsertCase1) {
    abel::status s;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(3);

    // ***************** Step 1 *****************
    // (k1, v1)
    s = lru_cache.insert("k1", "v1");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 1);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k1", "v1"}}));

    // ***************** Step 2 *****************
    // (k2, v2) -> (k1, v1)
    s = lru_cache.insert("k2", "v2");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 2);
    ASSERT_EQ(lru_cache.total_charge(), 2);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 3 *****************
    // (k3, v3) -> (k2, v2) -> (k1, v1)
    s = lru_cache.insert("k3","v3");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 3);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 4 *****************
    // (k4, v4) -> (k3, v3) -> (k2, v2)
    s = lru_cache.insert("k4", "v4");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 3);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}}));

    // ***************** Step 5 *****************
    // (k5, v5) -> (k4, v4) -> (k3, v3)
    s = lru_cache.insert("k5", "v5");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 3);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k3", "v3"}}));
}

TEST(LRUCacheTest, TestInsertCase2) {
    abel::status s;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(5);

    // ***************** Step 1 *****************
    // (k5, v5) -> (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1)
    lru_cache.insert("k1", "v1");
    lru_cache.insert("k2", "v2");
    lru_cache.insert("k3", "v3");
    lru_cache.insert("k4", "v4");
    lru_cache.insert("k5", "v5");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 2 *****************
    // (k3, v3) -> (k5, v5) -> (k4, v4) -> (k2, v2) -> (k1, v1)
    s = lru_cache.insert("k3", "v3");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k3", "v3"}, {"k5", "v5"}, {"k4", "v4"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 3 *****************
    // (k2, v2) -> (k3, v3) -> (k5, v5) -> (k4, v4) -> (k1, v1)
    s = lru_cache.insert("k2", "v2");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k2", "v2"}, {"k3", "v3"}, {"k5", "v5"}, {"k4", "v4"}, {"k1", "v1"}}));

    // ***************** Step 4 *****************
    // (k1, v1) -> (k2, v2) -> (k3, v3) -> (k5, v5) -> (k4, v4)
    s = lru_cache.insert("k1", "v1");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, {"k5", "v5"}, {"k4", "v4"}}));

    // ***************** Step 5 *****************
    // (k4, v4) -> (k1, v1) -> (k2, v2) -> (k3, v3) -> (k5, v5)
    s = lru_cache.insert("k4", "v4");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, {"k5", "v5"}}));

    // ***************** Step 6 *****************
    // (k4, v4) -> (k1, v1) -> (k2, v2) -> (k3, v3) -> (k5, v5)
    s = lru_cache.insert("k4", "v4");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, {"k5", "v5"}}));

    // ***************** Step 6 *****************
    // (k4, v4) -> (k1, v1) -> (k2, v2) -> (k3, v3) -> (k5, v5)
    s = lru_cache.insert("k0", "v0");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k0", "v0"}, {"k4", "v4"}, {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}}));
}

TEST(LRUCacheTest, TestInsertCase3) {
    abel::status s;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(10);

    // ***************** Step 1 *****************
    // (k1, v1)
    s = lru_cache.insert("k1", "v1");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 1);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k1", "v1"}}));

    // ***************** Step 2 *****************
    // (k2, v2) -> (k1, v1)
    s = lru_cache.insert("k2", "v2", 2);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 2);
    ASSERT_EQ(lru_cache.total_charge(), 3);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 3 *****************
    // (k3, v3) -> (k2, v1) -> (k1, v1)
    s = lru_cache.insert("k3", "v3", 3);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 6);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 4 *****************
    // (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1)
    s = lru_cache.insert("k4", "v4", 4);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 4);
    ASSERT_EQ(lru_cache.total_charge(), 10);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 5 *****************
    // (k5, v5) -> (k4, v4)
    s = lru_cache.insert("k5", "v5", 5);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 2);
    ASSERT_EQ(lru_cache.total_charge(), 9);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}}));

    // ***************** Step 6 *****************
    // (k6, v6)
    s = lru_cache.insert("k6", "v6", 6);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 6);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k6", "v6"}}));
}


TEST(LRUCacheTest, TestInsertCase4) {
    abel::status s;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(10);

    // ***************** Step 1 *****************
    // (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1)
    lru_cache.insert("k1", "v1", 1);
    lru_cache.insert("k2", "v2", 2);
    lru_cache.insert("k3", "v3", 3);
    lru_cache.insert("k4", "v4", 4);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 4);
    ASSERT_EQ(lru_cache.total_charge(), 10);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 2 *****************
    // empty
    lru_cache.insert("k11", "v11", 11);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 0);
    ASSERT_EQ(lru_cache.total_charge(), 0);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({}));

    // ***************** Step 3 *****************
    // empty
    lru_cache.insert("k11", "v11", 11);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 0);
    ASSERT_EQ(lru_cache.total_charge(), 0);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({}));

    // ***************** Step 4 *****************
    // (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1)
    lru_cache.insert("k1", "v1", 1);
    lru_cache.insert("k2", "v2", 2);
    lru_cache.insert("k3", "v3", 3);
    lru_cache.insert("k4", "v4", 4);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 4);
    ASSERT_EQ(lru_cache.total_charge(), 10);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 5 *****************
    // (k5, k5) -> (k4, v4)
    lru_cache.insert("k5", "v5", 5);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 2);
    ASSERT_EQ(lru_cache.total_charge(), 9);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}}));

    // ***************** Step 6 *****************
    // (k1, v1) -> (k5, k5) -> (k4, v4)
    lru_cache.insert("k1", "v1", 1);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 10);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k1", "v1"}, {"k5", "v5"}, {"k4", "v4"}}));

    // ***************** Step 7 *****************
    // (k5, v5) -> (k1, k1) -> (k4, v4)
    lru_cache.insert("k5", "v5", 5);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 10);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k1", "v1"}, {"k4", "v4"}}));

    // ***************** Step 8 *****************
    // (k6, v6)
    lru_cache.insert("k6", "v6", 6);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 6);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k6", "v6"}}));

    // ***************** Step 8 *****************
    // (k2, v2) -> (k6, v6)
    lru_cache.insert("k2", "v2", 2);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 2);
    ASSERT_EQ(lru_cache.total_charge(), 8);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k2", "v2"}, {"k6", "v6"}}));

    // ***************** Step 9 *****************
    // (k1, v1) -> (k2, v2) -> (k6, v6)
    lru_cache.insert("k1", "v1", 1);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 9);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k1", "v1"}, {"k2", "v2"}, {"k6", "v6"}}));

    // ***************** Step 10 *****************
    // (k3, v3) -> (k1, v1) -> (k2, v2)
    lru_cache.insert("k3", "v3", 3);
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 6);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k3", "v3"}, {"k1", "v1"}, {"k2", "v2"}}));
}

TEST(LRUCacheTest, TestRemoveCase1) {
    abel::status s;
    abel::lru_cache<std::string, std::string> lru_cache;
    lru_cache.set_capacity(5);

    // ***************** Step 1 *****************
    // (k5, v5) -> (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1);
    lru_cache.insert("k1", "v1");
    lru_cache.insert("k2", "v2");
    lru_cache.insert("k3", "v3");
    lru_cache.insert("k4", "v4");
    lru_cache.insert("k5", "v5");
    ASSERT_EQ(lru_cache.size(), 5);
    ASSERT_EQ(lru_cache.total_charge(), 5);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k5", "v5"}, {"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 2 *****************
    // (k4, v4) -> (k3, v3) -> (k2, v2) -> (k1, v1);
    s = lru_cache.remove("k5");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 4);
    ASSERT_EQ(lru_cache.total_charge(), 4);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}, {"k1", "v1"}}));

    // ***************** Step 3 *****************
    // (k4, v4) -> (k3, v3) -> (k2, v2)
    s = lru_cache.remove("k1");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 3);
    ASSERT_EQ(lru_cache.total_charge(), 3);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k3", "v3"}, {"k2", "v2"}}));

    // ***************** Step 4 *****************
    // (k4, v4) -> (k2, v2)
    s = lru_cache.remove("k3");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 2);
    ASSERT_EQ(lru_cache.total_charge(), 2);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}, {"k2", "v2"}}));

    // ***************** Step 5 *****************
    // (k4, v4)
    s = lru_cache.remove("k2");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 1);
    ASSERT_EQ(lru_cache.total_charge(), 1);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({{"k4", "v4"}}));

    // ***************** Step 6 *****************
    // empty
    s = lru_cache.remove("k4");
    ASSERT_TRUE(s.is_ok());
    ASSERT_EQ(lru_cache.size(), 0);
    ASSERT_EQ(lru_cache.total_charge(), 0);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({}));

    // ***************** Step 7 *****************
    // empty
    s = lru_cache.remove("k4");
    ASSERT_TRUE(s.is_not_found());
    ASSERT_EQ(lru_cache.size(), 0);
    ASSERT_EQ(lru_cache.total_charge(), 0);
    ASSERT_TRUE(lru_cache.lru_and_handle_table_consistent());
    ASSERT_TRUE(lru_cache.lru_as_expected({}));
}
