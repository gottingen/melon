//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/utility/string_printf.h>
#include <turbo/log/logging.h>
#include <melon/utility/containers/hash_tables.h>
#include <melon/utility/containers/flat_map.h>
#include "melon/utility/containers/pooled_map.h"
#include <melon/utility/containers/case_ignored_flat_map.h>

namespace {
class FlatMapTest : public ::testing::Test{
protected:
    FlatMapTest(){};
    virtual ~FlatMapTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

int g_foo_ctor = 0;
int g_foo_copy_ctor = 0;
int g_foo_assign = 0;
struct Foo {
    Foo() { ++g_foo_ctor; }
    Foo(const Foo&) { ++g_foo_copy_ctor; }
    void operator=(const Foo&) { ++g_foo_assign; }
};
struct Bar {
    int x;
};

TEST_F(FlatMapTest, initialization_of_values) {
    // Construct non-POD values w/o copy-construction.
    mutil::FlatMap<int, Foo> map;
    ASSERT_EQ(0, map.init(32));
    ASSERT_EQ(0, g_foo_ctor);
    ASSERT_EQ(0, g_foo_copy_ctor);
    ASSERT_EQ(0, g_foo_assign);
    map[1];
    ASSERT_EQ(1, g_foo_ctor);
    ASSERT_EQ(0, g_foo_copy_ctor);
    ASSERT_EQ(0, g_foo_assign);

    // Zeroize POD values.
    mutil::FlatMap<int, Bar> map2;
    ASSERT_EQ(0, map2.init(32));
    Bar& g = map2[1];
    ASSERT_EQ(0, g.x);
    g.x = 123;
    ASSERT_EQ(1u, map2.erase(1));
    ASSERT_EQ(123, g.x); // g is still accessible in this case.
    Bar& g2 = map2[1];
    ASSERT_EQ(&g, &g2);
    ASSERT_EQ(0, g2.x);
}

TEST_F(FlatMapTest, swap_pooled_allocator) {
    mutil::details::PooledAllocator<int, 128> a1;
    a1.allocate(1);
    const void* p1 = a1._pool._blocks;

    mutil::details::PooledAllocator<int, 128> a2;
    a2.allocate(1);
    const void* p2 = a2._pool._blocks;

    std::swap(a1, a2);

    ASSERT_EQ(p2, a1._pool._blocks);
    ASSERT_EQ(p1, a2._pool._blocks);
}

TEST_F(FlatMapTest, copy_flat_map) {
    typedef mutil::FlatMap<std::string, std::string> Map;
    Map uninit_m1;
    ASSERT_FALSE(uninit_m1.initialized());
    ASSERT_TRUE(uninit_m1.empty());
    // self assignment does nothing.
    uninit_m1 = uninit_m1;
    ASSERT_FALSE(uninit_m1.initialized());
    ASSERT_TRUE(uninit_m1.empty());
    // Copy construct from uninitialized map.
    Map uninit_m2 = uninit_m1;
    ASSERT_FALSE(uninit_m2.initialized());
    ASSERT_TRUE(uninit_m2.empty());
    // assign uninitialized map to uninitialized map.
    Map uninit_m3;
    uninit_m3 = uninit_m1;
    ASSERT_FALSE(uninit_m3.initialized());
    ASSERT_TRUE(uninit_m3.empty());
    // assign uninitialized map to initialized map.
    Map init_m4;
    ASSERT_EQ(0, init_m4.init(16));
    ASSERT_TRUE(init_m4.initialized());
    init_m4["hello"] = "world";
    ASSERT_EQ(1u, init_m4.size());
    init_m4 = uninit_m1;
    ASSERT_TRUE(init_m4.initialized());
    ASSERT_TRUE(init_m4.empty());

    Map m1;
    ASSERT_EQ(0, m1.init(16));
    size_t expected_count = 0;
    m1["hello"] = "world"; ++expected_count;
    m1["foo"] = "bar"; ++ expected_count;
    m1["friend"] = "alice"; ++ expected_count;
    m1["zone"] = "bj-01"; ++ expected_count;
    m1["city"] = "shanghai"; ++ expected_count;
    m1["owner"] = "bob"; ++ expected_count;
    m1["lang"] = "chinese"; ++ expected_count;
    ASSERT_TRUE(m1.initialized());
    ASSERT_EQ(expected_count, m1.size());
    // self assignment does nothing.
    m1 = m1;
    ASSERT_EQ(expected_count, m1.size());
    ASSERT_EQ("world", m1["hello"]);
    ASSERT_EQ("bar", m1["foo"]);
    ASSERT_EQ("bob", m1["owner"]);
    ASSERT_EQ("bj-01", m1["zone"]);
    ASSERT_EQ("shanghai", m1["city"]);
    ASSERT_EQ("chinese", m1["lang"]);
    ASSERT_EQ("alice", m1["friend"]);
    // Copy construct from initialized map.
    Map m2 = m1;
    ASSERT_TRUE(m2.initialized());
    ASSERT_EQ(expected_count, m2.size());
    ASSERT_EQ("world", m2["hello"]);
    ASSERT_EQ("bar", m2["foo"]);
    ASSERT_EQ("bob", m2["owner"]);
    ASSERT_EQ("bj-01", m2["zone"]);
    ASSERT_EQ("shanghai", m2["city"]);
    ASSERT_EQ("chinese", m2["lang"]);
    ASSERT_EQ("alice", m2["friend"]);
    // assign initialized map to uninitialized map.
    Map m3;
    m3 = m1;
    ASSERT_TRUE(m3.initialized());
    ASSERT_EQ(expected_count, m3.size());
    ASSERT_EQ("world", m3["hello"]);
    ASSERT_EQ("bar", m3["foo"]);
    ASSERT_EQ("bob", m3["owner"]);
    ASSERT_EQ("bj-01", m3["zone"]);
    ASSERT_EQ("shanghai", m3["city"]);
    ASSERT_EQ("chinese", m3["lang"]);
    ASSERT_EQ("alice", m3["friend"]);
    // assign initialized map to initialized map (triggering resize)
    Map m4;
    ASSERT_EQ(0, m4.init(2));
    ASSERT_LE(m4.bucket_count(), m1.bucket_count());
    const void* old_buckets4 = m4._buckets;
    m4 = m1;
    ASSERT_EQ(m1.bucket_count(), m4.bucket_count());
    ASSERT_NE(old_buckets4, m4._buckets);
    ASSERT_EQ(expected_count, m4.size());
    ASSERT_EQ("world", m4["hello"]);
    ASSERT_EQ("bar", m4["foo"]);
    ASSERT_EQ("bob", m4["owner"]);
    ASSERT_EQ("bj-01", m4["zone"]);
    ASSERT_EQ("shanghai", m4["city"]);
    ASSERT_EQ("chinese", m4["lang"]);
    ASSERT_EQ("alice", m4["friend"]);
    // assign initialized map to initialized map (no resize)
    const size_t bcs[] = { m1.bucket_count(), 32 };
    // less than m1.bucket_count but enough for holding the elements
    ASSERT_LE(bcs[0], m1.bucket_count());
    // larger than m1.bucket_count
    ASSERT_GE(bcs[1], m1.bucket_count());
    for (size_t i = 0; i < arraysize(bcs); ++i) {
        Map m5;
        ASSERT_EQ(0, m5.init(bcs[i]));
        const size_t old_bucket_count5 = m5.bucket_count();
        const void* old_buckets5 = m5._buckets;
        m5 = m1;
        ASSERT_EQ(old_bucket_count5, m5.bucket_count());
        ASSERT_EQ(old_buckets5, m5._buckets);
        ASSERT_EQ(expected_count, m5.size());
        ASSERT_EQ("world", m5["hello"]);
        ASSERT_EQ("bar", m5["foo"]);
        ASSERT_EQ("bob", m5["owner"]);
        ASSERT_EQ("bj-01", m5["zone"]);
        ASSERT_EQ("shanghai", m5["city"]);
        ASSERT_EQ("chinese", m5["lang"]);
        ASSERT_EQ("alice", m5["friend"]);
    }
}

TEST_F(FlatMapTest, seek_by_string_piece) {
    mutil::FlatMap<std::string, int> m;
    ASSERT_EQ(0, m.init(16));
    m["hello"] = 1;
    m["world"] = 2;
    mutil::StringPiece k1("hello");
    ASSERT_TRUE(m.seek(k1));
    ASSERT_EQ(1, *m.seek(k1));
    mutil::StringPiece k2("world");
    ASSERT_TRUE(m.seek(k2));
    ASSERT_EQ(2, *m.seek(k2));
    mutil::StringPiece k3("heheda");
    ASSERT_TRUE(m.seek(k3) == NULL);
}

TEST_F(FlatMapTest, to_lower) {
    for (int c = -128; c < 128; ++c) {
        ASSERT_EQ((char)::tolower(c), mutil::ascii_tolower(c)) << "c=" << c;
    }
    
    const size_t input_len = 102;
    char input[input_len + 1];
    char input2[input_len + 1];
    for (size_t i = 0; i < input_len; ++i) {
        int choice = rand() % 52;
        if (choice < 26) {
            input[i] = 'A' + choice;
            input2[i] = 'A' + choice;
        } else {
            input[i] = 'a' + choice - 26;
            input2[i] = 'a' + choice - 26;
        }
    }
    input[input_len] = '\0';
    input2[input_len] = '\0';
    mutil::Timer tm1;
    mutil::Timer tm2;
    mutil::Timer tm3;
    int sum = 0;
    tm1.start();
    sum += strcasecmp(input, input2);
    tm1.stop();
    tm3.start();
    sum += strncasecmp(input, input2, input_len);
    tm3.stop();
    tm2.start();
    sum += memcmp(input, input2, input_len);
    tm2.stop();
    LOG(INFO) << "tm1=" << tm1.n_elapsed()
              << " tm2=" << tm2.n_elapsed()
              << " tm3=" << tm3.n_elapsed() << " " << sum;
}

TEST_F(FlatMapTest, __builtin_ctzl_perf) {
    int s = 0;
    const size_t N = 10000;
    mutil::Timer tm1;
    tm1.start();
    for (size_t i = 1; i <= N; ++i) {
        s += __builtin_ctzl(i);
    }
    tm1.stop();
    LOG(INFO) << "__builtin_ctzl takes " << tm1.n_elapsed()/(double)N << "ns s=" << s;
}

TEST_F(FlatMapTest, case_ignored_map) {
    mutil::CaseIgnoredFlatMap<int> m1;
    ASSERT_EQ(0, m1.init(32));
    m1["Content-Type"] = 1;
    m1["content-Type"] = 10;
    m1["Host"] = 2;
    m1["HOST"] = 20;
    m1["Cache-Control"] = 3;
    m1["CachE-ControL"] = 30;
    ASSERT_EQ(10, m1["cONTENT-tYPE"]);
    ASSERT_EQ(20, m1["hOST"]);
    ASSERT_EQ(30, m1["cache-control"]);
}

TEST_F(FlatMapTest, case_ignored_set) {
    mutil::CaseIgnoredFlatSet s1;
    ASSERT_EQ(0, s1.init(32));
    s1.insert("Content-Type");
    ASSERT_EQ(1ul, s1.size());
    s1.insert("Content-TYPE");
    ASSERT_EQ(1ul, s1.size());
    s1.insert("Host");
    ASSERT_EQ(2ul, s1.size());
    s1.insert("HOST");
    ASSERT_EQ(2ul, s1.size());
    s1.insert("Cache-Control");
    ASSERT_EQ(3ul, s1.size());
    s1.insert("CachE-ControL");
    ASSERT_EQ(3ul, s1.size());
    ASSERT_TRUE(s1.seek("cONTENT-tYPE"));
    ASSERT_TRUE(s1.seek("hOST"));
    ASSERT_TRUE(s1.seek("cache-control"));
}


TEST_F(FlatMapTest, make_sure_all_methods_compile) {
    typedef mutil::FlatMap<int, long> M1;
    M1 m1;
    ASSERT_EQ(0, m1.init(32));
    ASSERT_EQ(0u, m1.size());
    m1[1] = 10;
    ASSERT_EQ(10, m1[1]);
    ASSERT_EQ(1u, m1.size());
    m1[2] = 20;
    ASSERT_EQ(20, m1[2]);
    ASSERT_EQ(2u, m1.size());
    m1.insert(1, 100);
    m1.insert({3, 30});
    ASSERT_EQ(100, m1[1]);
    ASSERT_EQ(3u, m1.size());
    ASSERT_TRUE(m1.seek(3));
    ASSERT_EQ(NULL, m1.seek(4));
    ASSERT_EQ(1u, m1.erase(3));
    ASSERT_EQ(0u, m1.erase(4));
    ASSERT_EQ(2u, m1.size());
    ASSERT_EQ(1u, m1.erase(2));
    ASSERT_EQ(1u, m1.size());
    for (M1::iterator it = m1.begin(); it != m1.end(); ++it) {
        std::cout << "[" << it->first << "," << it->second << "] ";
    }
    std::cout << std::endl;
    for (M1::const_iterator it = m1.begin(); it != m1.end(); ++it) {
        std::cout << "[" << it->first << "," << it->second << "] ";
    }
    std::cout << std::endl;

    typedef mutil::FlatSet<int> S1;
    S1 s1;
    ASSERT_EQ(0, s1.init(32));
    ASSERT_EQ(0u, s1.size());
    s1.insert(1);
    ASSERT_TRUE(s1.seek(1));
    ASSERT_EQ(1u, s1.size());
    s1.insert(2);
    ASSERT_TRUE(s1.seek(2));
    ASSERT_EQ(2u, s1.size());
    s1.insert(1);
    ASSERT_TRUE(s1.seek(1));
    ASSERT_EQ(2u, s1.size());
    ASSERT_EQ(NULL, s1.seek(3));
    ASSERT_EQ(0u, s1.erase(3));
    ASSERT_EQ(2u, s1.size());
    ASSERT_EQ(1u, s1.erase(2));
    ASSERT_EQ(1u, s1.size());
    for (S1::iterator it = s1.begin(); it != s1.end(); ++it) {
        std::cout << "[" << *it << "] ";
    }
    std::cout << std::endl;
    for (S1::const_iterator it = s1.begin(); it != s1.end(); ++it) {
        std::cout << "[" << *it << "] ";
    }
    std::cout << std::endl;
}

TEST_F(FlatMapTest, flat_map_of_string) {
    std::vector<std::string> keys;
    mutil::FlatMap<std::string, size_t> m1;
    std::map<std::string, size_t> m2;
    mutil::hash_map<std::string, size_t> m3;
    const size_t N = 10000;
    ASSERT_EQ(0, m1.init(N));
    mutil::Timer tm1, tm1_2, tm2, tm3;
    size_t sum = 0;
    keys.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        keys.push_back(mutil::string_printf("up_latency_as_key_%lu", i));
    }
    
