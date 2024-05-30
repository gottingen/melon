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
#include "melon/utility/synchronous_event.h"

namespace {
class SynchronousEventTest : public ::testing::Test{
protected:
    SynchronousEventTest(){
    };
    virtual ~SynchronousEventTest(){};
    virtual void SetUp() {
        srand(time(0));
    };
    virtual void TearDown() {
    };
};

struct Foo {};

typedef mutil::SynchronousEvent<int, int*> FooEvent;

FooEvent foo_event;
std::vector<std::pair<int, int> > result;

class FooObserver : public FooEvent::Observer {
public:
    FooObserver() : another_ob(NULL) {}
    
    void on_event(int x, int* p) {
        ++*p;
        result.push_back(std::make_pair(x, *p));
        if (another_ob) {
            foo_event.subscribe(another_ob);
        }
    }
    FooObserver* another_ob;
};


TEST_F(SynchronousEventTest, sanity) {
    const size_t N = 10;
    FooObserver foo_observer;
    FooObserver foo_observer2;
    foo_observer.another_ob = &foo_observer2;
    foo_event.subscribe(&foo_observer);
    int v = 0;
    result.clear();
    for (size_t i = 0; i < N; ++i) {
        foo_event.notify(i, &v);
    }
    ASSERT_EQ(2*N, result.size());
    for (size_t i = 0; i < 2*N; ++i) {
        ASSERT_EQ((int)i/2, result[i].first);
        ASSERT_EQ((int)i+1, result[i].second);
    }
}

}
