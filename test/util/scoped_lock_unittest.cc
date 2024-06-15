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
#include <melon/base/scoped_lock.h>
#include <errno.h>

namespace {
class ScopedLockTest : public ::testing::Test{
protected:
    ScopedLockTest(){
    };
    virtual ~ScopedLockTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

TEST_F(ScopedLockTest, mutex) {    
    pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
    {
        MELON_SCOPED_LOCK(m1);
        ASSERT_EQ(EBUSY, pthread_mutex_trylock(&m1));
    }
    ASSERT_EQ(0, pthread_mutex_trylock(&m1));
    pthread_mutex_unlock(&m1);
}

TEST_F(ScopedLockTest, spinlock) {    
    pthread_spinlock_t s1;
    pthread_spin_init(&s1, 0);
    {
        MELON_SCOPED_LOCK(s1);
        ASSERT_EQ(EBUSY, pthread_spin_trylock(&s1));
    }
    ASSERT_EQ(0, pthread_spin_lock(&s1));
    pthread_spin_unlock(&s1);
    pthread_spin_destroy(&s1);
}

TEST_F(ScopedLockTest, unique_lock_mutex) {
    pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
    {
        std::unique_lock<pthread_mutex_t> lck(m1);
        ASSERT_EQ(EBUSY, pthread_mutex_trylock(&m1));
        lck.unlock();
        {
            std::unique_lock<pthread_mutex_t> lck2(m1, std::try_to_lock);
            ASSERT_TRUE(lck2.owns_lock());
        }
        ASSERT_TRUE(lck.try_lock());
        ASSERT_TRUE(lck.owns_lock());
        std::unique_lock<pthread_mutex_t> lck2(m1, std::defer_lock);
        ASSERT_FALSE(lck2.owns_lock());
        std::unique_lock<pthread_mutex_t> lck3(m1, std::try_to_lock);
        ASSERT_FALSE(lck3.owns_lock());
    }
    {
        MELON_SCOPED_LOCK(m1);
        ASSERT_EQ(EBUSY, pthread_mutex_trylock(&m1));
    }
    ASSERT_EQ(0, pthread_mutex_trylock(&m1));
    {
        std::unique_lock<pthread_mutex_t> lck(m1, std::adopt_lock);
        ASSERT_TRUE(lck.owns_lock());
    }
    std::unique_lock<pthread_mutex_t> lck(m1, std::try_to_lock);
    ASSERT_TRUE(lck.owns_lock());
}

TEST_F(ScopedLockTest, unique_lock_spin) {
    pthread_spinlock_t s1;
    pthread_spin_init(&s1, 0);
    {
        std::unique_lock<pthread_spinlock_t> lck(s1);
        ASSERT_EQ(EBUSY, pthread_spin_trylock(&s1));
        lck.unlock();
        ASSERT_TRUE(lck.try_lock());
    }
    {
        MELON_SCOPED_LOCK(s1);
        ASSERT_EQ(EBUSY, pthread_spin_trylock(&s1));
    }
    ASSERT_EQ(0, pthread_spin_lock(&s1));
    pthread_spin_unlock(&s1);
    pthread_spin_destroy(&s1);
}

}