    tm1.start();
    for (size_t i = 0; i < N; ++i) {
        m1[keys[i]] += i;
    }
    tm1.stop();
    tm2.start();
    for (size_t i = 0; i < N; ++i) {
        m2[keys[i]] += i;
    }
    tm2.stop();
    tm3.start();
    for (size_t i = 0; i < N; ++i) {
        m3[keys[i]] += i;
    }
    tm3.stop();
    LOG(INFO) << "inserting strings takes " << tm1.n_elapsed() / N
              << " " << tm2.n_elapsed() / N
              << " " << tm3.n_elapsed() / N;

    tm1.start();
    for (size_t i = 0; i < N; ++i) {
        sum += *m1.seek(keys[i]);
    }
    tm1.stop();
    tm2.start();
    for (size_t i = 0; i < N; ++i) {
        sum += m2.find(keys[i])->second;
    }
    tm2.stop();
    tm3.start();
    for (size_t i = 0; i < N; ++i) {
        sum += m3.find(keys[i])->second;
    }
    tm3.stop();
    LOG(INFO) << "finding strings takes " << tm1.n_elapsed()/N
              << " " << tm2.n_elapsed()/N << " " << tm3.n_elapsed()/N;

    tm1.start();
    for (size_t i = 0; i < N; ++i) {
        sum += *m1.seek(keys[i].c_str());
    }
    tm1.stop();
    tm2.start();
    for (size_t i = 0; i < N; ++i) {
        sum += m2.find(keys[i].c_str())->second;
    }
    tm2.stop();
    tm3.start();
    for (size_t i = 0; i < N; ++i) {
        sum += m3.find(keys[i].c_str())->second;
    }
    tm3.stop();
    tm1_2.start();
    for (size_t i = 0; i < N; ++i) {
        sum += *find_cstr(m1, keys[i].c_str());
    }
    tm1_2.stop();

