
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <vector>

#include "testing/gtest_wrap.h"

#include "melon/container/flat_hash_map_dump.h"
#include "melon/container/flat_hash_set.h"
#include "melon/container/parallel_flat_hash_map.h"

namespace melon {
    namespace priv {
        namespace {

            TEST(DumpLoad, FlatHashSet_uin32) {
                melon::flat_hash_set<uint32_t> st1 = {1991, 1202};

                {
                    melon::binary_output_archive ar_out("./dump.data");
                    EXPECT_TRUE(st1.melon_map_dump(ar_out));
                }

                melon::flat_hash_set<uint32_t> st2;
                {
                    melon::binary_input_archive ar_in("./dump.data");
                    EXPECT_TRUE(st2.melon_map_load(ar_in));
                }
                EXPECT_TRUE(st1 == st2);
            }

            TEST(DumpLoad, FlatHashMap_uint64_uint32) {
                melon::flat_hash_map<uint64_t, uint32_t> mp1 = {
                        {78731, 99},
                        {13141, 299},
                        {2651,  101}};

                {
                    melon::binary_output_archive ar_out("./dump.data");
                    EXPECT_TRUE(mp1.melon_map_dump(ar_out));
                }

                melon::flat_hash_map<uint64_t, uint32_t> mp2;
                {
                    melon::binary_input_archive ar_in("./dump.data");
                    EXPECT_TRUE(mp2.melon_map_load(ar_in));
                }

                EXPECT_TRUE(mp1 == mp2);
            }

            TEST(DumpLoad, ParallelFlatHashMap_uint64_uint32) {
                melon::parallel_flat_hash_map<uint64_t, uint32_t> mp1 = {
                        {99,  299},
                        {992, 2991},
                        {299, 1299}};

                {
                    melon::binary_output_archive ar_out("./dump.data");
                    EXPECT_TRUE(mp1.melon_map_dump(ar_out));
                }

                melon::parallel_flat_hash_map<uint64_t, uint32_t> mp2;
                {
                    melon::binary_input_archive ar_in("./dump.data");
                    EXPECT_TRUE(mp2.melon_map_load(ar_in));
                }
                EXPECT_TRUE(mp1 == mp2);
            }

        }
    }
}

