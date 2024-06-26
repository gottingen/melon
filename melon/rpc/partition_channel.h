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



#ifndef MELON_RPC_PARTITION_CHANNEL_H_
#define MELON_RPC_PARTITION_CHANNEL_H_



#include <melon/rpc/parallel_channel.h>
#include <melon/rpc/selective_channel.h> // For DynamicPartitionChannel


namespace melon {

class NamingServiceThread;
class PartitionChannelBase;

// Representing a partition kind.
struct Partition {
    // Index of the partition kind, counting from 0.
    int index;

    // Number of partition kinds, a partition kind may have more than one
    // instances.
    int num_partition_kinds;
};

// Parse partition from a string tag which is often associated with servers
// in NamingServices.
class PartitionParser {
public:
    virtual ~PartitionParser() {}

    // Implement this method to get partition `out' from string `tag'.
    // Returns true on success.
    virtual bool ParseFromTag(const std::string& tag, Partition* out) = 0;
};

// For customizing PartitionChannel.
struct PartitionChannelOptions : public ChannelOptions {
    // Constructed with default values.
    PartitionChannelOptions();

    // Make RPC call stop soon (without waiting for the timeout) when failed
    // sub calls reached this number.
    // Default: number of sub channels, which means the RPC to ParallChannel
    // will not be canceled until all sub calls failed.
    int fail_limit;

    // Check comments on ParallelChannel.AddChannel in parallel_channel.h
    // Sub channels in PartitionChannel share the same mapper and merger.
    mutil::intrusive_ptr<CallMapper> call_mapper;
    mutil::intrusive_ptr<ResponseMerger> response_merger;
};

// PartitionChannel is a specialized ParallelChannel whose sub channels are
// built from a NamingService which specifies partitioning information in
// tags. This channel eases access to partitioned servers.
class PartitionChannel : public ChannelBase {
public:
    PartitionChannel();
    ~PartitionChannel();

    // Initialize this PartitionChannel with `num_partition_kinds' sub channels
    // sending requests to different partitions listed in `naming_service_url'.
    // `partition_parser' parses partition from tags associated with servers.
    // When this method succeeds, `partition_parser' is owned by this channel,
    // otherwise `partition_parser' is unmodified and can be used for other
    // usages.
    // For example:
    //   num_partition_kinds = 3
    //   partition_parser = parse N/M as Partition{index=N, num_partition_kinds=M}
    //   naming_service = s1(tag=1/3) s2(tag=2/3) s3(tag=0/3) s4(tag=1/4) s5(tag=2/3)
    //   load_balancer = rr
    // 3 sub channels(c0,c1,c2) will be created:
    //   - c0 sends requests to s3 because the tag=0/3 means s3 is the first
    //     partition kind in 3 kinds.
    //   - c1 sends requests to s1 because the tag=1/3 means s1 is the second
    //     partition kind in 3 kinds. s4(tag=1/4) is ignored because number of
    //     partition kinds does not match.
    //   - c2 sends requests to s2 and s5 because the tags=2/3 means they're
    //     both the third partition kind in 3 kinds. s2 and s5 will be load-
    //     balanced with "rr" algorithm.
    //                               /   c0 -> s3      (rr)
    //   request -> PartitionChannel --  c1 -> s1      (rr)
    //                               \   c2 -> s2, s5  (rr)
    int Init(int num_partition_kinds,
             PartitionParser* partition_parser,
             const char* naming_service_url, 
             const char* load_balancer_name,
             const PartitionChannelOptions* options);

    // Access sub channels corresponding to partitions in parallel.
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);

    int partition_count() const;

private:
    bool initialized() const { return _parser != NULL; }

    int CheckHealth();

    PartitionChannelBase* _pchan;
    mutil::intrusive_ptr<NamingServiceThread> _nsthread_ptr;
    PartitionParser* _parser;
};

// As the name implies, this combo channel discovers differently partitioned
// servers and builds sub PartitionChannels on-the-fly for different groups 
// of servers. When multiple partitioning methods co-exist, traffic is 
// splitted based on capacities, namely # of servers in groups. The main 
// purpose of this channel is to transit from one partitioning method to 
// another smoothly. For example, with proper deployment, servers can be 
// changed from M-partitions to N-partitions losslessly without changing the
// client code.
class DynamicPartitionChannel : public ChannelBase {
public:
    DynamicPartitionChannel();
    ~DynamicPartitionChannel();

    // Unlike PartitionChannel, DynamicPartitionChannel does not need 
    // `num_partition_kinds'. It discovers and groups differently partitioned 
    // servers automatically.
    int Init(PartitionParser* partition_parser,
             const char* naming_service_url, 
             const char* load_balancer_name,
             const PartitionChannelOptions* options);
    
    // Access partitions according to their capacities.
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);

private:
    bool initialized() const { return _parser != NULL; }

    int CheckHealth() {
        return static_cast<ChannelBase*>(&_schan)->CheckHealth();
    }

    class Partitioner;

    SelectiveChannel _schan;
    Partitioner* _partitioner;
    mutil::intrusive_ptr<NamingServiceThread> _nsthread_ptr;
    PartitionParser* _parser;
};

} // namespace melon


#endif  // MELON_RPC_PARTITION_CHANNEL_H_