    LOG(INFO) << "finding c_strings takes " << tm1.n_elapsed()/N
              << " " << tm2.n_elapsed()/N << " " << tm3.n_elapsed()/N
              << " " << tm1_2.n_elapsed()/N << " sum=" << sum;
    
    for (size_t i = 0; i < N; ++i) {
        ASSERT_EQ(i, m1[keys[i]]) << "i=" << i;
        ASSERT_EQ(i, m2[keys[i]]);
        ASSERT_EQ(i, m3[keys[i]]);
    }
}

TEST_F(FlatMapTest, fast_iterator) {
    typedef mutil::FlatMap<uint64_t, uint64_t> M1;
    typedef mutil::SparseFlatMap<uint64_t, uint64_t> M2;

    M1 m1;
    M2 m2;

    ASSERT_EQ(0, m1.init(16384));
    ASSERT_EQ(-1, m1.init(1));
    ASSERT_EQ(0, m2.init(16384));

    ASSERT_EQ(NULL, m1._thumbnail);
    ASSERT_TRUE(NULL != m2._thumbnail);

    const size_t N = 170;
    std::vector<uint64_t> keys;
    keys.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        keys.push_back(rand());
    }

    mutil::Timer tm2;
    tm2.start();
    for (size_t i = 0; i < N; ++i) {
        m2[keys[i]] = i;
    }
    tm2.stop();

