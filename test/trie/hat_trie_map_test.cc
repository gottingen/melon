// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "testing/trie_test_helper.h"
#include "abel/trie/htrie_map.h"
#include "gtest/gtest.h"


TEST(hat_trie, test_insert_too_long_string) {
    abel::htrie_map<char, std::int64_t, abel::trie_internal::str_hash<char>, std::uint8_t> map;
    map.burst_threshold(8);

    for (std::size_t i = 0; i < 1000; i++) {
        map.insert(testing::utils::get_key<char>(i), testing::utils::get_value<std::int64_t>(i));
    }

    const std::string long_string(map.max_key_size(), 'a');
    EXPECT_TRUE(map.insert(long_string, testing::utils::get_value<std::int64_t>(1000)).second);

    const std::string too_long_string(map.max_key_size() + 1, 'a');
    EXPECT_THROW(map.insert(too_long_string, testing::utils::get_value<std::int64_t>(1001)),
                 std::length_error);
}


TEST(hat_trie, test_range_erase) {
    // insert x values, delete all except 14 first and 6 last value
    using TMap = abel::htrie_map<char, std::int64_t>;

    const std::size_t nb_values = 1000;
    TMap map = testing::utils::get_filled_map<TMap>(nb_values, 8);

    auto it_first = std::next(map.begin(), 14);
    auto it_last = std::next(map.begin(), 994);

    auto it = map.erase(it_first, it_last);
    EXPECT_EQ(std::distance(it, map.end()), 6);
    EXPECT_EQ(map.size(), 20);
    EXPECT_EQ(std::distance(map.begin(), map.end()), 20);
}

TEST(hat_trie, test_erase_with_empty_trie_node) {
    // Construct a hat-trie so that the multiple erases occur on trie_node without
    // any child.
    abel::htrie_map<char, int> map = {
            {"k11", 1},
            {"k12", 2},
            {"k13", 3},
            {"k14", 4}};
    map.burst_threshold(4);
    map.insert("k1", 5);
    map.insert("k", 6);
    map.insert("", 7);

    EXPECT_EQ(map.erase("k11"), 1);
    EXPECT_EQ(map.erase("k12"), 1);
    EXPECT_EQ(map.erase("k13"), 1);
    EXPECT_EQ(map.erase("k14"), 1);
    EXPECT_EQ(std::distance(map.begin(), map.end()), 3);

    EXPECT_EQ(map.erase("k1"), 1);
    EXPECT_EQ(std::distance(map.begin(), map.end()), 2);

    EXPECT_EQ(map.erase("k"), 1);
    EXPECT_EQ(std::distance(map.begin(), map.end()), 1);

    EXPECT_EQ(map.erase(""), 1);
    EXPECT_EQ(std::distance(map.begin(), map.end()), 0);
}

/**
 * emplace
 */
TEST(hat_trie, test_emplace) {
    abel::htrie_map<char, testing::move_only_test> map;
    map.emplace("test1", 1);
    map.emplace_ks("testIgnore", 4, 3);

    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.at("test1") == testing::move_only_test(1));
    EXPECT_TRUE(map.at("test") == testing::move_only_test(3));
}

/**
 * equal_prefix_range
 */
TEST(hat_trie, test_equal_prefix_range) {
    // Generate the sequence: Key 2, Key 20, 21, 22, ... , 29, 200, 201, 202, ...
    // , 299, 2000, 2001, ... , Key 2999
    std::set<std::string> sequence_set;
    for (std::size_t i = 1; i <= 1000; i = i * 10) {
        for (std::size_t j = 2 * i; j < 3 * i; j++) {
            sequence_set.insert("Key " + std::to_string(j));
        }
    }

    abel::htrie_map<char, int> map;
    map.burst_threshold(7);

    for (int i = 0; i < 4000; i++) {
        map.insert("Key " + std::to_string(i), i);
    }

    // Return sequence: Key 2, Key 20, 21, 22, ... , 29, 200, 201, 202, ... , 299,
    // 2000, 2001, ... , Key 2999
    auto range = map.equal_prefix_range("Key 2");
    EXPECT_EQ(std::distance(range.first, range.second), 1111);

    std::set<std::string> set;
    for (auto it = range.first; it != range.second; ++it) {
        set.insert(it.key());
    }
    EXPECT_EQ(set.size(), 1111);
    EXPECT_TRUE(set == sequence_set);

    range = map.equal_prefix_range("");
    EXPECT_EQ(std::distance(range.first, range.second), 4000);

    range = map.equal_prefix_range("Key 1000");
    EXPECT_EQ(std::distance(range.first, range.second), 1);
    EXPECT_EQ(range.first.key(), "Key 1000");

    range = map.equal_prefix_range("aKey 1000");
    EXPECT_EQ(std::distance(range.first, range.second), 0);

    range = map.equal_prefix_range("Key 30000");
    EXPECT_EQ(std::distance(range.first, range.second), 0);

    range = map.equal_prefix_range("Unknown");
    EXPECT_EQ(std::distance(range.first, range.second), 0);

    range = map.equal_prefix_range("KE");
    EXPECT_EQ(std::distance(range.first, range.second), 0);
}

