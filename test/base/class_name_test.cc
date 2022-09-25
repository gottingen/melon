
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "melon/base/class_name.h"
#include "melon/log/logging.h"

namespace melon::base {
    namespace foobar {
        struct MyClass {
        };
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
        ASSERT_EQ("add_something", melon::base::demangle("add_something"));
        ASSERT_EQ("dp::FiberPBCommand<proto::PbRouteTable, proto::PbRouteAck>::marshal(dp::ParamWriter*)::__FUNCTION__",
                  melon::base::demangle(
                          "_ZZN2dp14FiberPBCommandIN5proto12PbRouteTableENS1_10PbRouteAckEE7marshalEPNS_11ParamWriterEE12__FUNCTION__"));
        ASSERT_EQ("7&8", melon::base::demangle("7&8"));
    }

    TEST_F(ClassNameTest, class_name_sanity) {
        ASSERT_EQ("char", melon::base::class_name_str('\0'));
        ASSERT_STREQ("short", melon::base::class_name<short>());
        ASSERT_EQ("long", melon::base::class_name_str(1L));
        ASSERT_EQ("unsigned long", melon::base::class_name_str(1UL));
        ASSERT_EQ("float", melon::base::class_name_str(1.1f));
        ASSERT_EQ("double", melon::base::class_name_str(1.1));
        ASSERT_STREQ("char*", melon::base::class_name<char *>());
        ASSERT_STREQ("char const*", melon::base::class_name<const char *>());
        ASSERT_STREQ("melon::base::foobar::MyClass", melon::base::class_name<melon::base::foobar::MyClass>());

        int array[32];
        ASSERT_EQ("int [32]", melon::base::class_name_str(array));

        MELON_LOG(INFO) << melon::base::class_name_str(this);
        MELON_LOG(INFO) << melon::base::class_name_str(*this);
    }
}