    mutil::Timer tm1;
    tm1.start();
    for (size_t i = 0; i < N; ++i) {
        m1[keys[i]] = i;
    }
    tm1.stop();

    LOG(INFO) << "m1.insert=" << tm1.n_elapsed()/(double)N
              << "ns m2.insert=" << tm2.n_elapsed()/(double)N;
    tm1.start();
    for (M1::iterator it = m1.begin(); it != m1.end(); ++it);
    tm1.stop();

    tm2.start();
    for (M2::iterator it = m2.begin(); it != m2.end(); ++it);
    tm2.stop();
    LOG(INFO) << "m1.iterate=" << tm1.n_elapsed()/(double)N
              << "ns m2.iterate=" << tm2.n_elapsed()/(double)N;

    M1::iterator it1 = m1.begin();
    M2::iterator it2 = m2.begin();
    for ( ; it1 != m1.end() && it2 != m2.end(); ++it1, ++it2) {
        ASSERT_EQ(it1->first, it2->first);
        ASSERT_EQ(it1->second, it2->second);
    }
    ASSERT_EQ(m1.end(), it1);
    ASSERT_EQ(m2.end(), it2);
}

template <typename Key, typename Value, typename OnPause>
static void list_flat_map(std::vector<Key>* keys,
                          const mutil::FlatMap<Key, Value>& map,
                          size_t max_one_pass,
                          OnPause& on_pause) {
    keys->clear();
    typedef mutil::FlatMap<Key, Value> Map;
    size_t n = 0;
    for (typename Map::const_iterator it = map.begin(); it != map.end(); ++it) {
        if (++n >= max_one_pass) {
            typename Map::PositionHint hint;
            map.save_iterator(it, &hint);
            n = 0;
            on_pause(hint);
            it = map.restore_iterator(hint);
            if (it == map.begin()) { // resized
                keys->clear();
            }
            if (it == map.end()) {
                break;
            }
        }
        keys->push_back(it->first);
    }
}

typedef mutil::FlatMap<uint64_t, uint64_t> PositionHintMap;

static void fill_position_hint_map(PositionHintMap* map,
                                   std::vector<uint64_t>* keys) {
    srand(time(NULL));
    const size_t N = 170;
    if (!map->initialized()) {
        ASSERT_EQ(0, map->init(N * 3 / 2, 80));
    }
    
    keys->reserve(N);
    keys->clear();
    map->clear();
    for (size_t i = 0; i < N; ++i) {
        uint64_t key = rand();
        if (map->seek(key)) {
            continue;
        }
        keys->push_back(key);
        (*map)[key] = i;
    }
    LOG(INFO) << map->bucket_info();
}

struct CountOnPause {
    CountOnPause() : num_paused(0) {}
    void operator()(const PositionHintMap::PositionHint&) {
        ++num_paused;
    }
    size_t num_paused;
};

TEST_F(FlatMapTest, do_nothing_during_iteration) {
    PositionHintMap m1;
    std::vector<uint64_t> keys;
    fill_position_hint_map(&m1, &keys);

    // Iteration w/o insert/erasure should be same with single-threaded iteration.
    std::vector<uint64_t> keys_out;
    CountOnPause on_pause1;
    list_flat_map(&keys_out, m1, 10, on_pause1);
    EXPECT_EQ(m1.size() / 10, on_pause1.num_paused);
    ASSERT_EQ(m1.size(), keys_out.size());
    std::sort(keys_out.begin(), keys_out.end());
    for (size_t i = 0; i < keys_out.size(); ++i) {
        ASSERT_TRUE(m1.seek(keys_out[i])) << "i=" << i;
        if (i) {
            ASSERT_NE(keys_out[i-1], keys_out[i]) << "i=" << i;
        }
    }
}

struct RemoveInsertVisitedOnPause {
    RemoveInsertVisitedOnPause() : keys(NULL), map(NULL) {
        removed_keys.init(32);
        inserted_keys.init(32);
    }
    void operator()(const PositionHintMap::PositionHint& hint) {
        // Remove one
        do {
            int index = rand() % keys->size();
            uint64_t removed_key = (*keys)[index];
            if (removed_keys.seek(removed_key)) {
                continue;
            }
            ASSERT_EQ(1u, map->erase(removed_key));
            removed_keys.insert(removed_key);
            break;
        } while (true);
        
        // Insert one
        uint64_t inserted_key =
            ((rand() % hint.offset) + rand() * hint.nbucket);
        inserted_keys.insert(inserted_key);
        ++(*map)[inserted_key];
    }
    mutil::FlatSet<uint64_t> removed_keys;
    mutil::FlatSet<uint64_t> inserted_keys;
    const std::vector<uint64_t>* keys;
    PositionHintMap* map;
};

TEST_F(FlatMapTest, erase_insert_visited_during_iteration) {
    PositionHintMap m1;
    std::vector<uint64_t> keys;
    fill_position_hint_map(&m1, &keys);

    // Erase/insert visisted values should not affect the result.
    const size_t old_map_size = m1.size();
    RemoveInsertVisitedOnPause on_pause2;
    std::vector<uint64_t> keys_out;
    on_pause2.map = &m1;
    on_pause2.keys = &keys_out;
    list_flat_map(&keys_out, m1, 10, on_pause2);
    EXPECT_EQ(old_map_size / 10, on_pause2.removed_keys.size());
    ASSERT_EQ(old_map_size, keys_out.size());
    std::sort(keys_out.begin(), keys_out.end());
    for (size_t i = 0; i < keys_out.size(); ++i) {
        if (i) {
            ASSERT_NE(keys_out[i-1], keys_out[i]) << "i=" << i;
        }
        if (!m1.seek(keys_out[i])) {
            ASSERT_TRUE(on_pause2.removed_keys.seek(keys_out[i])) << "i=" << i;
        }
        ASSERT_FALSE(on_pause2.inserted_keys.seek(keys_out[i])) << "i=" << i;
    }
}