TEST(hat_trie, test_equal_prefix_range_empty) {
    abel::htrie_map<char, int> map;

    auto range = map.equal_prefix_range("");
    EXPECT_EQ(std::distance(range.first, range.second), 0);

    range = map.equal_prefix_range("A");
    EXPECT_EQ(std::distance(range.first, range.second), 0);

    range = map.equal_prefix_range("Aa");
    EXPECT_EQ(std::distance(range.first, range.second), 0);
}

/**
 * longest_prefix
 */
TEST(hat_trie, test_longest_prefix) {
    abel::htrie_map<char, int> map(4);
    map = {{"a",       1},
           {"aa",      1},
           {"aaa",     1},
           {"aaaaa",   1},
           {"aaaaaa",  1},
           {"aaaaaaa", 1},
           {"ab",      1},
           {"abcde",   1},
           {"abcdf",   1},
           {"abcdg",   1},
           {"abcdh",   1},
           {"babc",    1}};

    EXPECT_EQ(map.longest_prefix("a").key(), "a");
    EXPECT_EQ(map.longest_prefix("aa").key(), "aa");
    EXPECT_EQ(map.longest_prefix("aaa").key(), "aaa");
    EXPECT_EQ(map.longest_prefix("aaaa").key(), "aaa");
    EXPECT_EQ(map.longest_prefix("ab").key(), "ab");
    EXPECT_EQ(map.longest_prefix("abc").key(), "ab");
    EXPECT_EQ(map.longest_prefix("abcd").key(), "ab");
    EXPECT_EQ(map.longest_prefix("abcdz").key(), "ab");
    EXPECT_EQ(map.longest_prefix("abcde").key(), "abcde");
    EXPECT_EQ(map.longest_prefix("abcdef").key(), "abcde");
    EXPECT_EQ(map.longest_prefix("abcdefg").key(), "abcde");
    EXPECT_TRUE(map.longest_prefix("dabc") == map.end());
    EXPECT_TRUE(map.longest_prefix("b") == map.end());
    EXPECT_TRUE(map.longest_prefix("bab") == map.end());
    EXPECT_TRUE(map.longest_prefix("babd") == map.end());
    EXPECT_TRUE(map.longest_prefix("") == map.end());

    map.insert("", 1);
    EXPECT_EQ(map.longest_prefix("dabc").key(), "");
    EXPECT_EQ(map.longest_prefix("").key(), "");
}

/**
 * erase_prefix
 */
TEST(hat_trie, test_erase_prefix) {
    abel::htrie_map<char, std::int64_t> map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(10000, 200);

    EXPECT_EQ(map.erase_prefix("Key 1"), 1111);
    EXPECT_EQ(map.size(), 8889);

    EXPECT_EQ(map.erase_prefix("Key 22"), 111);
    EXPECT_EQ(map.size(), 8778);

    EXPECT_EQ(map.erase_prefix("Key 333"), 11);
    EXPECT_EQ(map.size(), 8767);

    EXPECT_EQ(map.erase_prefix("Key 4444"), 1);
    EXPECT_EQ(map.size(), 8766);

    EXPECT_EQ(map.erase_prefix("Key 55555"), 0);
    EXPECT_EQ(map.size(), 8766);

    for (auto it = map.begin(); it != map.end(); ++it) {
        EXPECT_TRUE(it.key().find("Key 1") == std::string::npos);
        EXPECT_TRUE(it.key().find("Key 22") == std::string::npos);
        EXPECT_TRUE(it.key().find("Key 333") == std::string::npos);
        EXPECT_TRUE(it.key().find("Key 4444") == std::string::npos);
    }

    EXPECT_EQ(std::distance(map.begin(), map.end()), map.size());
}

TEST(hat_trie, test_erase_prefix_all_1) {
    abel::htrie_map<char, std::int64_t> map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(1000, 8);
    EXPECT_EQ(map.size(), 1000);
    EXPECT_EQ(map.erase_prefix(""), 1000);
    EXPECT_EQ(map.size(), 0);
}

