//
// Created by liyinbin on 2020/2/2.
//


#include <gtest/gtest.h>
#include <deque>
#include <random>
#include <abel/container/fixed_circular_buffer.h>
#include <abel/algorithm/container.h>

using namespace abel;

using cb16_t = fixed_circular_buffer<int, 16>;


TEST(fixed_circular_buffer, test_edge_cases) {
    cb16_t cb;
    EXPECT_TRUE(cb.begin() == cb.end());
    cb.push_front(3);  // underflows indexes
    EXPECT_EQ(cb[0], 3);
    EXPECT_TRUE(cb.begin() < cb.end());
    cb.push_back(4);
    EXPECT_EQ(cb.size(), 2u);
    EXPECT_EQ(cb[0], 3);
    EXPECT_EQ(cb[1], 4);
    cb.pop_back();
    EXPECT_EQ(cb.back(), 3);
    cb.push_front(1);
    cb.pop_back();
    EXPECT_EQ(cb.back(), 1);
}

using deque = std::deque<int>;

TEST(fixed_circular_buffer, test_random_walk) {
    auto rand = std::default_random_engine();
    auto op_gen = std::uniform_int_distribution<unsigned>(0, 6);
    deque d;
    cb16_t c;
    for (auto i = 0; i != 1000000; ++i) {
        auto op = op_gen(rand);
        switch (op) {
            case 0:
                if (d.size() < 16) {
                    auto n = rand();
                    c.push_back(n);
                    d.push_back(n);
                }
                break;
            case 1:
                if (d.size() < 16) {
                    auto n = rand();
                    c.push_front(n);
                    d.push_front(n);
                }
                break;
            case 2:
                if (!d.empty()) {
                    auto n = d.back();
                    auto m = c.back();
                    EXPECT_EQ(n, m);
                    c.pop_back();
                    d.pop_back();
                }
                break;
            case 3:
                if (!d.empty()) {
                    auto n = d.front();
                    auto m = c.front();
                    EXPECT_EQ(n, m);
                    c.pop_front();
                    d.pop_front();
                }
                break;
            case 4:
                c_sort(c);
                c_sort(d);
                break;
            case 5:
                if (!d.empty()) {
                    auto u = std::uniform_int_distribution<size_t>(0, d.size() - 1);
                    auto idx = u(rand);
                    auto m = c[idx];
                    auto n = c[idx];
                    EXPECT_EQ(m, n);
                }
                break;
            case 6:
                c.clear();
                d.clear();
                break;
            case 7:
                c_reverse(c);
                c_reverse(d);
            default:
                abort();
        }
        EXPECT_EQ(c.size(), d.size());
        EXPECT_TRUE(c_equal(c, d));
    }
}
