// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "testing/trie_test_helper.h"
#include "abel/trie/htrie_set.h"
#include "gtest/gtest.h"


/**
 * operator=
 */
TEST(hat_set, test_assign_operator) {
    abel::htrie_set<char> set = {"test1", "test2"};
    EXPECT_EQ(set.size(), 2);

    set = {"test3"};
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.count("test3"), 1);
}

TEST(hat_set, test_copy_operator) {
    abel::htrie_set<char> set = {"test1", "test2", "test3", "test4"};
    abel::htrie_set<char> set2 = set;
    abel::htrie_set<char> set3;
    set3 = set;

    EXPECT_TRUE(set == set2);
    EXPECT_TRUE(set == set3);
}

TEST(hat_set, test_move_operator) {
    abel::htrie_set<char> set = {"test1", "test2"};
    abel::htrie_set<char> set2 = std::move(set);

    EXPECT_TRUE(set.empty());
    EXPECT_TRUE(set.begin() == set.end());
    EXPECT_EQ(set2.size(), 2);
    EXPECT_TRUE(set2 == (abel::htrie_set<char>{"test1", "test2"}));

    abel::htrie_set<char> set3;
    set3 = std::move(set2);

    EXPECT_TRUE(set2.empty());
    EXPECT_TRUE(set2.begin() == set2.end());
    EXPECT_EQ(set3.size(), 2);
    EXPECT_TRUE(set3 == (abel::htrie_set<char>{"test1", "test2"}));

    set2 = {"test1"};
    EXPECT_TRUE(set2 == (abel::htrie_set<char>{"test1"}));
}

/**
 * serialize and deserialize
 */
TEST(hat_set, test_serialize_deserialize) {
    // insert x values; delete some values; serialize set; deserialize in new set;
    // check equal. for deserialization, test it with and without hash
    // compatibility.
    const std::size_t nb_values = 1000;

    abel::htrie_set<char> set(0);

    set.insert("");
    for (std::size_t i = 1; i < nb_values + 40; i++) {
        set.insert(testing::utils::get_key<char>(i));
    }

    for (std::size_t i = nb_values; i < nb_values + 40; i++) {
        set.erase(testing::utils::get_key<char>(i));
    }
    EXPECT_EQ(set.size(), nb_values);

    testing::serializer serial;
    set.serialize(serial);

    testing::deserializer dserial(serial.str());
    auto set_deserialized = decltype(set)::deserialize(dserial, true);
    EXPECT_TRUE(set_deserialized == set);

    testing::deserializer dserial2(serial.str());
    set_deserialized = decltype(set)::deserialize(dserial2, false);
    EXPECT_TRUE(set_deserialized == set);
}