TEST(hat_trie, test_erase_prefix_all_2) {
    abel::htrie_map<char, std::int64_t> map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(1000, 8);
    EXPECT_EQ(map.size(), 1000);
    EXPECT_EQ(map.erase_prefix("Ke"), 1000);
    EXPECT_EQ(map.size(), 0);
}

TEST(hat_trie, test_erase_prefix_none) {
    abel::htrie_map<char, std::int64_t> map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(1000, 8);
    EXPECT_EQ(map.erase_prefix("Kea"), 0);
    EXPECT_EQ(map.size(), 1000);
}

TEST(hat_trie, test_erase_prefix_empty_map) {
    abel::htrie_map<char, std::int64_t> map;
    EXPECT_EQ(map.erase_prefix("Kea"), 0);
    EXPECT_EQ(map.erase_prefix(""), 0);
}

/**
 * operator== and operator!=
 */
TEST(hat_trie, test_compare) {
    abel::htrie_map<char, std::int64_t> map = {
            {"test1", 10},
            {"test2", 20},
            {"test3", 30}};
    abel::htrie_map<char, std::int64_t> map2 = {
            {"test3", 30},
            {"test2", 20},
            {"test1", 10}};
    abel::htrie_map<char, std::int64_t> map3 = {
            {"test1", 10},
            {"test2", 20},
            {"test3", -1}};
    abel::htrie_map<char, std::int64_t> map4 = {{"test3", 30},
                                                {"test2", 20}};

    EXPECT_TRUE(map == map);
    EXPECT_TRUE(map2 == map2);
    EXPECT_TRUE(map3 == map3);
    EXPECT_TRUE(map4 == map4);

    EXPECT_TRUE(map == map2);
    EXPECT_TRUE(map != map3);
    EXPECT_TRUE(map != map4);
    EXPECT_TRUE(map2 != map3);
    EXPECT_TRUE(map2 != map4);
    EXPECT_TRUE(map3 != map4);
}

/**
 * clear
 */
TEST(hat_trie, test_clear) {
    abel::htrie_map<char, std::int64_t> map = {{"test1", 10},
                                               {"test2", 20}};

    map.clear();
    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.begin() == map.end());
    EXPECT_TRUE(map.cbegin() == map.cend());
}

/**
 * operator=
 */
TEST(hat_trie, test_assign_operator) {
    abel::htrie_map<char, std::int64_t> map = {{"test1", 10},
                                               {"test2", 20}};
    EXPECT_EQ(map.size(), 2);

    map = {{"test3", 30}};
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map.at("test3"), 30);
}

TEST(hat_trie, test_copy_operator) {
    abel::htrie_map<char, std::int64_t> map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(1000, 8);
    abel::htrie_map<char, std::int64_t> map2 = map;
    abel::htrie_map<char, std::int64_t> map3;
    map3 = map;

    EXPECT_TRUE(map == map2);
    EXPECT_TRUE(map == map3);
}

TEST(hat_trie, test_move_operator) {
    const std::size_t nb_elements = 1000;
    const abel::htrie_map<char, std::int64_t> init_map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(nb_elements, 8);

    abel::htrie_map<char, std::int64_t> map =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(nb_elements, 8);
    abel::htrie_map<char, std::int64_t> map2 =
            testing::utils::get_filled_map<abel::htrie_map<char, std::int64_t>>(1, 8);
    map2 = std::move(map);

    EXPECT_TRUE(map.empty());
    EXPECT_TRUE(map.begin() == map.end());
    EXPECT_EQ(map2.size(), nb_elements);
    EXPECT_TRUE(map2 == init_map);

    abel::htrie_map<char, std::int64_t> map3;
    map3 = std::move(map2);

    EXPECT_TRUE(map2.empty());
    EXPECT_TRUE(map2.begin() == map2.end());
    EXPECT_EQ(map3.size(), nb_elements);
    EXPECT_TRUE(map3 == init_map);

    map2 = {{"test1", 10}};
    EXPECT_TRUE(map2 == (abel::htrie_map<char, std::int64_t>{{"test1", 10}}));
}

/**
 * at
 */
TEST(hat_trie, test_at) {
    // insert x values, use at for known and unknown values.
    abel::htrie_map<char, std::int64_t> map = {{"test1", 10},
                                               {"test2", 20}};
    map.insert("test4", 40);

    EXPECT_EQ(map.at("test1"), 10);
    EXPECT_EQ(map.at("test2"), 20);
    EXPECT_THROW(map.at("test3"), std::out_of_range);
    EXPECT_EQ(map.at("test4"), 40);

    const abel::htrie_map<char, std::int64_t> map_const = {
            {"test1", 10},
            {"test2", 20},
            {"test4", 40}};

    EXPECT_EQ(map_const.at("test1"), 10);
    EXPECT_EQ(map_const.at("test2"), 20);
    EXPECT_THROW(map_const.at("test3"), std::out_of_range);
    EXPECT_EQ(map_const.at("test4"), 40);
}