struct RemoveHintedOnPause {
    RemoveHintedOnPause() : map(NULL) {
        removed_keys.init(32);
    }
    void operator()(const PositionHintMap::PositionHint& hint) {
        uint64_t removed_key = hint.key;
        ASSERT_EQ(1u, map->erase(removed_key));
        removed_keys.insert(removed_key);
    }
    mutil::FlatSet<uint64_t> removed_keys;
    PositionHintMap* map;
};

TEST_F(FlatMapTest, erase_hinted_during_iteration) {
    PositionHintMap m1;
    std::vector<uint64_t> keys;
    fill_position_hint_map(&m1, &keys);

    // Erasing hinted values
    RemoveHintedOnPause on_pause3;
    std::vector<uint64_t> keys_out;
    on_pause3.map = &m1;
    list_flat_map(&keys_out, m1, 10, on_pause3);
    // Not equal sometimes because of backward of iterator
    // EXPECT_EQ(m1.size() / 10, on_pause3.removed_keys.size());
    const size_t old_keys_out_size = keys_out.size();
    std::sort(keys_out.begin(), keys_out.end());
    keys_out.resize(std::unique(keys_out.begin(), keys_out.end()) - keys_out.begin());
    LOG_IF(INFO, keys_out.size() != old_keys_out_size)
        << "Iterated " << old_keys_out_size - keys_out.size()
        << " duplicated elements";
    ASSERT_EQ(m1.size(), keys_out.size());
    for (size_t i = 0; i < keys_out.size(); ++i) {
        if (i) {
            ASSERT_NE(keys_out[i-1], keys_out[i]) << "i=" << i;
        }
        if (!m1.seek(keys_out[i])) {
            ASSERT_TRUE(on_pause3.removed_keys.seek(keys_out[i])) << "i=" << i;
        }
    }
}

struct RemoveInsertUnvisitedOnPause {
    RemoveInsertUnvisitedOnPause()
        : keys_out(NULL), all_keys(NULL), map(NULL) {
        removed_keys.init(32);
        inserted_keys.init(32);
    }
    void operator()(const PositionHintMap::PositionHint& hint) {
        // Insert one
        do {
            uint64_t inserted_key =
                ((rand() % (hint.nbucket - hint.offset)) + hint.offset
                 + rand() * hint.nbucket);
            if (std::find(all_keys->begin(), all_keys->end(), inserted_key)
                != all_keys->end()) {
                continue;
            }
            all_keys->push_back(inserted_key);
            inserted_keys.insert(inserted_key);
            ++(*map)[inserted_key];
            break;
        } while (true);

        // Remove one
        while (true) {
            int index = rand() % all_keys->size();
            uint64_t removed_key = (*all_keys)[index];
            if (removed_key == hint.key) {
                continue;
            }
            if (removed_keys.seek(removed_key)) {
                continue;
            }
            if (std::find(keys_out->begin(), keys_out->end(), removed_key)
                != keys_out->end()) {
                continue;
            }
            ASSERT_EQ(1u, map->erase(removed_key));
            removed_keys.insert(removed_key);
            break;
        }
    }
    mutil::FlatSet<uint64_t> removed_keys;
    mutil::FlatSet<uint64_t> inserted_keys;
    const std::vector<uint64_t>* keys_out;
    std::vector<uint64_t>* all_keys;
    PositionHintMap* map;
};

TEST_F(FlatMapTest, erase_insert_unvisited_during_iteration) {
    PositionHintMap m1;
    std::vector<uint64_t> keys;
    fill_position_hint_map(&m1, &keys);

    // Erase/insert unvisited values should affect keys_out
    RemoveInsertUnvisitedOnPause on_pause4;
    std::vector<uint64_t> keys_out;
    on_pause4.map = &m1;
    on_pause4.keys_out = &keys_out;
    on_pause4.all_keys = &keys;
    list_flat_map(&keys_out, m1, 10, on_pause4);
    EXPECT_EQ(m1.size() / 10, on_pause4.removed_keys.size());
    ASSERT_EQ(m1.size(), keys_out.size());
    std::sort(keys_out.begin(), keys_out.end());
    for (size_t i = 0; i < keys_out.size(); ++i) {
        if (i) {
            ASSERT_NE(keys_out[i-1], keys_out[i]) << "i=" << i;
        }
        ASSERT_TRUE(m1.seek(keys_out[i])) << "i=" << i;
    }
}

inline uint64_t fmix64 (uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;

    return k;
}

template <typename T>
struct PointerHasher {
    size_t operator()(const T* p) const
    { return fmix64(reinterpret_cast<uint64_t>(p)); }
};

template <typename T>
struct PointerHasher2 {
    size_t operator()(const T* p) const
    { return reinterpret_cast<uint64_t>(p); }
};

