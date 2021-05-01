//
// Created by liyinbin on 2021/4/19.
//


#include "abel/io/iobuf_view.h"

#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "abel/base/random.h"

using namespace std::literals;

namespace abel {

    iobuf MakeAToZBuffer() {
        iobuf_builder nbb;

        nbb.append(make_foreign_slice("abc"));
        nbb.append(make_foreign_slice("d"));
        nbb.append("efgh");
        nbb.append("ijk");
        nbb.append('l');
        nbb.append('m');
        nbb.append(create_buffer_slow("nopqrstuvwxyz"));
        return nbb.destructive_get();
    }

    std::string RandomString() {
        std::string str;

        for (int i = 0; i != 100; ++i) {
            str += std::to_string(i);
        }
        return str;
    }

    TEST(ForwardView, Basic) {
        auto buffer = MakeAToZBuffer();
        iobuf_forward_view view(buffer);
        EXPECT_EQ(view.size(), buffer.byte_size());
        EXPECT_FALSE(view.empty());

        char expected = 'a';
        auto iter = view.begin();
        while (iter != view.end()) {
            EXPECT_EQ(expected, *iter);
            ++expected;
            ++iter;
        }
        EXPECT_EQ('z' + 1, expected);
    }

    TEST(ForwardView, Search) {
        auto buffer = create_buffer_slow(std::string(10485760, 'a'));
        iobuf_forward_view view(buffer);
        constexpr auto kFound = "aaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
                kNotFound = "aaaaaaaaaaaaaaaaaaaaab"sv;
        EXPECT_EQ(view.begin(), std::search(view.begin(), view.end(), kFound.begin(),
                                            kFound.end()));
        EXPECT_EQ(view.end(), std::search(view.begin(), view.end(), kNotFound.begin(),
                                          kNotFound.end()));
    }

    TEST(RandomView, Basic) {
        auto buffer = MakeAToZBuffer();
        iobuf_view view(buffer);
        EXPECT_EQ(view.size(), buffer.byte_size());
        EXPECT_FALSE(view.empty());

        char expected = 'a';
        auto iter = view.begin();
        while (iter != view.end()) {
            EXPECT_EQ(expected, *iter);
            ++expected;
            ++iter;
        }
        EXPECT_EQ('z' + 1, expected);
        for (int i = 'a'; i <= 'z'; ++i) {
            auto iter = view.begin();
            iter = iter + (i - 'a');
            EXPECT_EQ(i, *iter);
            EXPECT_EQ(i - 'a', iter - view.begin());
        }
        iter = view.begin();
        iter += 'z' - 'a' + 1;  // to `end()`.
        EXPECT_EQ(iter, view.end());
    }

    TEST(RandomView, Search0) {
        auto buffer = create_buffer_slow("");
        iobuf_view view(buffer);
        constexpr auto kKey = "aaaaaaaaaaaaaaaaaaaaaaaaaaa"sv;
        auto result = std::search(view.begin(), view.end(), kKey.begin(), kKey.end());
        EXPECT_EQ(view.begin(), result);
    }

    TEST(RandomView, Search1) {
        auto buffer = create_buffer_slow(std::string(10485760, 'a'));
        iobuf_view view(buffer);
        constexpr auto kFound = "aaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
                kNotFound = "aaaaaaaaaaaaaaaaaaaaab"sv;
        auto result1 =
                std::search(view.begin(), view.end(), kFound.begin(), kFound.end());
        EXPECT_EQ(view.begin(), result1);
        EXPECT_EQ(0, result1 - view.begin());
        auto result2 =
                std::search(view.begin(), view.end(), kNotFound.begin(), kNotFound.end());
        EXPECT_EQ(view.end(), result2);
        EXPECT_EQ(view.size(), result2 - view.begin());
    }

    TEST(RandomView, Search2) {
        auto buffer = MakeAToZBuffer();
        iobuf_view view(buffer);
        constexpr auto kFound = "hijklmn"sv;
        auto result =
                std::search(view.begin(), view.end(), kFound.begin(), kFound.end());
        EXPECT_EQ(7, result - view.begin());
    }

    TEST(RandomView, RandomSearch) {
        for (int i = 0; i != 100000; ++i) {
            auto value = RandomString();
            auto temp = abel::format("asdfdsf{}XXXADFFDAF", value);
            auto start = 0;

            iobuf_builder builder;
            while (start != temp.size()) {
                auto size = abel::Random<int>(1, temp.size() - start);
                builder.append(temp.substr(start, size));
                start += size;
            }
            auto buffer = builder.destructive_get();

            iobuf_view view(buffer);
            auto result =
                    std::search(view.begin(), view.end(), value.begin(), value.end());
            if (result - view.begin() != 7) {
                abort();
            }
            ASSERT_EQ(7, result - view.begin());
        }
    }

}  // namespace abel