/**
 * equal_range
 */
TEST(hat_trie, test_equal_range) {
    abel::htrie_map<char, std::int64_t> map = {{"test1", 10},
                                               {"test2", 20}};

    auto it_pair = map.equal_range("test1");
    EXPECT_EQ(std::distance(it_pair.first, it_pair.second), 1);
    EXPECT_EQ(it_pair.first.value(), 10);

    it_pair = map.equal_range("");
    EXPECT_TRUE(it_pair.first == it_pair.second);
    EXPECT_TRUE(it_pair.first == map.end());
}

/**
 * operator[]
 */
TEST(hat_trie, test_access_operator) {
    // insert x values, use at for known and unknown values.
    abel::htrie_map<char, std::int64_t> map = {{"test1", 10},
                                               {"test2", 20}};

    EXPECT_EQ(map["test1"], 10);
    EXPECT_EQ(map["test2"], 20);
    EXPECT_EQ(map["test3"], std::int64_t());

    map["test3"] = 30;
    EXPECT_EQ(map["test3"], 30);

    EXPECT_EQ(map.size(), 3);
}

/**
 * shrink_to_fit
 */
TEST(hat_trie, test_shrink_to_fit) {
    using TMap = abel::htrie_map<char, std::int64_t>;
    using char_tt = typename TMap::char_type;
    using value_tt = typename TMap::mapped_type;

    const std::size_t nb_elements = 4000;
    const std::size_t burst_threshold = 7;

    TMap map;
    TMap map2;

    map.burst_threshold(burst_threshold);
    map2.burst_threshold(burst_threshold);

    for (std::size_t i = 0; i < nb_elements / 2; i++) {
        map.insert(testing::utils::get_key<char_tt>(i), testing::utils::get_value<value_tt>(i));
        map2.insert(testing::utils::get_key<char_tt>(i), testing::utils::get_value<value_tt>(i));
    }

    EXPECT_TRUE(map == map2);
    map2.shrink_to_fit();
    EXPECT_TRUE(map == map2);

    for (std::size_t i = nb_elements / 2; i < nb_elements; i++) {
        map.insert(testing::utils::get_key<char_tt>(i), testing::utils::get_value<value_tt>(i));
        map2.insert(testing::utils::get_key<char_tt>(i), testing::utils::get_value<value_tt>(i));
    }

    EXPECT_TRUE(map == map2);
    map2.shrink_to_fit();
    EXPECT_TRUE(map == map2);
}

/**
 * swap
 */
TEST(hat_trie, test_swap) {
    abel::htrie_map<char, std::int64_t> map = {{"test1", 10},
                                               {"test2", 20}};
    abel::htrie_map<char, std::int64_t> map2 = {
            {"test3", 30},
            {"test4", 40},
            {"test5", 50}};

    using std::swap;
    swap(map, map2);

    EXPECT_TRUE(map == (abel::htrie_map<char, std::int64_t>{
            {"test3", 30},
            {"test4", 40},
            {"test5", 50}}));
    EXPECT_TRUE(map2 == (abel::htrie_map<char, std::int64_t>{{"test1", 10},
                                                             {"test2", 20}}));
}

/**
 * serialize and deserialize
 */
TEST(hat_trie, test_serialize_deserialize_empty_map) {
    // serialize empty map; deserialize in new map; check equal.
    // for deserialization, test it with and without hash compatibility.
    const abel::htrie_map<char, testing::move_only_test> empty_map;

    testing::serializer serial;
    empty_map.serialize(serial);

    testing::deserializer dserial(serial.str());
    auto empty_map_deserialized = decltype(empty_map)::deserialize(dserial, true);
    EXPECT_TRUE(empty_map_deserialized == empty_map);

    testing::deserializer dserial2(serial.str());
    empty_map_deserialized = decltype(empty_map)::deserialize(dserial2, false);
    EXPECT_TRUE(empty_map_deserialized == empty_map);
}

