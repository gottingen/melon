// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/at_exit.h"
#include "melon/utility/atomic_sequence_num.h"
#include "melon/utility/lazy_instance.h"
#include "melon/utility/memory/aligned_memory.h"
#include "melon/utility/threading/simple_thread.h"
#include <gtest/gtest.h>

namespace {

mutil::StaticAtomicSequenceNumber constructed_seq_;
mutil::StaticAtomicSequenceNumber destructed_seq_;

class ConstructAndDestructLogger {
 public:
  ConstructAndDestructLogger() {
    constructed_seq_.GetNext();
  }
  ~ConstructAndDestructLogger() {
    destructed_seq_.GetNext();
  }
};

class SlowConstructor {
 public:
  SlowConstructor() : some_int_(0) {
    // Sleep for 1 second to try to cause a race.
    mutil::PlatformThread::Sleep(mutil::TimeDelta::FromSeconds(1));
    ++constructed;
    some_int_ = 12;
  }
  int some_int() const { return some_int_; }

  static int constructed;
 private:
  int some_int_;
};

int SlowConstructor::constructed = 0;

class SlowDelegate : public mutil::DelegateSimpleThread::Delegate {
 public:
  explicit SlowDelegate(mutil::LazyInstance<SlowConstructor>* lazy)
      : lazy_(lazy) {}

  virtual void Run() OVERRIDE {
    EXPECT_EQ(12, lazy_->Get().some_int());
    EXPECT_EQ(12, lazy_->Pointer()->some_int());
  }

 private:
  mutil::LazyInstance<SlowConstructor>* lazy_;
};

}  // namespace

static mutil::LazyInstance<ConstructAndDestructLogger> lazy_logger =
    LAZY_INSTANCE_INITIALIZER;

TEST(LazyInstanceTest, Basic) {
  {
    mutil::ShadowingAtExitManager shadow;

    EXPECT_EQ(0, constructed_seq_.GetNext());
    EXPECT_EQ(0, destructed_seq_.GetNext());

    lazy_logger.Get();
    EXPECT_EQ(2, constructed_seq_.GetNext());
    EXPECT_EQ(1, destructed_seq_.GetNext());

    lazy_logger.Pointer();
    EXPECT_EQ(3, constructed_seq_.GetNext());
    EXPECT_EQ(2, destructed_seq_.GetNext());
  }
  EXPECT_EQ(4, constructed_seq_.GetNext());
  EXPECT_EQ(4, destructed_seq_.GetNext());
}

static mutil::LazyInstance<SlowConstructor> lazy_slow =
    LAZY_INSTANCE_INITIALIZER;

TEST(LazyInstanceTest, ConstructorThreadSafety) {
  {
    mutil::ShadowingAtExitManager shadow;

    SlowDelegate delegate(&lazy_slow);
    EXPECT_EQ(0, SlowConstructor::constructed);

    mutil::DelegateSimpleThreadPool pool("lazy_instance_cons", 5);
    pool.AddWork(&delegate, 20);
    EXPECT_EQ(0, SlowConstructor::constructed);

    pool.Start();
    pool.JoinAll();
    EXPECT_EQ(1, SlowConstructor::constructed);
  }
}

namespace {

// DeleteLogger is an object which sets a flag when it's destroyed.
// It accepts a bool* and sets the bool to true when the dtor runs.
class DeleteLogger {
 public:
  DeleteLogger() : deleted_(NULL) {}
  ~DeleteLogger() { *deleted_ = true; }

  void SetDeletedPtr(bool* deleted) {
    deleted_ = deleted;
  }

 private:
  bool* deleted_;
};

}  // anonymous namespace

TEST(LazyInstanceTest, LeakyLazyInstance) {
  // Check that using a plain LazyInstance causes the dtor to run
  // when the AtExitManager finishes.
  bool deleted1 = false;
  {
    mutil::ShadowingAtExitManager shadow;
    static mutil::LazyInstance<DeleteLogger> test = LAZY_INSTANCE_INITIALIZER;
    test.Get().SetDeletedPtr(&deleted1);
  }
  EXPECT_TRUE(deleted1);

  // Check that using a *leaky* LazyInstance makes the dtor not run
  // when the AtExitManager finishes.
  bool deleted2 = false;
  {
    mutil::ShadowingAtExitManager shadow;
    static mutil::LazyInstance<DeleteLogger>::Leaky
        test = LAZY_INSTANCE_INITIALIZER;
    test.Get().SetDeletedPtr(&deleted2);
  }
  EXPECT_FALSE(deleted2);
}

namespace {

template <size_t alignment>
class AlignedData {
 public:
  AlignedData() {}
  ~AlignedData() {}
  mutil::AlignedMemory<alignment, alignment> data_;
};

}  // anonymous namespace

#define EXPECT_ALIGNED(ptr, align) \
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(ptr) & (align - 1))

TEST(LazyInstanceTest, Alignment) {
  using mutil::LazyInstance;

  // Create some static instances with increasing sizes and alignment
  // requirements. By ordering this way, the linker will need to do some work to
  // ensure proper alignment of the static data.
  static LazyInstance<AlignedData<4> > align4 = LAZY_INSTANCE_INITIALIZER;
  static LazyInstance<AlignedData<32> > align32 = LAZY_INSTANCE_INITIALIZER;
  static LazyInstance<AlignedData<4096> > align4096 = LAZY_INSTANCE_INITIALIZER;

  EXPECT_ALIGNED(align4.Pointer(), 4);
  EXPECT_ALIGNED(align32.Pointer(), 32);
  EXPECT_ALIGNED(align4096.Pointer(), 4096);
}
