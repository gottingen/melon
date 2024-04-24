// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <gtest/gtest.h>
#include "melon/utility/class_name.h"
#include "melon/utility/logging.h"

namespace mutil {
namespace foobar {
struct MyClass {};
}
}

namespace {
class ClassNameTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        srand(time(0));
    };
};

TEST_F(ClassNameTest, demangle) {
    ASSERT_EQ("add_something", mutil::demangle("add_something"));
    ASSERT_EQ("guard variable for mutil::my_ip()::ip",
              mutil::demangle("_ZGVZN5mutil5my_ipEvE2ip"));
    ASSERT_EQ("dp::FiberPBCommand<proto::PbRouteTable, proto::PbRouteAck>::marshal(dp::ParamWriter*)::__FUNCTION__",
              mutil::demangle("_ZZN2dp14FiberPBCommandIN5proto12PbRouteTableENS1_10PbRouteAckEE7marshalEPNS_11ParamWriterEE12__FUNCTION__"));
    ASSERT_EQ("7&8", mutil::demangle("7&8"));
}

TEST_F(ClassNameTest, class_name_sanity) {
    ASSERT_EQ("char", mutil::class_name_str('\0'));
    ASSERT_STREQ("short", mutil::class_name<short>());
    ASSERT_EQ("long", mutil::class_name_str(1L));
    ASSERT_EQ("unsigned long", mutil::class_name_str(1UL));
    ASSERT_EQ("float", mutil::class_name_str(1.1f));
    ASSERT_EQ("double", mutil::class_name_str(1.1));
    ASSERT_STREQ("char*", mutil::class_name<char*>());
    ASSERT_STREQ("char const*", mutil::class_name<const char*>());
    ASSERT_STREQ("mutil::foobar::MyClass", mutil::class_name<mutil::foobar::MyClass>());

    int array[32];
    ASSERT_EQ("int [32]", mutil::class_name_str(array));

    LOG(INFO) << mutil::class_name_str(this);
    LOG(INFO) << mutil::class_name_str(*this);
}
}
