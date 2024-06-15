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
#include <memory>

namespace {

class UniquePtrTest : public testing::Test {
protected:
    virtual void SetUp() {
    };
};

struct Foo {
    Foo() : destroyed(false), called_func(false) {}
    void destroy() { destroyed = true; }
    void func() { called_func = true; }
    bool destroyed;
    bool called_func;
};

struct FooDeleter {
    void operator()(Foo* f) const {
        f->destroy();
    }
};

TEST_F(UniquePtrTest, basic) {
    Foo foo;
    ASSERT_FALSE(foo.destroyed);
    ASSERT_FALSE(foo.called_func);
    {
        std::unique_ptr<Foo, FooDeleter> foo_ptr(&foo);
        foo_ptr->func();
        ASSERT_TRUE(foo.called_func);
    }
    ASSERT_TRUE(foo.destroyed);
}

static std::unique_ptr<Foo> generate_foo(Foo* foo) {
    std::unique_ptr<Foo> foo_ptr(foo);
    return foo_ptr;
}

TEST_F(UniquePtrTest, return_unique_ptr) {
    Foo* foo = new Foo;
    std::unique_ptr<Foo> foo_ptr = generate_foo(foo);
    ASSERT_EQ(foo, foo_ptr.get());
}

}
