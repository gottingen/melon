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


// Date 2014/10/13 19:47:59

#include <pthread.h>                                // pthread_*

#include <cstddef>
#include <memory>
#include <iostream>
#include <sstream>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/var/var.h>
#include <gtest/gtest.h>

namespace {
class StatusTest : public testing::Test {
protected:
    void SetUp() {
    }
    void TearDown() {
        ASSERT_EQ(0UL, melon::var::Variable::count_exposed());
    }
};

TEST_F(StatusTest, status) {
    melon::var::Status<std::string> st1;
    st1.set_value("hello %d", 9);
    ASSERT_EQ(0, st1.expose("var1"));
    ASSERT_EQ("hello 9", melon::var::Variable::describe_exposed("var1"));
    ASSERT_EQ("\"hello 9\"", melon::var::Variable::describe_exposed("var1", true));
    std::vector<std::string> vars;
    melon::var::Variable::list_exposed(&vars);
        for (auto v : vars) {
            MLOG(ERROR) << v;
        }
    ASSERT_EQ(1UL, vars.size());
    ASSERT_EQ("var1", vars[0]);
    ASSERT_EQ(1UL, melon::var::Variable::count_exposed());


    melon::var::Status<std::string> st2;
    st2.set_value("world %d", 10);
    ASSERT_EQ(-1, st2.expose("var1"));
    ASSERT_EQ(1UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("world 10", st2.get_description());
    ASSERT_EQ("hello 9", melon::var::Variable::describe_exposed("var1"));
    ASSERT_EQ(1UL, melon::var::Variable::count_exposed());

    ASSERT_TRUE(st1.hide());
    ASSERT_EQ(0UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("", melon::var::Variable::describe_exposed("var1"));
    ASSERT_EQ(0, st1.expose("var1"));
    ASSERT_EQ(1UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("hello 9",
              melon::var::Variable::describe_exposed("var1"));
    
    ASSERT_EQ(0, st2.expose("var2"));
    ASSERT_EQ(2UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("hello 9", melon::var::Variable::describe_exposed("var1"));
    ASSERT_EQ("world 10", melon::var::Variable::describe_exposed("var2"));
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(2UL, vars.size());
    ASSERT_EQ("var1", vars[0]);
    ASSERT_EQ("var2", vars[1]);

    ASSERT_TRUE(st2.hide());
    ASSERT_EQ(1UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("", melon::var::Variable::describe_exposed("var2"));
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(1UL, vars.size());
    ASSERT_EQ("var1", vars[0]);

    st2.expose("var2 again");
    ASSERT_EQ("world 10", melon::var::Variable::describe_exposed("var2_again"));
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(2UL, vars.size());
    ASSERT_EQ("var1", vars[0]);
    ASSERT_EQ("var2_again", vars[1]);
    ASSERT_EQ(2UL, melon::var::Variable::count_exposed());

    melon::var::Status<std::string> st3("var3", "foobar");
    ASSERT_EQ("var3", st3.name());
    ASSERT_EQ(3UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("foobar", melon::var::Variable::describe_exposed("var3"));
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(3UL, vars.size());
    ASSERT_EQ("var1", vars[0]);
    ASSERT_EQ("var3", vars[1]);
    ASSERT_EQ("var2_again", vars[2]);
    ASSERT_EQ(3UL, melon::var::Variable::count_exposed());

    melon::var::Status<int> st4("var4", 9);
    ASSERT_EQ("var4", st4.name());
    ASSERT_EQ(4UL, melon::var::Variable::count_exposed());
    ASSERT_EQ("9", melon::var::Variable::describe_exposed("var4"));
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(4UL, vars.size());
    ASSERT_EQ("var1", vars[0]);
    ASSERT_EQ("var3", vars[1]);
    ASSERT_EQ("var4", vars[2]);
    ASSERT_EQ("var2_again", vars[3]);

    melon::var::Status<void*> st5((void*)19UL);
    MLOG(INFO) << st5;
    ASSERT_EQ("0x13", st5.get_description());
}

void print1(std::ostream& os, void* arg) {
    os << arg;
}

int64_t print2(void* arg) {
    return *(int64_t*)arg;
}

TEST_F(StatusTest, passive_status) {
    melon::var::BasicPassiveStatus<std::string> st1("var11", print1, (void*)9UL);
    MLOG(INFO) << st1;
    std::ostringstream ss;
    ASSERT_EQ(0, melon::var::Variable::describe_exposed("var11", ss));
    ASSERT_EQ("0x9", ss.str());
    std::vector<std::string> vars;
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(1UL, vars.size());
    ASSERT_EQ("var11", vars[0]);
    ASSERT_EQ(1UL, melon::var::Variable::count_exposed());

    int64_t tmp2 = 9;
    melon::var::BasicPassiveStatus<int64_t> st2("var12", print2, &tmp2);
    ss.str("");
    ASSERT_EQ(0, melon::var::Variable::describe_exposed("var12", ss));
    ASSERT_EQ("9", ss.str());
    melon::var::Variable::list_exposed(&vars);
    ASSERT_EQ(2UL, vars.size());
    ASSERT_EQ("var11", vars[0]);
    ASSERT_EQ("var12", vars[1]);
    ASSERT_EQ(2UL, melon::var::Variable::count_exposed());
}

struct Foo {
    int x;
    Foo() : x(0) {}
    explicit Foo(int x2) : x(x2) {}
    Foo operator+(const Foo& rhs) const {
        return Foo(x + rhs.x);
    }
};

std::ostream& operator<<(std::ostream& os, const Foo& f) {
    return os << "Foo{" << f.x << "}";
}

TEST_F(StatusTest, non_primitive) {
    melon::var::Status<Foo> st;
    ASSERT_EQ(0, st.get_value().x);
    st.set_value(Foo(1));
    ASSERT_EQ(1, st.get_value().x);
}
} // namespace
