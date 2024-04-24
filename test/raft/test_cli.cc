// Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved



#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <melon/utility/unique_ptr.h>
#include <melon/rpc/server.h>
#include "melon/raft/raft.h"
#include "melon/raft/cli.h"
#include "melon/raft/node.h"

class CliTest : public testing::Test {
public:
    void SetUp() {
        google::SetCommandLineOption("raft_sync", "false");
        ::system("rm -rf data");
    }
    void TearDown() {
        ::system("rm -rf data");
    }
};

class MockFSM : public melon::raft::StateMachine {
public:
    virtual void on_apply(melon::raft::Iterator& /*iter*/) {
        ASSERT_FALSE(true) << "Can't reach here";
    }
};

class RaftNode {
public:
    RaftNode() : _node(NULL) {}
    ~RaftNode() {
        stop();
        delete _node;
    }
    int start(int port, bool is_leader) {
        if (melon::raft::add_service(&_server, port) != 0) {
            return -1;
        }
        if (_server.Start(port, NULL) != 0) {
            return -1;
        }
        melon::raft::NodeOptions options;
        std::string prefix;
        mutil::string_printf(&prefix, "local://./data/%d", port);
        options.log_uri = prefix + "/log";
        options.raft_meta_uri = prefix + "/raft_meta";
        options.snapshot_uri = prefix + "/snapshot";
        options.fsm = &_fsm;
        options.node_owns_fsm = false;
        options.disable_cli = false;
        mutil::ip_t my_ip;
        EXPECT_EQ(0, mutil::str2ip("127.0.0.1", &my_ip));
        mutil::EndPoint addr(my_ip, port);
        melon::raft::PeerId my_id(addr, 0);
        if (is_leader) {
            options.initial_conf.add_peer(my_id);
        }
        _node = new melon::raft::Node("test", my_id);
        return _node->init(options);
    }
    void stop() {
        if (_node) {
            _node->shutdown(NULL);
            _node->join();
        }
        _server.Stop(0);
        _server.Join();
    }
    melon::raft::PeerId peer_id() const { return _node->node_id().peer_id; }
protected:
    melon::Server _server;
    melon::raft::Node* _node;
    MockFSM _fsm;
};

TEST_F(CliTest, add_and_remove_peer) {
    RaftNode node1;
    ASSERT_EQ(0, node1.start(9500, true));
    // Add a non-exists peer should return ECATCHUP
    mutil::Status st;
    melon::raft::Configuration old_conf;
    melon::raft::PeerId peer1 = node1.peer_id();
    old_conf.add_peer(peer1);
    melon::raft::PeerId peer2("127.0.0.1:9501");
    st = melon::raft::cli::add_peer("test", old_conf, peer2,
                             melon::raft::cli::CliOptions());
    ASSERT_FALSE(st.ok());
    LOG(INFO) << "st=" << st;
    RaftNode node2;
    ASSERT_EQ(0, node2.start(peer2.addr.port, false));
    st = melon::raft::cli::add_peer("test", old_conf, peer2,
                             melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok()) << st;
    st = melon::raft::cli::add_peer("test", old_conf, peer2,
                             melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok()) << st;
    melon::raft::PeerId peer3("127.0.0.1:9502");
    RaftNode node3;
    ASSERT_EQ(0, node3.start(peer3.addr.port, false));
    old_conf.add_peer(peer2);
    st = melon::raft::cli::add_peer("test", old_conf, peer3,
                             melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok()) << st;
    old_conf.add_peer(peer3);
    st = melon::raft::cli::remove_peer("test", old_conf, peer1,
                                melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok()) << st;
    usleep(1000 * 1000);
    // Retried remove_peer
    st = melon::raft::cli::remove_peer("test", old_conf, peer1,
                                melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok()) << st;
}

TEST_F(CliTest, set_peer) {
    RaftNode node1;
    ASSERT_EQ(0, node1.start(9500, false));
    melon::raft::Configuration conf1;
    for (int i = 0; i < 3; ++i) {
        melon::raft::PeerId peer_id = node1.peer_id();
        peer_id.addr.port += i;
        conf1.add_peer(peer_id);
    }
    mutil::Status st;
    st = melon::raft::cli::reset_peer("test", node1.peer_id(), conf1,
                                melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok());
    melon::raft::Configuration conf2;
    conf2.add_peer(node1.peer_id());
    st = melon::raft::cli::reset_peer("test", node1.peer_id(), conf2,
                             melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok());
    usleep(4 * 1000 * 1000);
    ASSERT_TRUE(node1._node->is_leader());
}

TEST_F(CliTest, change_peers) {
    RaftNode node1;
    ASSERT_EQ(0, node1.start(9500, false));
    melon::raft::Configuration conf1;
    for (int i = 0; i < 3; ++i) {
        melon::raft::PeerId peer_id = node1.peer_id();
        peer_id.addr.port += i;
        conf1.add_peer(peer_id);
    }
    mutil::Status st;
    st = melon::raft::cli::reset_peer("test", node1.peer_id(), conf1,
                                melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok());
    melon::raft::Configuration conf2;
    conf2.add_peer(node1.peer_id());
    st = melon::raft::cli::reset_peer("test", node1.peer_id(), conf2,
                             melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok());
    usleep(4 * 1000 * 1000);
    ASSERT_TRUE(node1._node->is_leader());
}

TEST_F(CliTest, change_peer) {
    size_t N = 10;
    std::unique_ptr<RaftNode[]> nodes(new RaftNode[N]);
    nodes[0].start(9500, true);
    for (size_t i = 1; i < N; ++i) {
        ASSERT_EQ(0, nodes[i].start(9500 + i, false));
    }
    melon::raft::Configuration conf;
    for (size_t i = 0; i < N; ++i) {
        conf.add_peer("127.0.0.1:" + std::to_string(9500 + i));
    }
    mutil::Status st;
    for (size_t i = 0; i < N; ++i) {
        usleep(1000 * 1000);
        melon::raft::Configuration new_conf;
        new_conf.add_peer("127.0.0.1:" + std::to_string(9500 + i));
        st = melon::raft::cli::change_peers("test", conf, new_conf, melon::raft::cli::CliOptions());
        ASSERT_TRUE(st.ok()) << st;
    }
    usleep(1000 * 1000);
    st = melon::raft::cli::change_peers("test", conf, conf, melon::raft::cli::CliOptions());
    ASSERT_TRUE(st.ok()) << st;
    for (size_t i = 0; i < N; ++i) {
        usleep(10 * 1000);
        melon::raft::Configuration new_conf;
        new_conf.add_peer("127.0.0.1:" + std::to_string(9500 + i));
        LOG(WARNING) << "change " << conf << " to " << new_conf;
        st = melon::raft::cli::change_peers("test", conf, new_conf, melon::raft::cli::CliOptions());
        ASSERT_TRUE(st.ok()) << st;
        usleep(1000 * 1000);
        LOG(WARNING) << "change " << new_conf << " to " << conf;
        st = melon::raft::cli::change_peers("test", new_conf, conf, melon::raft::cli::CliOptions());
        ASSERT_TRUE(st.ok()) << st << " i=" << i;
    }
}

