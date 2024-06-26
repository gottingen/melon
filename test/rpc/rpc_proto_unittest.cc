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


// Date: 2015/03/31 14:50:20


#include <gtest/gtest.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <melon/proto/rpc/melon_rpc_meta.pb.h>
#include "echo.pb.h"

namespace {
using namespace google::protobuf;
using namespace melon;

void BuildDependency(const FileDescriptor *file_desc, DescriptorPool *pool) {
    for (int i = 0; i < file_desc->dependency_count(); ++i) {
        const FileDescriptor *fd = file_desc->dependency(i);
        BuildDependency(fd, pool);
        FileDescriptorProto proto;
        fd->CopyTo(&proto);
        ASSERT_TRUE(pool->BuildFile(proto) != NULL);
    }
    FileDescriptorProto proto;
    file_desc->CopyTo(&proto);
    ASSERT_TRUE(pool->BuildFile(proto) != NULL);
}

TEST(ProtoTest, proto) {
    policy::RpcMeta meta;
    const Descriptor *desc = meta.GetDescriptor();
    const FileDescriptor *file_desc = desc->file();
    DescriptorPool pool;
    DynamicMessageFactory factory(&pool);
    BuildDependency(file_desc, &pool);
    FileDescriptorProto file_desc_proto;
    file_desc->CopyTo(&file_desc_proto);
    const FileDescriptor *new_file_desc = pool.BuildFile(file_desc_proto);
    ASSERT_TRUE(new_file_desc != NULL);
    const Descriptor *new_desc = new_file_desc->FindMessageTypeByName(desc->name());
    ASSERT_TRUE(new_desc != NULL);
    meta.set_correlation_id(123);
    std::string data;
    ASSERT_TRUE(meta.SerializeToString(&data));
    Message *msg = factory.GetPrototype(new_desc)->New();
    ASSERT_TRUE(msg != NULL);
    ASSERT_TRUE(msg->ParseFromString(data));
    ASSERT_TRUE(msg->SerializeToString(&data));
    policy::RpcMeta new_meta;
    ASSERT_TRUE(new_meta.ParseFromString(data));
    ASSERT_EQ(123, new_meta.correlation_id());
}

TEST(ProtoTest, required_enum) {
    test::Message1 msg1;
    msg1.set_stat(test::STATE0_NUM_1);
    std::string buf;
    ASSERT_TRUE(msg1.SerializeToString(&buf));
    test::Message2 msg2;
    ASSERT_TRUE(msg2.ParseFromString(buf));
    ASSERT_EQ((int)msg1.stat(), (int)msg2.stat());
    msg1.set_stat(test::STATE0_NUM_2);
    ASSERT_TRUE(msg1.SerializeToString(&buf));
    ASSERT_FALSE(msg2.ParseFromString(buf));
}
} //namespace
