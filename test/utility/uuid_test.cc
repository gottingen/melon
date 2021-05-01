//
// Created by liyinbin on 2021/5/1.
//

#include "abel/utility/uuid.h"
#include "gtest/gtest.h"

namespace abel {

    TEST(uuid, Compare) {
        // Constexpr-friendly.
        static constexpr uuid kUuid1("123e4567-e89b-12d3-a456-426614174000");
        static constexpr uuid kUuid2;

        EXPECT_EQ("123e4567-e89b-12d3-a456-426614174000",
                  uuid("123e4567-e89b-12d3-a456-426614174000").to_string());
        EXPECT_EQ("123e4567-e89b-12d3-a456-426614174000", kUuid1.to_string());
        EXPECT_EQ("123e4567-e89b-12d3-a456-426614174000",
                  uuid("123E4567-E89B-12D3-a456-426614174000").to_string());
        EXPECT_EQ("00000000-0000-0000-0000-000000000000", uuid().to_string());
        EXPECT_EQ("00000000-0000-0000-0000-000000000000", kUuid2.to_string());
    }

    TEST(uuid, parse_uuid) {
        auto parsed = parse_uuid("123e4567-e89b-12d3-a456-426614174000");
        ASSERT_TRUE(parsed);
        EXPECT_EQ("123e4567-e89b-12d3-a456-426614174000", parsed->to_string());
        EXPECT_FALSE(parse_uuid("123e4567-e89b-12d3-a456-42661417400"));
        EXPECT_FALSE(parse_uuid("123e4567-e89b-12d3-a456-4266141740000"));
        EXPECT_FALSE(parse_uuid("123e4567-e89b-12d3-a456=426614174000"));
        EXPECT_FALSE(parse_uuid("123e4567-e89b-12d3-a456-42661417400G"));
    }

}  // namespace abel