TEST(hat_trie, test_serialize_deserialize_map) {
    // insert x values; delete some values; serialize map; deserialize in new map;
    // check equal. for deserialization, test it with and without hash
    // compatibility.
    const std::size_t nb_values = 1000;

    abel::htrie_map<char, testing::move_only_test> map(7);

    map.insert("", testing::utils::get_value<testing::move_only_test>(0));
    for (std::size_t i = 1; i < nb_values + 40; i++) {
        map.insert(testing::utils::get_key<char>(i), testing::utils::get_value<testing::move_only_test>(i));
    }

    for (std::size_t i = nb_values; i < nb_values + 40; i++) {
        map.erase(testing::utils::get_key<char>(i));
    }
    EXPECT_EQ(map.size(), nb_values);

    testing::serializer serial;
    map.serialize(serial);

    testing::deserializer dserial(serial.str());
    auto map_deserialized = decltype(map)::deserialize(dserial, true);
    EXPECT_TRUE(map == map_deserialized);

    testing::deserializer dserial2(serial.str());
    map_deserialized = decltype(map)::deserialize(dserial2, false);
    EXPECT_TRUE(map_deserialized == map);
}

TEST(hat_trie, test_serialize_deserialize_with_different_hash) {
    // insert x values; delete some values; serialize map; deserialize it in a new
    // map with an incompatible hash; check equal.
    struct str_hash {
        std::size_t operator()(const char *key, std::size_t key_size) const {
            return abel::trie_internal::str_hash<char>()(key, key_size) + 123;
        }
    };

    const std::size_t nb_values = 1000;

    abel::htrie_map<char, testing::move_only_test> map(7);

    map.insert("", testing::utils::get_value<testing::move_only_test>(0));
    for (std::size_t i = 1; i < nb_values + 40; i++) {
        map.insert(testing::utils::get_key<char>(i), testing::utils::get_value<testing::move_only_test>(i));
    }

    for (std::size_t i = nb_values; i < nb_values + 40; i++) {
        map.erase(testing::utils::get_key<char>(i));
    }
    EXPECT_EQ(map.size(), nb_values);

    testing::serializer serial;
    map.serialize(serial);

    testing::deserializer dserial(serial.str());
    auto map_deserialized =
            abel::htrie_map<char, testing::move_only_test, str_hash>::deserialize(dserial);

    EXPECT_EQ(map.size(), map_deserialized.size());
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const auto it_element_rhs = map_deserialized.find(it.key());
        EXPECT_TRUE(it_element_rhs != map_deserialized.cend() &&
                    it.value() == it_element_rhs.value());
    }
}

TEST(hat_trie, test_serialize_deserialize_map_no_burst) {
    // test deserialization when there is only a hash node.
    // set burst_threshold to x+1; insert x values; serialize map; deserialize in
    // new map; check equal. for deserialization, test it with and without hash
    // compatibility.
    const std::size_t nb_values = 100;

    abel::htrie_map<char, testing::move_only_test> map(nb_values + 1);

    map.insert("", testing::utils::get_value<testing::move_only_test>(0));
    for (std::size_t i = 1; i < nb_values; i++) {
        map.insert(testing::utils::get_key<char>(i), testing::utils::get_value<testing::move_only_test>(i));
    }

    EXPECT_EQ(map.size(), nb_values);

    testing::serializer serial;
    map.serialize(serial);

    testing::deserializer dserial(serial.str());
    auto map_deserialized = decltype(map)::deserialize(dserial, true);
    EXPECT_TRUE(map == map_deserialized);

    testing::deserializer dserial2(serial.str());
    map_deserialized = decltype(map)::deserialize(dserial2, false);
    EXPECT_TRUE(map_deserialized == map);
}

/**
 * Various operations on empty map
 */
TEST(hat_trie, test_empty_map) {
    abel::htrie_map<char, int> map;

    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());

    EXPECT_TRUE(map.begin() == map.end());
    EXPECT_TRUE(map.begin() == map.cend());
    EXPECT_TRUE(map.cbegin() == map.cend());

    EXPECT_TRUE(map.find("") == map.end());
    EXPECT_TRUE(map.find("test") == map.end());

    EXPECT_EQ(map.count(""), 0);
    EXPECT_EQ(map.count("test"), 0);

    EXPECT_THROW(map.at(""), std::out_of_range);
    EXPECT_THROW(map.at("test"), std::out_of_range);

    auto range = map.equal_range("test");
    EXPECT_TRUE(range.first == range.second);

    auto range_prefix = map.equal_prefix_range("test");
    EXPECT_TRUE(range_prefix.first == range_prefix.second);

    EXPECT_TRUE(map.longest_prefix("test") == map.end());

    EXPECT_EQ(map.erase("test"), 0);
    EXPECT_TRUE(map.erase(map.begin(), map.end()) == map.end());

    EXPECT_EQ(map.erase_prefix("test"), 0);

    EXPECT_EQ(map["new value"], int{});
}