TEST_F(FlatMapTest, perf_cmp_with_map_storing_pointers) {
    const size_t REP = 4;
    int* ptr[2048];
    for (size_t i = 0; i < ARRAY_SIZE(ptr); ++i) {
        ptr[i] = new int;
    }

    std::set<int*> m1;
    mutil::FlatSet<int*, PointerHasher<int> > m2;
    mutil::hash_set<int*, PointerHasher<int> > m3;

    std::vector<int*> r;
    int sum;
    mutil::Timer tm;

    r.reserve(ARRAY_SIZE(ptr)*REP);
    ASSERT_EQ(0, m2.init(ARRAY_SIZE(ptr)));

    for (size_t i = 0; i < ARRAY_SIZE(ptr); ++i) {
        m1.insert(ptr[i]);
        m2.insert(ptr[i]);
        m3.insert(ptr[i]);
        for (size_t j = 0; j < REP; ++j) {
            r.push_back(ptr[i]);
        }
    }
    ASSERT_EQ(m1.size(), m2.size());
    ASSERT_EQ(m1.size(), m3.size());

    std::random_shuffle(r.begin(), r.end());

    sum = 0;
    tm.start();
    for (size_t i = 0; i < r.size(); ++i) {
        sum += (m2.seek(r[i]) != NULL);
    }
    tm.stop();
    LOG(INFO) << "FlatMap takes " << tm.n_elapsed()/r.size();

    sum = 0;
    tm.start();
    for (size_t i = 0; i < r.size(); ++i) {
        sum += (m1.find(r[i]) != m1.end());
    }
    tm.stop();
    LOG(INFO) << "std::set takes " << tm.n_elapsed()/r.size();

    sum = 0;
    tm.start();
    for (size_t i = 0; i < r.size(); ++i) {
        sum += (m3.find(r[i]) != m3.end());
    }
    tm.stop();
    LOG(INFO) << "std::set takes " << tm.n_elapsed()/r.size() << " sum=" << sum;

    for (size_t i = 0; i < ARRAY_SIZE(ptr); ++i) {
        delete ptr[i];
    }
}


int n_con = 0;
int n_cp_con = 0;
int n_des = 0;
int n_cp = 0;
struct Value {
    Value() : x_(0) { ++n_con; }
    Value(int x) : x_(x) { ++ n_con; }
    Value (const Value& rhs) : x_(rhs.x_) { ++ n_cp_con; }
    ~Value() { ++ n_des; }
    
    Value& operator= (const Value& rhs) {
        x_ = rhs.x_;
        ++ n_cp;
        return *this;
    }
    
    bool operator== (const Value& rhs) const { return x_ == rhs.x_; }
    bool operator!= (const Value& rhs) const { return x_ != rhs.x_; }

friend std::ostream& operator<< (std::ostream& os, const Value& v)
    { return os << v.x_; }

    int x_;
};

int n_con_key = 0;
int n_cp_con_key = 0;
int n_des_key = 0;
struct Key {
    Key() : x_(0) { ++n_con_key; }
    Key(int x) : x_(x) { ++ n_con_key; }
    Key(const Key& rhs) : x_(rhs.x_) { ++ n_cp_con_key; }
    void operator=(const Key& rhs) { x_ = rhs.x_; }
    ~Key() { ++ n_des_key; }
    int x_;
};
struct KeyHasher {
    size_t operator()(const Key& k) const { return k.x_; }
};
struct KeyEqualTo {
    size_t operator()(const Key& k1, const Key& k2) const
    { return k1.x_ == k2.x_; }
};

TEST_F(FlatMapTest, key_value_are_not_constructed_before_first_insertion) {
    mutil::FlatMap<Key, Value, KeyHasher, KeyEqualTo> m;
    ASSERT_EQ(0, m.init(32));
    ASSERT_EQ(0, n_con_key);
    ASSERT_EQ(0, n_cp_con_key);
    ASSERT_EQ(0, n_con);
    ASSERT_EQ(0, n_cp_con);
    const Key k1 = 1;
    ASSERT_EQ(1, n_con_key);
    ASSERT_EQ(0, n_cp_con_key);
    ASSERT_EQ(NULL, m.seek(k1));
    ASSERT_EQ(0u, m.erase(k1));
    ASSERT_EQ(1, n_con_key);
    ASSERT_EQ(0, n_cp_con_key);
    ASSERT_EQ(0, n_con);
    ASSERT_EQ(0, n_cp_con);
}

TEST_F(FlatMapTest, manipulate_uninitialized_map) {
    mutil::FlatMap<int, int> m;
    ASSERT_FALSE(m.initialized());
    for (mutil::FlatMap<int,int>::iterator it = m.begin(); it != m.end(); ++it) {
        LOG(INFO) << "nothing";
    }
    ASSERT_EQ(NULL, m.seek(1));
    ASSERT_EQ(0u, m.erase(1));
    ASSERT_EQ(0u, m.size());
    ASSERT_TRUE(m.empty());
    ASSERT_EQ(0u, m.bucket_count());
    ASSERT_EQ(0u, m.load_factor());
}

TEST_F(FlatMapTest, perf_small_string_map) {
    mutil::Timer tm1;
    mutil::Timer tm2;
    mutil::Timer tm3;
    mutil::Timer tm4;

    for (int i = 0; i < 10; ++i) {
        tm3.start();
        mutil::PooledMap<std::string, std::string> m3;
        m3["Content-type"] = "application/json";
        m3["Request-Id"] = "true";
        m3["Status-Code"] = "200";
        tm3.stop();

        tm4.start();
        mutil::CaseIgnoredFlatMap<std::string> m4;
        m4.init(16);
        m4["Content-type"] = "application/json";
        m4["Request-Id"] = "true";
        m4["Status-Code"] = "200";
        tm4.stop();

        tm1.start();
        mutil::FlatMap<std::string, std::string> m1;
        m1.init(16);
        m1["Content-type"] = "application/json";
        m1["Request-Id"] = "true";
        m1["Status-Code"] = "200";
        tm1.stop();

        tm2.start();
        std::map<std::string, std::string> m2;
        m2["Content-type"] = "application/json";
        m2["Request-Id"] = "true";
        m2["Status-Code"] = "200";
        tm2.stop();
    
        LOG(INFO) << "flatmap=" << tm1.n_elapsed()
                  << " ci_flatmap=" << tm4.n_elapsed()
                  << " map=" << tm2.n_elapsed()
                  << " pooled_map=" << tm3.n_elapsed();
    }
}


