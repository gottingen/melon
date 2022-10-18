
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include <pthread.h>                                // pthread_*

#include <cstddef>
#include <memory>
#include <iostream>
#include <sstream>
#include "melon/times/time.h"
#include "melon/metrics/all.h"

namespace {
    class StatusTest : public testing::Test {
    protected:

        void SetUp() {
        }

        void TearDown() {
            ASSERT_EQ(0UL, melon::variable_base::count_exposed());
        }
    };

    TEST_F(StatusTest, status) {
        melon::read_most_gauge<std::string> st1;
        st1.set_value("hello {}", 9);
        ASSERT_EQ(0, st1.expose("var1", ""));
        ASSERT_EQ("hello 9", melon::variable_base::describe_exposed("var1"));
        ASSERT_EQ("\"hello 9\"", melon::variable_base::describe_exposed("var1", true));
        std::vector<std::string> vars;
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ(1UL, melon::variable_base::count_exposed());

        melon::read_most_gauge<std::string> st2;
        st2.set_value("world {}", 10);
        ASSERT_EQ(-1, st2.expose("var1", ""));
        ASSERT_EQ(1UL, melon::variable_base::count_exposed());
        ASSERT_EQ("world 10", st2.get_description());
        ASSERT_EQ("hello 9", melon::variable_base::describe_exposed("var1"));
        ASSERT_EQ(1UL, melon::variable_base::count_exposed());

        ASSERT_TRUE(st1.hide());
        ASSERT_EQ(0UL, melon::variable_base::count_exposed());
        ASSERT_EQ("", melon::variable_base::describe_exposed("var1"));
        ASSERT_EQ(0, st1.expose("var1", ""));
        ASSERT_EQ(1UL, melon::variable_base::count_exposed());
        ASSERT_EQ("hello 9",
                  melon::variable_base::describe_exposed("var1"));

        ASSERT_EQ(0, st2.expose("var2", ""));
        ASSERT_EQ(2UL, melon::variable_base::count_exposed());
        ASSERT_EQ("hello 9", melon::variable_base::describe_exposed("var1"));
        ASSERT_EQ("world 10", melon::variable_base::describe_exposed("var2"));
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(2UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var2", vars[1]);

        ASSERT_TRUE(st2.hide());
        ASSERT_EQ(1UL, melon::variable_base::count_exposed());
        ASSERT_EQ("", melon::variable_base::describe_exposed("var2"));
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var1", vars[0]);

        st2.expose("var2 again", "");
        ASSERT_EQ("world 10", melon::variable_base::describe_exposed("var2_again"));
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(2UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var2_again", vars[1]);
        ASSERT_EQ(2UL, melon::variable_base::count_exposed());

        melon::read_most_gauge<std::string> st3("var3", "foobar");
        ASSERT_EQ("var3", st3.name());
        ASSERT_EQ(3UL, melon::variable_base::count_exposed());
        ASSERT_EQ("foobar", melon::variable_base::describe_exposed("var3"));
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(3UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var3", vars[1]);
        ASSERT_EQ("var2_again", vars[2]);
        ASSERT_EQ(3UL, melon::variable_base::count_exposed());

        melon::read_most_gauge<int> st4("var4", 9);
        ASSERT_EQ("var4", st4.name());
        ASSERT_EQ(4UL, melon::variable_base::count_exposed());
        ASSERT_EQ("9", melon::variable_base::describe_exposed("var4"));
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(4UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var3", vars[1]);
        ASSERT_EQ("var4", vars[2]);
        ASSERT_EQ("var2_again", vars[3]);

        melon::read_most_gauge<void *> st5((void *) 19UL);
        MELON_LOG(INFO) << st5;
        ASSERT_EQ("0x13", st5.get_description());
    }

    void print1(std::ostream &os, void *arg) {
        os << arg;
    }

    int64_t print2(void *arg) {
        return *(int64_t *) arg;
    }

    TEST_F(StatusTest, passive_status) {
        melon::basic_status_gauge<std::string> st1("var11", print1, (void *) 9UL);
        MELON_LOG(INFO) << st1;
        std::ostringstream ss;
        ASSERT_EQ(0, melon::variable_base::describe_exposed("var11", ss));
        ASSERT_EQ("0x9", ss.str());
        std::vector<std::string> vars;
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var11", vars[0]);
        ASSERT_EQ(1UL, melon::variable_base::count_exposed());

        int64_t tmp2 = 9;
        melon::basic_status_gauge<int64_t> st2("var12", print2, &tmp2);
        ss.str("");
        ASSERT_EQ(0, melon::variable_base::describe_exposed("var12", ss));
        ASSERT_EQ("9", ss.str());
        melon::variable_base::list_exposed(&vars);
        ASSERT_EQ(2UL, vars.size());
        ASSERT_EQ("var11", vars[0]);
        ASSERT_EQ("var12", vars[1]);
        ASSERT_EQ(2UL, melon::variable_base::count_exposed());
    }

    struct Foo {
        int x;

        Foo() : x(0) {}

        explicit Foo(int x2) : x(x2) {}

        Foo operator+(const Foo &rhs) const {
            return Foo(x + rhs.x);
        }
    };

    std::ostream &operator<<(std::ostream &os, const Foo &f) {
        return os << "Foo{" << f.x << "}";
    }

    TEST_F(StatusTest, non_primitive) {
        melon::read_most_gauge<Foo> st;
        ASSERT_EQ(0, st.get_value().x);
        st.set_value(Foo(1));
        ASSERT_EQ(1, st.get_value().x);
    }
} // namespace
