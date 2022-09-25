
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <string>
#include "melon/container/flat_hash_map.h"
#include "melon/container/parallel_node_hash_map.h"
#include "melon/container/parallel_flat_hash_map.h"
#include "testing/gtest_wrap.h"

TEST(CaseIgnoredFlatHashMap, all) {
    melon::case_ignored_flat_hash_map<std::string, std::string> map;
    map["abc"] = "melon";
    EXPECT_EQ(map["Abc"], "melon");
    EXPECT_EQ(map["ABc"], "melon");
    EXPECT_EQ(map["AbC"], "melon");
    EXPECT_EQ(map["ABC"], "melon");
    EXPECT_EQ(map["abc"], "melon");
}