TEST_F(FlatMapTest, sanity) {
    typedef mutil::FlatMap<uint64_t, long> Map;
    Map m;

    ASSERT_FALSE(m.initialized());
    m.init(1000, 70);
    ASSERT_TRUE(m.initialized());
    ASSERT_EQ(0UL, m.size());
    ASSERT_TRUE(m.empty());
    ASSERT_EQ(0UL, m._pool.count_allocated());

    const uint64_t k1 = 1;
    // hashed to the same bucket of k1.
    const uint64_t k2 = k1 + m.bucket_count();

    const uint64_t k3 = k1 + 1;

    // Initial insertion
    m[k1] = 10;
    ASSERT_EQ(1UL, m.size());
    ASSERT_FALSE(m.empty());
    long* p = m.seek(k1);
    ASSERT_TRUE(p && *p == 10);
    ASSERT_EQ(0UL, m._pool.count_allocated());
    
    ASSERT_EQ(NULL, m.seek(k2));

    // Override
    m[k1] = 100;
    ASSERT_EQ(1UL, m.size());
    ASSERT_FALSE(m.empty());
    p = m.seek(k1);
    ASSERT_TRUE(p && *p == 100);
    
    // Insert another
    m[k3] = 20;
    ASSERT_EQ(2UL, m.size());
    ASSERT_FALSE(m.empty());
    p = m.seek(k3);
    ASSERT_TRUE(p && *p == 20);
    ASSERT_EQ(0UL, m._pool.count_allocated());

    m[k2] = 30;
    ASSERT_EQ(1UL, m._pool.count_allocated());
    ASSERT_EQ(0UL, m._pool.count_free());
    ASSERT_EQ(3UL, m.size());
    ASSERT_FALSE(m.empty());
    p = m.seek(k2);
    ASSERT_TRUE(p && *p == 30);
    
    ASSERT_EQ(NULL, m.seek(2049));

    Map::iterator it = m.begin();
    ASSERT_EQ(k1, it->first);
    ++it;
    ASSERT_EQ(k2, it->first);
    ++it;
    ASSERT_EQ(k3, it->first);
    ++it;
    ASSERT_EQ(m.end(), it);

    // Erase exist
    ASSERT_EQ(1UL, m.erase(k1));
    ASSERT_EQ(2UL, m.size());
    ASSERT_FALSE(m.empty());
    ASSERT_EQ(NULL, m.seek(k1));
    ASSERT_EQ(30, *m.seek(k2));
    ASSERT_EQ(20, *m.seek(k3));
    ASSERT_EQ(1UL, m._pool.count_allocated());
    ASSERT_EQ(1UL, m._pool.count_free());

    // default constructed
    ASSERT_EQ(0, m[k1]);
    ASSERT_EQ(0, m[5]);
    ASSERT_EQ(0, m[1029]);
    ASSERT_EQ(0, m[2053]);

    // Clear
    m.clear();
    ASSERT_EQ(m.size(), 0ul);
    ASSERT_TRUE(m.empty());
    ASSERT_EQ(NULL, m.seek(k1));
    ASSERT_EQ(NULL, m.seek(k2));
    ASSERT_EQ(NULL, m.seek(k3));
}

TEST_F(FlatMapTest, random_insert_erase) {
    srand (0);

    {
        mutil::hash_map<uint64_t, Value> ref[2];
        typedef mutil::FlatMap<uint64_t, Value> Map;
        Map ht[2];
        ht[0].init (40);
        ht[1] = ht[0];
    
        for (int j = 0; j < 30; ++j) {
            // Make snapshot
            ht[1] = ht[0];
            ref[1] = ref[0];

            for (int i = 0; i < 100000; ++i) {
                int k = rand() % 0xFFFF;
                int p = rand() % 1000;
                if (p < 600) {
                    ht[0].insert(k, i);
                    ref[0][k] = i;
                } else if(p < 999) {
                    ht[0].erase (k);
                    ref[0].erase (k);
                } else {
                    ht[0].clear();
                    ref[0].clear();
                }
            }
            
            LOG(INFO) << "Check j=" << j;
            // bi-check
            for (int i=0; i<2; ++i) {
                for (Map::iterator it = ht[i].begin(); it != ht[i].end(); ++it)
                {
                    mutil::hash_map<uint64_t, Value>::iterator it2 = ref[i].find(it->first);
                    ASSERT_TRUE (it2 != ref[i].end());
                    ASSERT_EQ (it2->second, it->second);
                }
        
                for (mutil::hash_map<uint64_t, Value>::iterator it = ref[i].begin();
                     it != ref[i].end(); ++it)
                {
                    Value* p_value = ht[i].seek(it->first);
                    ASSERT_TRUE (p_value != NULL);
                    ASSERT_EQ (it->second, p_value->x_);
                }
                ASSERT_EQ (ht[i].size(), ref[i].size());
            }
        }

    }
    // cout << "ht[0] = " << show(ht[0]) << endl
    //      << "ht[1] = " << show(ht[1]) << endl;

    //ASSERT_EQ (ht[0]._pool->alloc_num(), 0ul);
    ASSERT_EQ (n_con + n_cp_con, n_des);

    LOG(INFO) << "n_con:" << n_con << std::endl
              << "n_cp_con:" << n_cp_con << std::endl
              << "n_con+n_cp_con:" <<  n_con+n_cp_con <<  std::endl
              << "n_des:" << n_des << std::endl
              << "n_cp:" << n_cp;    
}

