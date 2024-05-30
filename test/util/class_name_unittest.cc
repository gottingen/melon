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
#include <melon/utility/class_name.h>
#include <turbo/log/logging.h>

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
