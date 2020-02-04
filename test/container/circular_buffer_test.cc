//
// Created by liyinbin on 2020/1/31.
//

#include <gtest/gtest.h>
#include <abel/container/circular_buffer.h>

TEST(circular, erase) {
    abel::circular_buffer<int> buf;

    buf.push_back(3);
    buf.erase(buf.begin(), buf.end());

    EXPECT_TRUE(buf.size() == 0);
    EXPECT_TRUE(buf.empty());

    buf.push_back(1);
    buf.push_back(2);
    buf.push_back(3);
    buf.push_back(4);
    buf.push_back(5);

    buf.erase(std::remove_if(buf.begin(), buf.end(), [] (int v) { return (v & 1) == 0; }), buf.end());

    EXPECT_TRUE(buf.size() == 3);
    EXPECT_TRUE(!buf.empty());
    {
        auto i = buf.begin();
        EXPECT_EQ(*i++, 1);
        EXPECT_EQ(*i++, 3);
        EXPECT_EQ(*i++, 5);
        EXPECT_TRUE(i == buf.end());
    }
}

TEST(circular, eraseMiddle) {
    abel::circular_buffer<int> buf;

    for (int i = 0; i < 10; ++i) {
        buf.push_back(i);
    }

    auto i = buf.erase(buf.begin() + 3, buf.begin() + 6);
    EXPECT_EQ(*i, 6);

    i = buf.begin();
    EXPECT_EQ(*i++, 0);
    EXPECT_EQ(*i++, 1);
    EXPECT_EQ(*i++, 2);
    EXPECT_EQ(*i++, 6);
    EXPECT_EQ(*i++, 7);
    EXPECT_EQ(*i++, 8);
    EXPECT_EQ(*i++, 9);
    EXPECT_TRUE(i == buf.end());
}

TEST(circular, iterator) {
    abel::circular_buffer<int> buf;

    buf.push_back(1);
    buf.push_back(2);
    buf.push_back(3);
    buf.push_back(4);
    buf.push_back(5);

    int *ptr_to_3 = &buf[2];
    auto iterator_to_3 = buf.begin() + 2;
    assert(*ptr_to_3 == 3);
    assert(*iterator_to_3 == 3);

    buf.erase(buf.begin(), buf.begin() + 2);

    EXPECT_TRUE(*ptr_to_3 == 3);
    EXPECT_TRUE(*iterator_to_3 == 3);

    buf.erase(buf.begin() + 1, buf.end());

    EXPECT_TRUE(*ptr_to_3 == 3);
    EXPECT_TRUE(*iterator_to_3 == 3);

    EXPECT_TRUE(buf.size() == 1);
}