template <typename T> void perf_insert_erase(bool random, const T& value)
{
    size_t nkeys[] = { 100, 1000, 10000 };
    const size_t NPASS = ARRAY_SIZE(nkeys);

    std::vector<uint64_t> keys;
    mutil::FlatMap<uint64_t, T> id_map;
    std::map<uint64_t, T> std_map;
    mutil::PooledMap<uint64_t, T> pooled_map;
    mutil::hash_map<uint64_t, T> hash_map;
    mutil::Timer id_tm, std_tm, pooled_tm, hash_tm;
    
    size_t max_nkeys = 0;
    for (size_t i = 0; i < NPASS; ++i) {
        max_nkeys = std::max(max_nkeys, nkeys[i]);
    }
    id_map.init((size_t)(nkeys[NPASS-1] * 1.5));

    // Make DS hot
    for (size_t i = 0; i < max_nkeys; ++i) {
        id_map[i] = value;
        std_map[i] = value;
        pooled_map[i] = value;
        hash_map[i] = value;
    }
    id_map.clear();
    std_map.clear();
    pooled_map.clear();
    hash_map.clear();

    LOG(INFO) << "[ value = " << sizeof(T) << " bytes ]";
    for (size_t pass = 0; pass < NPASS; ++pass) {
        int start = rand();
        keys.clear();
        for (size_t i = 0; i < nkeys[pass]; ++i) {
            keys.push_back(start + i);
        }

        if (random) {
            random_shuffle(keys.begin(), keys.end());
        }
        
        id_map.clear();        
        id_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            id_map[keys[i]] = value;
        }
        id_tm.stop();

        std_map.clear();
        std_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            std_map[keys[i]] = value;
        }
        std_tm.stop();

        pooled_map.clear();
        pooled_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            pooled_map[keys[i]] = value;
        }
        pooled_tm.stop();

        hash_map.clear();
        hash_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            hash_map[keys[i]] = value;
        }
        hash_tm.stop();
        
        LOG(INFO) << (random ? "Randomly" : "Sequentially")
                  << " inserting " << keys.size()
                  << " into FlatMap/std::map/mutil::PooledMap/mutil::hash_map takes "
                  << id_tm.n_elapsed() / keys.size()
                  << "/" << std_tm.n_elapsed() / keys.size()
                  << "/" << pooled_tm.n_elapsed() / keys.size()
                  << "/" << hash_tm.n_elapsed() / keys.size();
        
        if (random) {
            random_shuffle(keys.begin(), keys.end());
        }
        
        id_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            id_map.erase(keys[i]);
        }
        id_tm.stop();

        std_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            std_map.erase(keys[i]);
        }
        std_tm.stop();

        pooled_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            pooled_map.erase(keys[i]);
        }
        pooled_tm.stop();

        hash_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            hash_map.erase(keys[i]);
        }
        hash_tm.stop();
        
        LOG(INFO) << (random ? "Randomly" : "Sequentially")
                  << " erasing " << keys.size()
                  << " from FlatMap/std::map/mutil::PooledMap/mutil::hash_map takes "
                  << id_tm.n_elapsed() / keys.size()
                  << "/" << std_tm.n_elapsed() / keys.size()
                  << "/" << pooled_tm.n_elapsed() / keys.size()
                  << "/" << hash_tm.n_elapsed() / keys.size();
    }
}

template <typename T> void perf_seek(const T& value) {
    size_t nkeys[] = { 100, 1000, 10000 };
    const size_t NPASS = ARRAY_SIZE(nkeys);
    std::vector<uint64_t> keys;
    std::vector<uint64_t> rkeys;
    mutil::FlatMap<uint64_t, T> id_map;
    std::map<uint64_t, T> std_map;
    mutil::PooledMap<uint64_t, T> pooled_map;
    mutil::hash_map<uint64_t, T> hash_map;
    
    mutil::Timer id_tm, std_tm, pooled_tm, hash_tm;
    
    id_map.init((size_t)(nkeys[NPASS-1] * 1.5));
    LOG(INFO) << "[ value = " << sizeof(T) << " bytes ]";
    for (size_t pass = 0; pass < NPASS; ++pass) {
        int start = rand();
        keys.clear();
        for (size_t i = 0; i < nkeys[pass]; ++i) {
            keys.push_back(start + i);
        }
        
        id_map.clear();        
        for (size_t i = 0; i < keys.size(); ++i) {
            id_map[keys[i]] = value;
        }

        std_map.clear();
        for (size_t i = 0; i < keys.size(); ++i) {
            std_map[keys[i]] = value;
        }
        
        pooled_map.clear();
        for (size_t i = 0; i < keys.size(); ++i) {
            pooled_map[keys[i]] = value;
        }

        hash_map.clear();
        for (size_t i = 0; i < keys.size(); ++i) {
            hash_map[keys[i]] = value;
        }
        
        random_shuffle(keys.begin(), keys.end());
        
        long sum = 0;
        id_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            sum += *(long*)id_map.seek(keys[i]);
        }
        id_tm.stop();

        std_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            sum += (long&)std_map.find(keys[i])->second;
        }
        std_tm.stop();

        pooled_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            sum += (long&)pooled_map.find(keys[i])->second;
        }
        pooled_tm.stop();

        hash_tm.start();
        for (size_t i = 0; i < keys.size(); ++i) {
            sum += (long&)hash_map.find(keys[i])->second;
        }
        hash_tm.stop();
        
        LOG(INFO) << "Seeking " << keys.size()
                  << " from FlatMap/std::map/mutil::PooledMap/mutil::hash_map takes "
                  << id_tm.n_elapsed() / keys.size()
                  << "/" << std_tm.n_elapsed() / keys.size()
                  << "/" << pooled_tm.n_elapsed() / keys.size()
                  << "/" << hash_tm.n_elapsed() / keys.size()
                  << " sum=" << sum;
    }
}

struct Dummy1 {
    long data[4];
};
struct Dummy2 {
    long data[16];
};

TEST_F(FlatMapTest, perf) {
    perf_insert_erase<long>(false, 100);
    perf_insert_erase<Dummy1>(false, Dummy1());
    perf_insert_erase<Dummy2>(false, Dummy2());
    perf_insert_erase<long>(true, 100);
    perf_insert_erase<Dummy1>(true, Dummy1());
    perf_insert_erase<Dummy2>(true, Dummy2());
    perf_seek<long>(100);
    perf_seek<Dummy1>(Dummy1());
    perf_seek<Dummy2>(Dummy2());
    perf_seek<long>(100);
    perf_seek<Dummy1>(Dummy1());
    perf_seek<Dummy2>(Dummy2());
}

TEST_F(FlatMapTest, copy) {
    mutil::FlatMap<int, int> m1;
    mutil::FlatMap<int, int> m2;
    ASSERT_EQ(0, m1.init(32));
    m1[1] = 1;
    m1[2] = 2;
    m2 = m1;
    ASSERT_FALSE(m1.is_too_crowded(m1.size()));
    ASSERT_FALSE(m2.is_too_crowded(m1.size()));
}

}
