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
#include <errno.h>
#include <melon/utility/thread_local.h>

namespace {

MELON_THREAD_LOCAL int * dummy = NULL;
const size_t NTHREAD = 8;
static bool processed[NTHREAD+1];
static bool deleted[NTHREAD+1];
static bool register_check = false;

struct YellObj {
    static int nc;
    static int nd;
    YellObj() {
        ++nc;
    }
    ~YellObj() {
        ++nd;
    }
};
int YellObj::nc = 0;
int YellObj::nd = 0;

static void check_global_variable() {
    EXPECT_TRUE(processed[NTHREAD]);
    EXPECT_TRUE(deleted[NTHREAD]);
    EXPECT_EQ(2, YellObj::nc);
    EXPECT_EQ(2, YellObj::nd);
}

class BaiduThreadLocalTest : public ::testing::Test{
protected:
    BaiduThreadLocalTest(){
        if (!register_check) {
            register_check = true;
            mutil::thread_atexit(check_global_variable);
        }
    };
    virtual ~BaiduThreadLocalTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};


MELON_THREAD_LOCAL void* x;

void* foo(void* arg) {
    x = arg;
    usleep(10000);
    printf("x=%p\n", x);
    return NULL;
}

TEST_F(BaiduThreadLocalTest, thread_local_keyword) {
    pthread_t th[2];
    pthread_create(&th[0], NULL, foo, (void*)1);
    pthread_create(&th[1], NULL, foo, (void*)2);
    pthread_join(th[0], NULL);
    pthread_join(th[1], NULL);
}

void* yell(void*) {
    YellObj* p = mutil::get_thread_local<YellObj>();
    EXPECT_TRUE(p);
    EXPECT_EQ(2, YellObj::nc);
    EXPECT_EQ(0, YellObj::nd);
    EXPECT_EQ(p, mutil::get_thread_local<YellObj>());
    EXPECT_EQ(2, YellObj::nc);
    EXPECT_EQ(0, YellObj::nd);
    return NULL;
}

TEST_F(BaiduThreadLocalTest, get_thread_local) {
    YellObj::nc = 0;
    YellObj::nd = 0;
    YellObj* p = mutil::get_thread_local<YellObj>();
    ASSERT_TRUE(p);
    ASSERT_EQ(1, YellObj::nc);
    ASSERT_EQ(0, YellObj::nd);
    ASSERT_EQ(p, mutil::get_thread_local<YellObj>());
    ASSERT_EQ(1, YellObj::nc);
    ASSERT_EQ(0, YellObj::nd);
    pthread_t th;
    ASSERT_EQ(0, pthread_create(&th, NULL, yell, NULL));
    pthread_join(th, NULL);
    EXPECT_EQ(2, YellObj::nc);
    EXPECT_EQ(1, YellObj::nd);
}

void delete_dummy(void* arg) {
    *(bool*)arg = true;
    if (dummy) {
        delete dummy;
        dummy = NULL;
    } else {
        printf("dummy is NULL\n");
    }
}

void* proc_dummy(void* arg) {
    bool *p = (bool*)arg;
    *p = true;
    EXPECT_TRUE(dummy == NULL);
    dummy = new int(p - processed);
    mutil::thread_atexit(delete_dummy, deleted + (p - processed));
    return NULL;
}

TEST_F(BaiduThreadLocalTest, sanity) {
    errno = 0;
    ASSERT_EQ(-1, mutil::thread_atexit(NULL));
    ASSERT_EQ(EINVAL, errno);

    processed[NTHREAD] = false;
    deleted[NTHREAD] = false;
    proc_dummy(&processed[NTHREAD]);
    
    pthread_t th[NTHREAD];
    for (size_t i = 0; i < NTHREAD; ++i) {
        processed[i] = false;
        deleted[i] = false;
        ASSERT_EQ(0, pthread_create(&th[i], NULL, proc_dummy, processed + i));
    }
    for (size_t i = 0; i < NTHREAD; ++i) {
        ASSERT_EQ(0, pthread_join(th[i], NULL));
        ASSERT_TRUE(processed[i]);
        ASSERT_TRUE(deleted[i]);
    }
}

static std::ostringstream* oss = NULL;
inline std::ostringstream& get_oss() {
    if (oss == NULL) {
        oss = new std::ostringstream;
    }
    return *oss;
}

void fun1() {
    get_oss() << "fun1" << std::endl;
}

void fun2() {
    get_oss() << "fun2" << std::endl;
}

void fun3(void* arg) {
    get_oss() << "fun3(" << (uintptr_t)arg << ")" << std::endl;
}

void fun4(void* arg) {
    get_oss() << "fun4(" << (uintptr_t)arg << ")" << std::endl;
}

static void check_result() {
  // Don't use gtest function since this function might be invoked when the main
  // thread quits, instances required by gtest functions are likely destroyed.
  assert(get_oss().str() == "fun4(0)\nfun3(2)\nfun2\n");
}

TEST_F(BaiduThreadLocalTest, call_order_and_cancel) {
    mutil::thread_atexit_cancel(NULL);
    mutil::thread_atexit_cancel(NULL, NULL);

    ASSERT_EQ(0, mutil::thread_atexit(check_result));

    ASSERT_EQ(0, mutil::thread_atexit(fun1));
    ASSERT_EQ(0, mutil::thread_atexit(fun1));
    ASSERT_EQ(0, mutil::thread_atexit(fun2));
    ASSERT_EQ(0, mutil::thread_atexit(fun3, (void*)1));
    ASSERT_EQ(0, mutil::thread_atexit(fun3, (void*)1));
    ASSERT_EQ(0, mutil::thread_atexit(fun3, (void*)2));
    ASSERT_EQ(0, mutil::thread_atexit(fun4, NULL));

    mutil::thread_atexit_cancel(NULL);
    mutil::thread_atexit_cancel(NULL, NULL);
    mutil::thread_atexit_cancel(fun1);
    mutil::thread_atexit_cancel(fun3, NULL);
    mutil::thread_atexit_cancel(fun3, (void*)1);
}

} // namespace
