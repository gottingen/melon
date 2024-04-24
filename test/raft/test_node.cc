// libraft - Quorum-based replication of states across machines.
// Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <melon/utility/logging.h>
#include <melon/utility/files/file_path.h>
#include <melon/utility/file_util.h>
#include <melon/utility/fast_rand.h>
#include <melon/rpc/closure_guard.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/countdown_event.h>
#include "util.h"

namespace melon::raft {
extern melon::var::Adder<int64_t> g_num_nodes;
DECLARE_int32(raft_max_parallel_append_entries_rpc_num);
DECLARE_bool(raft_enable_append_entries_cache);
DECLARE_int32(raft_max_append_entries_cache_size);
DECLARE_bool(raft_enable_witness_to_leader);

}

using melon::raft::raft_mutex_t;
class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() {
    }
    void TearDown() {
    }
};

class NodeTest : public testing::TestWithParam<const char*> {
protected:
    void SetUp() {
        g_dont_print_apply_log = false;
        //logging::FLAGS_v = 90;
        // google::SetCommandLineOption("minloglevel", "1");
        google::SetCommandLineOption("crash_on_fatal_log", "true");
        if (GetParam() == std::string("NoReplication")) {
            melon::raft::FLAGS_raft_max_parallel_append_entries_rpc_num = 1;
            melon::raft::FLAGS_raft_enable_append_entries_cache = false;
        } else if (GetParam() == std::string("NoCache")) {
            melon::raft::FLAGS_raft_max_parallel_append_entries_rpc_num = 32;
            melon::raft::FLAGS_raft_enable_append_entries_cache = false;
        } else if (GetParam() == std::string("HasCache")) {
            melon::raft::FLAGS_raft_max_parallel_append_entries_rpc_num = 32;
            melon::raft::FLAGS_raft_enable_append_entries_cache = true;
            melon::raft::FLAGS_raft_max_append_entries_cache_size = 8;
        }
        LOG(INFO) << "Start unitests: " << GetParam();
        ::system("rm -rf data");
        ASSERT_EQ(0, melon::raft::g_num_nodes.get_value());
    }
    void TearDown() {
        ::system("rm -rf data");
        // Sleep for a while to wait all timer has stopped
        if (melon::raft::g_num_nodes.get_value() != 0) {
            usleep(1000 * 1000);
            ASSERT_EQ(0, melon::raft::g_num_nodes.get_value());
        }
    }
private:
    mutil::ShadowingAtExitManager exit_manager_;
};

TEST_P(NodeTest, InitShutdown) {
    melon::Server server;
    int ret = melon::raft::add_service(&server, "0.0.0.0:5006");
    ASSERT_EQ(0, ret);
    ASSERT_EQ(0, server.Start("0.0.0.0:5006", NULL));

    melon::raft::NodeOptions options;
    options.fsm = new MockFSM(mutil::EndPoint());
    options.log_uri = "local://./data/log";
    options.raft_meta_uri = "local://./data/raft_meta";
    options.snapshot_uri = "local://./data/snapshot";

    melon::raft::Node node("unittest", melon::raft::PeerId(mutil::EndPoint(mutil::my_ip(), 5006), 0));
    ASSERT_EQ(0, node.init(options));

    node.shutdown(NULL);
    node.join();

    //FIXME:
    fiber::CountdownEvent cond;
    mutil::IOBuf data;
    data.append("hello");
    melon::raft::Task task;
    task.data = &data;
    task.done = NEW_APPLYCLOSURE(&cond);
    node.apply(task);
    cond.wait();
}

TEST_P(NodeTest, Server) {
    melon::Server server1;
    melon::Server server2;
    ASSERT_EQ(0, melon::raft::add_service(&server1, "0.0.0.0:5006"));
    ASSERT_EQ(0, melon::raft::add_service(&server1, "0.0.0.0:5006"));
    ASSERT_EQ(0, melon::raft::add_service(&server2, "0.0.0.0:5007"));
    server1.Start("0.0.0.0:5006", NULL);
    server2.Start("0.0.0.0:5007", NULL);
}

TEST_P(NodeTest, SingleNode) {
    melon::Server server;
    int ret = melon::raft::add_service(&server, 5006);
    server.Start(5006, NULL);
    ASSERT_EQ(0, ret);

    melon::raft::PeerId peer;
    peer.addr.ip = mutil::my_ip();
    peer.addr.port = 5006;
    peer.idx = 0;
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(peer);

    melon::raft::NodeOptions options;
    options.election_timeout_ms = 300;
    options.initial_conf = melon::raft::Configuration(peers);
    options.fsm = new MockFSM(mutil::EndPoint());
    options.log_uri = "local://./data/log";
    options.raft_meta_uri = "local://./data/raft_meta";
    options.snapshot_uri = "local://./data/snapshot";

    melon::raft::Node node("unittest", peer);
    ASSERT_EQ(0, node.init(options));

    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        node.apply(task);
    }
    cond.wait();

    cond.reset(1);
    node.shutdown(NEW_SHUTDOWNCLOSURE(&cond, 0));
    cond.wait();

    server.Stop(200);
    server.Join();
}

TEST_P(NodeTest, NoLeader) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    cluster.start(peers[1].addr);

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(1, nodes.size());

    melon::raft::Node* follower = nodes[0];

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, EPERM);
        follower->apply(task);
    }
    cond.wait();

    // add peer1
    melon::raft::PeerId peer3;
    peer3.addr.ip = mutil::my_ip();
    peer3.addr.port = 5006 + 3;
    peer3.idx = 0;

    cond.reset(1);
    follower->add_peer(peer3, NEW_ADDPEERCLOSURE(&cond, EPERM));
    cond.wait();
    LOG(INFO) << "add peer " << peer3;

    // remove peer1
    melon::raft::PeerId peer0;
    peer0.addr.ip = mutil::my_ip();
    peer0.addr.port = 5006 + 0;
    peer0.idx = 0;

    cond.reset(1);
    follower->remove_peer(peer0, NEW_REMOVEPEERCLOSURE(&cond, EPERM));
    cond.wait();
    LOG(INFO) << "remove peer " << peer0;
}

TEST_P(NodeTest, TripleNode) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "no closure");
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        leader->apply(task);
    }

    cluster.ensure_same();

    {
        melon::Channel channel;
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_HTTP;

        if (channel.Init(leader->node_id().peer_id.addr, &options) != 0) {
            LOG(ERROR) << "Fail to initialize channel";
        }

        {
            melon::Controller cntl;
            cntl.http_request().uri() = "/raft_stat";
            cntl.http_request().set_method(melon::HTTP_METHOD_GET);

            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL/*done*/);

            LOG(INFO) << "http return: \n" << cntl.response_attachment();
        }

        {
            melon::Controller cntl;
            cntl.http_request().uri() = "/raft_stat/unittest";
            cntl.http_request().set_method(melon::HTTP_METHOD_GET);

            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL/*done*/);

            LOG(INFO) << "http return: \n" << cntl.response_attachment();
        }
    }

    // stop cluster
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, LeaderFail) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // stop leader
    mutil::EndPoint old_leader = leader->node_id().peer_id.addr;
    LOG(WARNING) << "stop leader " << leader->node_id();
    cluster.stop(leader->node_id().peer_id.addr);

    // apply something when follower
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    cond.reset(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "follower apply: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, -1);
        nodes[0]->apply(task);
    }
    cond.wait();

    // elect new leader
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "elect new leader " << leader->node_id();

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // old leader restart
    ASSERT_EQ(0, cluster.start(old_leader));
    LOG(WARNING) << "restart old leader " << old_leader;

    // apply something
    cond.reset(10);
    for (int i = 20; i < 30; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // stop and clean old leader
    LOG(WARNING) << "stop old leader " << old_leader;
    cluster.stop(old_leader);
    LOG(WARNING) << "clean old leader data " << old_leader;
    cluster.clean(old_leader);

    sleep(2);
    // restart old leader
    ASSERT_EQ(0, cluster.start(old_leader));
    LOG(WARNING) << "restart old leader " << old_leader;

    cluster.ensure_same();

    cluster.stop_all();
}

TEST_P(NodeTest, LeaderFailWithWitness) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        if (i == 0) {
            peer.role = melon::raft::Role::WITNESS;
        }
        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 30, nullptr, peers[i].is_witness()));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // stop leader
    mutil::EndPoint old_leader = leader->node_id().peer_id.addr;
    LOG(WARNING) << "stop leader " << leader->node_id();
    cluster.stop(leader->node_id().peer_id.addr);

    // apply something when follower
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    cond.reset(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "follower apply: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, -1);
        // node 0 is witness;
        nodes[1]->apply(task);
    }
    cond.wait();

    // elect new leader
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "elect new leader " << leader->node_id();

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // old leader restart
    ASSERT_EQ(0, cluster.start(old_leader));
    LOG(WARNING) << "restart old leader " << old_leader;

    // apply something
    cond.reset(10);
    for (int i = 20; i < 30; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // stop and clean old leader
    LOG(WARNING) << "stop old leader " << old_leader;
    cluster.stop(old_leader);
    LOG(WARNING) << "clean old leader data " << old_leader;
    cluster.clean(old_leader);

    sleep(2);
    // restart old leader
    ASSERT_EQ(0, cluster.start(old_leader));
    LOG(WARNING) << "restart old leader " << old_leader;

    cluster.ensure_same();

    cluster.stop_all();
}

TEST_P(NodeTest, JoinNode) {
    std::vector<melon::raft::PeerId> peers;
    melon::raft::PeerId peer0;
    peer0.addr.ip = mutil::my_ip();
    peer0.addr.port = 5006;
    peer0.idx = 0;

    // start cluster
    peers.push_back(peer0);
    Cluster cluster("unittest", peers);
    ASSERT_EQ(0, cluster.start(peer0.addr));
    LOG(INFO) << "start single cluster " << peer0;

    cluster.wait_leader();

    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    ASSERT_EQ(leader->node_id().peer_id, peer0);
    LOG(WARNING) << "leader is " << leader->node_id();

    fiber::CountdownEvent cond(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // start peer1
    melon::raft::PeerId peer1;
    peer1.addr.ip = mutil::my_ip();
    peer1.addr.port = 5006 + 1;
    peer1.idx = 0;
    ASSERT_EQ(0, cluster.start(peer1.addr, true));
    LOG(INFO) << "start peer " << peer1;
    // wait until started successfully
    usleep(1000* 1000);

    // add peer1
    cond.reset(1);
    leader->add_peer(peer1, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();
    LOG(INFO) << "add peer " << peer1;

    cluster.ensure_same();

    // add peer2 when peer not start
    melon::raft::PeerId peer2;
    peer2.addr.ip = mutil::my_ip();
    peer2.addr.port = 5006 + 2;
    peer2.idx = 0;

    cond.reset(1);
    peers.push_back(peer1);
    leader->add_peer(peer2, NEW_ADDPEERCLOSURE(&cond, melon::raft::ECATCHUP));
    cond.wait();

    // start peer2 after some seconds wait 
    sleep(2);
    ASSERT_EQ(0, cluster.start(peer2.addr, true));
    LOG(INFO) << "start peer " << peer2;

    usleep(1000 * 1000L);

    melon::raft::PeerId peer4("192.168.1.1:1234");

    // re add peer2
    cond.reset(2);
    // {peer0,peer1} add peer2
    leader->add_peer(peer2, NEW_ADDPEERCLOSURE(&cond, 0));
    // concurrent configration change
    leader->add_peer(peer4, NEW_ADDPEERCLOSURE(&cond, EBUSY));
    cond.wait();

    cond.reset(1);
    // retry add_peer direct ok
    leader->add_peer(peer2, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();

    cluster.ensure_same();

    cluster.stop_all();
}

TEST_P(NodeTest, Leader_step_down_during_install_snapshot) {
    std::vector<melon::raft::PeerId> peers;
    melon::raft::PeerId peer0;
    peer0.addr.ip = mutil::my_ip();
    peer0.addr.port = 5006;
    peer0.idx = 0;

    // start cluster
    peers.push_back(peer0);
    Cluster cluster("unittest", peers, 1000);
    ASSERT_EQ(0, cluster.start(peer0.addr));
    LOG(INFO) << "start single cluster " << peer0;

    cluster.wait_leader();

    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    ASSERT_EQ(leader->node_id().peer_id, peer0);
    LOG(WARNING) << "leader is " << leader->node_id();

    fiber::CountdownEvent cond(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(256 * 1024, 'a');
        data.append(data_buf);
        
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // trigger leader snapshot
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    cond.reset(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(256 * 1024, 'b');
        data.append(data_buf);
        
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    
    // trigger leader snapshot again to compact logs
    LOG(WARNING) << "trigger leader snapshot again";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // start peer1
    melon::raft::PeerId peer1;
    peer1.addr.ip = mutil::my_ip();
    peer1.addr.port = 5006 + 1;
    peer1.idx = 0;
    ASSERT_EQ(0, cluster.start(peer1.addr, true));
    LOG(INFO) << "start peer " << peer1;
    // wait until started successfully
    usleep(1000* 1000);

    // add peer1, leader step down while caught_up
    cond.reset(1);
    LOG(INFO) << "add peer: " << peer1;
    leader->add_peer(peer1, NEW_ADDPEERCLOSURE(&cond, EPERM));
    usleep(500 * 1000);

    {
        melon::Channel channel;
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_HTTP;
        if (channel.Init(leader->node_id().peer_id.addr, &options) != 0) {
            LOG(ERROR) << "Fail to initialize channel";
        }
        {
            melon::Controller cntl;
            cntl.http_request().uri() = "/raft_stat/unittest";
            cntl.http_request().set_method(melon::HTTP_METHOD_GET);
            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL/* done*/);
            LOG(INFO) << "http return: \n" << cntl.response_attachment();
        }
    }

    LOG(INFO) << "leader " << leader->node_id() 
                << " step_down because of some error";
    mutil::Status status;
    status.set_error(melon::raft::ERAFTTIMEDOUT, "Majority of the group dies");
    leader->_impl->step_down(leader->_impl->_current_term, false, status);
    cond.wait(); 
    
    // add peer1 again, success 
    LOG(INFO) << "add peer again: " << peer1;
    cond.reset(1);
    cluster.wait_leader();
    leader = cluster.leader();
    leader->add_peer(peer1, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait(); 
    
    cluster.ensure_same();
    
    LOG(INFO) << "stop cluster";
    cluster.stop_all();
}


TEST_P(NodeTest, Report_error_during_install_snapshot) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(256 * 1024, 'a');
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    // stop follower
    LOG(WARNING) << "stop follower";
    mutil::EndPoint follower_addr = nodes[0]->node_id().peer_id.addr;
    cluster.stop(follower_addr);

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(256 * 1024, 'b');
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // trigger leader snapshot
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // apply something
    cond.reset(10);
    for (int i = 20; i < 30; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(256 * 1024, 'c');
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // trigger leader snapshot again to compact logs
    LOG(WARNING) << "trigger leader snapshot again";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    LOG(WARNING) << "restart follower";
    ASSERT_EQ(0, cluster.start(follower_addr));
    usleep(1*1000*1000);
    
    // trigger newly-started follower report_error when install_snapshot
    cluster._nodes.back()->_impl->_snapshot_executor->report_error(EIO, "%s", 
                                                    "Fail to close writer");
    
    sleep(2);
    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, RemoveFollower) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    fiber::CountdownEvent cond(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    const melon::raft::PeerId follower_id = nodes[0]->node_id().peer_id;
    const mutil::EndPoint follower_addr = follower_id.addr;
    // stop follower
    LOG(WARNING) << "stop and clean follower " << follower_addr;
    cluster.stop(follower_addr);
    cluster.clean(follower_addr);

    // remove follower
    LOG(WARNING) << "remove follower " << follower_addr;
    cond.reset(1);
    leader->remove_peer(follower_id, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.followers(&nodes);
    ASSERT_EQ(1, nodes.size());

    peers.clear();
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        if (peer.addr != follower_addr) {
            peers.push_back(peer);
        }
    }

    // start follower
    LOG(WARNING) << "start follower " << follower_addr;
    ASSERT_EQ(0, cluster.start(follower_addr));

    // re add follower fail when leader step down
    LOG(WARNING) << "add follower " << follower_addr;
    cond.reset(1);
    leader->add_peer(follower_id, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();

    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    cluster.ensure_same();
}

TEST_P(NodeTest, RemoveLeader) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    fiber::CountdownEvent cond(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    mutil::EndPoint old_leader_addr = leader->node_id().peer_id.addr;
    LOG(WARNING) << "remove leader " << old_leader_addr;
    cond.reset(1);
    leader->remove_peer(leader->node_id().peer_id, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();

    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    LOG(INFO) << "here";
    cond.wait();

    LOG(WARNING) << "stop and clear leader " << old_leader_addr;
    cluster.stop(old_leader_addr);
    cluster.clean(old_leader_addr);

    LOG(WARNING) << "start old leader " << old_leader_addr;
    cluster.start(old_leader_addr);

    LOG(WARNING) << "add old leader " << old_leader_addr;
    cond.reset(1);
    peers.clear();
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        if (peer.addr != old_leader_addr) {
            peers.push_back(peer);
        }
    }
    leader->add_peer(melon::raft::PeerId(old_leader_addr, 0), NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    cluster.ensure_same();
}

TEST_P(NodeTest, restart_without_stable_meta) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    // stop follower
    LOG(WARNING) << "stop follower";
    mutil::EndPoint follower_addr = nodes[0]->node_id().peer_id.addr;
    cluster.stop(follower_addr);

    ::system(mutil::string_printf("rm -rf ./data/%s/stable/*",
                                 mutil::endpoint2str(follower_addr).c_str()).c_str());

    LOG(INFO) << "restart follower";
    ASSERT_EQ(0, cluster.start(follower_addr));

    fiber_usleep(1000*1000);
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(INFO) << "leader is " << leader->node_id();

    // apply something
    cond.reset(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, PreVote) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    fiber::CountdownEvent cond(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());
    const melon::raft::PeerId follower_id = nodes[0]->node_id().peer_id;
    const mutil::EndPoint follower_addr = follower_id.addr;

    const int64_t saved_term = leader->_impl->_current_term;
    //remove follower
    LOG(WARNING) << "remove follower " << follower_addr;
    cond.reset(1);
    leader->remove_peer(follower_id, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    sleep(2);

    //add follower
    LOG(WARNING) << "add follower " << follower_addr;
    peers.clear();
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        if (peer.addr != follower_addr) {
            peers.push_back(peer);
        }
    }
    cond.reset(1);
    leader->add_peer(follower_id, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();

    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);

    ASSERT_EQ(saved_term, leader->_impl->_current_term);
}

TEST_P(NodeTest, Vote_timedout) {
    google::SetCommandLineOption("raft_step_down_when_vote_timedout", "true");
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 2; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers, 500);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 1));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    usleep(1000 * 1000);
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_FALSE(nodes.empty());
    // stop follower, only one node left 
    const mutil::EndPoint follower_addr = nodes[0]->_impl->_server_id.addr;
    cluster.stop(follower_addr);
    
    // wait old leader to step down 
    usleep(2000 * 1000);
    // trigger old leader to vote, expecting fail when vote timedout
    std::unique_lock<raft_mutex_t> lck(leader->_impl->_mutex);
    leader->_impl->elect_self(&lck);
    lck.unlock();
    usleep(3000 * 1000);
   
    // start the stopped follower
    LOG(WARNING) << "restart follower";
    cluster.start(follower_addr);
    usleep(2000 * 1000);

    ASSERT_TRUE(cluster.ensure_same(5));
    LOG(WARNING) << "cluster stop";
    cluster.stop_all();

    google::SetCommandLineOption("raft_step_down_when_vote_timedout", "false");
}

TEST_P(NodeTest, SetPeer1) {
    // bootstrap from null
    Cluster cluster("unittest", std::vector<melon::raft::PeerId>());
    melon::raft::PeerId boot_peer;
    boot_peer.addr.ip = mutil::my_ip();
    boot_peer.addr.port = 5006;
    boot_peer.idx = 0;

    ASSERT_EQ(0, cluster.start(boot_peer.addr));
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(1, nodes.size());

    std::vector<melon::raft::PeerId> peers;
    peers.push_back(boot_peer);
    ASSERT_TRUE(nodes[0]->reset_peers(melon::raft::Configuration(peers)).ok());

    cluster.wait_leader();
}

TEST_P(NodeTest, SetPeer2) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    std::cout << "Here" << std::endl;
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    mutil::EndPoint leader_addr = leader->node_id().peer_id.addr;
    LOG(WARNING) << "leader is " << leader->node_id();
    std::cout << "Here" << std::endl;

    fiber::CountdownEvent cond(10);
    // apply something
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    std::cout << "Here" << std::endl;

    // check follower
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());
    melon::raft::PeerId follower_peer1 = nodes[0]->node_id().peer_id;
    melon::raft::PeerId follower_peer2 = nodes[1]->node_id().peer_id;

    LOG(WARNING) << "stop and clean follower " << follower_peer1;
    cluster.stop(follower_peer1.addr);
    cluster.clean(follower_peer1.addr);

    std::cout << "Here" << std::endl;
    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    
    std::cout << "Here" << std::endl;
    //set peer when no quorum die
    std::vector<melon::raft::PeerId> new_peers;
    LOG(WARNING) << "set peer to " << leader_addr;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        if (peer != follower_peer1) {
            new_peers.push_back(peer);
        }
    }
    LOG(WARNING) << "stop and clean follower " << follower_peer2;
    cluster.stop(follower_peer2.addr);
    cluster.clean(follower_peer2.addr);

    // leader will stepdown, become follower
    sleep(2);

    new_peers.clear();
    new_peers.push_back(melon::raft::PeerId(leader_addr, 0));

    // new peers equal current conf
    ASSERT_TRUE(leader->reset_peers(melon::raft::Configuration(peers)).ok());
    // set peer when quorum die
    LOG(WARNING) << "set peer to " << leader_addr;
    new_peers.clear();
    new_peers.push_back(melon::raft::PeerId(leader_addr, 0));
    ASSERT_TRUE(leader->reset_peers(melon::raft::Configuration(new_peers)).ok());

    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    ASSERT_EQ(leader->node_id().peer_id.addr, leader_addr);

    LOG(WARNING) << "start old follower " << follower_peer1;
    ASSERT_EQ(0, cluster.start(follower_peer1.addr, true));
    LOG(WARNING) << "start old follower " << follower_peer2;
    ASSERT_EQ(0, cluster.start(follower_peer2.addr, true));

    LOG(WARNING) << "add old follower " << follower_peer1;
    cond.reset(1);
    leader->add_peer(follower_peer1, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();

    LOG(WARNING) << "add old follower " << follower_peer2;
    cond.reset(1);
    new_peers.push_back(follower_peer1);
    leader->add_peer(follower_peer2, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();

    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    cluster.ensure_same();
}

TEST_P(NodeTest, RestoreSnapshot) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    mutil::EndPoint leader_addr = leader->node_id().peer_id.addr;

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    // trigger leader snapshot
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // stop leader
    LOG(WARNING) << "stop leader";
    cluster.stop(leader->node_id().peer_id.addr);

    sleep(2);

    LOG(WARNING) << "restart leader";
    ASSERT_EQ(0, cluster.start(leader_addr));

    cluster.ensure_same();

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, InstallSnapshot) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(2, nodes.size());

    // stop follower
    LOG(WARNING) << "stop follower";
    mutil::EndPoint follower_addr = nodes[0]->node_id().peer_id.addr;
    cluster.stop(follower_addr);

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // trigger leader snapshot
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // apply something
    cond.reset(10);
    for (int i = 20; i < 30; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // trigger leader snapshot again to compact logs
    LOG(WARNING) << "trigger leader snapshot again";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    LOG(WARNING) << "restart follower";
    ASSERT_EQ(0, cluster.start(follower_addr));

    sleep(2);

    cluster.ensure_same();

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, install_snapshot_exceed_max_task_num) {
    google::SetCommandLineOption("raft_max_install_snapshot_tasks_num", "1");
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 5; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(4, nodes.size());

    // stop follower
    LOG(WARNING) << "stop follower";
    mutil::EndPoint follower_addr = nodes[0]->node_id().peer_id.addr;
    mutil::EndPoint follower_addr2 = nodes[1]->node_id().peer_id.addr;
    cluster.stop(follower_addr);
    cluster.stop(follower_addr2);

    // apply something
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(128 * 1024, 'a');
        data.append(data_buf);
        
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);        
    }
    cond.wait();

    // trigger leader snapshot
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // apply something
    cond.reset(10);
    for (int i = 20; i < 30; i++) {
        mutil::IOBuf data;
        std::string data_buf;
        data_buf.resize(128 * 1024, 'b');
        data.append(data_buf);
        
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);    
    }
    cond.wait();

    // trigger leader snapshot again to compact logs
    LOG(WARNING) << "trigger leader snapshot again";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    LOG(WARNING) << "restart follower";
    ASSERT_EQ(0, cluster.start(follower_addr));
    ASSERT_EQ(0, cluster.start(follower_addr2));

    usleep(5 * 1000 * 1000);

    cluster.ensure_same();

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
    google::SetCommandLineOption("raft_max_install_snapshot_tasks_num", "1000");
}

TEST_P(NodeTest, NoSnapshot) {
    melon::Server server;
    melon::ServerOptions server_options;
    int ret = melon::raft::add_service(&server, "0.0.0.0:5006");
    ASSERT_EQ(0, ret);
    ASSERT_EQ(0, server.Start(5006, &server_options));

    melon::raft::PeerId peer;
    peer.addr.ip = mutil::my_ip();
    peer.addr.port = 5006;
    peer.idx = 0;
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(peer);

    melon::raft::NodeOptions options;
    options.election_timeout_ms = 300;
    options.initial_conf = melon::raft::Configuration(peers);
    options.fsm = new MockFSM(mutil::EndPoint());
    options.log_uri = "local://./data/log";
    options.raft_meta_uri = "local://./data/raft_meta";

    melon::raft::Node node("unittest", peer);
    ASSERT_EQ(0, node.init(options));

    // wait node elect to leader
    sleep(2);

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        node.apply(task);
    }
    cond.wait();

    // trigger snapshot, not expect ret
    cond.reset(1);
    node.snapshot(NEW_SNAPSHOTCLOSURE(&cond, -1));
    cond.wait();

    // shutdown
    cond.reset(1);
    node.shutdown(NEW_SHUTDOWNCLOSURE(&cond, 0));
    cond.wait();

    // stop
    server.Stop(200);
    server.Join();
}

TEST_P(NodeTest, AutoSnapshot) {
    melon::Server server;
    melon::ServerOptions server_options;
    int ret = melon::raft::add_service(&server, "0.0.0.0:5006");
    ASSERT_EQ(0, ret);
    ASSERT_EQ(0, server.Start(5006, &server_options));

    melon::raft::PeerId peer;
    peer.addr.ip = mutil::my_ip();
    peer.addr.port = 5006;
    peer.idx = 0;
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(peer);

    melon::raft::NodeOptions options;
    options.election_timeout_ms = 300;
    options.initial_conf = melon::raft::Configuration(peers);
    options.fsm = new MockFSM(mutil::EndPoint());
    options.log_uri = "local://./data/log";
    options.raft_meta_uri = "local://./data/raft_meta";
    options.snapshot_uri = "local://./data/snapshot";
    options.snapshot_interval_s = 10;

    melon::raft::Node node("unittest", peer);
    ASSERT_EQ(0, node.init(options));

    // wait node elect to leader
    sleep(2);

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        node.apply(task);
    }
    cond.wait();

    sleep(10);
    ASSERT_GT(static_cast<MockFSM*>(options.fsm)->snapshot_index, 0);

    // shutdown
    cond.reset(1);
    node.shutdown(NEW_SHUTDOWNCLOSURE(&cond, 0));
    cond.wait();

    // stop
    server.Stop(200);
    server.Join();
}

TEST_P(NodeTest, LeaderShouldNotChange) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader0 = cluster.leader();
    ASSERT_TRUE(leader0 != NULL);
    LOG(WARNING) << "leader is " << leader0->node_id();
    const int64_t saved_term = leader0->_impl->_current_term;
    usleep(5000 * 1000);
    cluster.wait_leader();
    melon::raft::Node* leader1 = cluster.leader();
    LOG(WARNING) << "leader is " << leader1->node_id();
    ASSERT_EQ(leader0, leader1);
    ASSERT_EQ(saved_term, leader1->_impl->_current_term);
    cluster.stop_all();
}

TEST_P(NodeTest, RecoverFollower) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 1));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    usleep(1000 * 1000);
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_FALSE(nodes.empty());
    const mutil::EndPoint follower_addr = nodes[0]->_impl->_server_id.addr;
    cluster.stop(follower_addr);

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "no closure");
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        leader->apply(task);
    }
    // wait leader to compact logs
    usleep(5000 * 1000);

    // Start the stopped follower, expecting that leader would recover it
    LOG(WARNING) << "restart follower";
    cluster.start(follower_addr);
    LOG(WARNING) << "restart follower done";
    LOG(WARNING) << "here";
    ASSERT_TRUE(cluster.ensure_same(5));
    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, leader_transfer) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 1));
    }
    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    melon::raft::PeerId target = nodes[0]->node_id().peer_id;
    ASSERT_EQ(0, leader->transfer_leadership_to(target));
    usleep(10 * 1000);
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_EQ(target, leader->node_id().peer_id);
    ASSERT_TRUE(cluster.ensure_same(5));
    cluster.stop_all();
}
TEST_P(NodeTest, leader_witness_temporary_be_leader) {
    FLAGS_raft_enable_witness_to_leader = true;
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        if (i == 0) {
            peer.role = melon::raft::Role::WITNESS;
        }
        peers.push_back(peer);
    }
    // start cluster
    Cluster cluster("unittest", peers, 5000);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 30, nullptr, peers[i].is_witness()));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);

    // stop follower so witness would had more entry logs than follower
    melon::raft::Node* follower_node = nodes[1];
    melon::raft::PeerId follower = follower_node->node_id().peer_id;
    cluster.stop(follower.addr);
    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // stop leader
    mutil::EndPoint old_leader = leader->node_id().peer_id.addr;
    LOG(WARNING) << "stop leader " << leader->node_id();
    cluster.stop(leader->node_id().peer_id.addr);

    // old follower restart
    ASSERT_EQ(0, cluster.start(follower.addr));
    LOG(WARNING) << "restart old follower " << follower.addr;

    // elect leader
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    // wait witness auto step_down and transfer leader.
    while (true) {
        if (leader->is_leader()) {
            usleep(1000* 1000);
            continue;
        }
            break;
    }
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    
    cluster.start(old_leader);
    LOG(WARNING) << "restart old leader " << old_leader;
    cluster.ensure_same();

    cluster.stop_all();
}

TEST_P(NodeTest, leader_transfer_before_log_is_compleleted) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers, 5000);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 1));
    }
    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    melon::raft::PeerId target = nodes[0]->node_id().peer_id;
    cluster.stop(target.addr);
    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    ASSERT_EQ(EHOSTUNREACH, leader->transfer_leadership_to(target));
    cond.reset(1);
    melon::raft::Task task;
    mutil::IOBuf data;
    data.resize(5, 'a');
    task.data = &data;
    task.done = NEW_APPLYCLOSURE(&cond, 0);
    leader->apply(task);
    cond.wait();
    melon::raft::Node* saved_leader = leader;
    cluster.start(target.addr);
    usleep(5000 * 1000);
    LOG(INFO) << "here";
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_EQ(saved_leader->node_id().peer_id, leader->node_id().peer_id);
    ASSERT_TRUE(cluster.ensure_same(5));
    cluster.stop_all();
}

TEST_P(NodeTest, leader_transfer_resume_on_failure) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr, false, 1));
    }
    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    melon::raft::PeerId target = nodes[0]->node_id().peer_id;
    cluster.stop(target.addr);
    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    ASSERT_EQ(EHOSTUNREACH, leader->transfer_leadership_to(target));
    melon::raft::Node* saved_leader = leader;
    cond.reset(1);
    melon::raft::Task task;
    mutil::IOBuf data;
    data.resize(5, 'a');
    task.data = &data;
    task.done = NEW_APPLYCLOSURE(&cond, 0);
    leader->apply(task);
    cond.wait();
    //cluster.start(target.addr);
    usleep(1000 * 1000);
    LOG(INFO) << "here";
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_EQ(saved_leader, leader);
    LOG(INFO) << "restart the target follower";
    cluster.start(target.addr);
    usleep(1000 * 1000);
    data.resize(5, 'a');
    task.data = &data;
    cond.reset(1);
    task.done = NEW_APPLYCLOSURE(&cond, 0);
    leader->apply(task);
    cond.wait();
    ASSERT_TRUE(cluster.ensure_same(5));
    cluster.stop_all();
}

class MockFSM1 : public MockFSM {
protected:
    MockFSM1() : MockFSM(mutil::EndPoint()) {}
    virtual int on_snapshot_load(melon::raft::SnapshotReader* reader) {
        (void)reader;
        return -1;
    }
};

TEST_P(NodeTest, shutdown_and_join_work_after_init_fails) {
    melon::Server server;
    int ret = melon::raft::add_service(&server, 5006);
    server.Start(5006, NULL);
    ASSERT_EQ(0, ret);

    melon::raft::PeerId peer;
    peer.addr.ip = mutil::my_ip();
    peer.addr.port = 5006;
    peer.idx = 0;
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(peer);

    {
        melon::raft::NodeOptions options;
        options.election_timeout_ms = 300;
        options.initial_conf = melon::raft::Configuration(peers);
        options.fsm = new MockFSM1();
        options.log_uri = "local://./data/log";
        options.raft_meta_uri = "local://./data/raft_meta";
        options.snapshot_uri = "local://./data/snapshot";
        melon::raft::Node node("unittest", peer);
        ASSERT_EQ(0, node.init(options));
        sleep(1);
        fiber::CountdownEvent cond(10);
        for (int i = 0; i < 10; i++) {
            mutil::IOBuf data;
            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
            data.append(data_buf);
            melon::raft::Task task;
            task.data = &data;
            task.done = NEW_APPLYCLOSURE(&cond, 0);
            node.apply(task);
        }
        cond.wait();
        LOG(INFO) << "begin to save snapshot";
        node.snapshot(NULL);
        LOG(INFO) << "begin to shutdown";
        node.shutdown(NULL);
        node.join();
    }
    
    {
        melon::raft::NodeOptions options;
        options.election_timeout_ms = 300;
        options.initial_conf = melon::raft::Configuration(peers);
        options.fsm = new MockFSM1();
        options.log_uri = "local://./data/log";
        options.raft_meta_uri = "local://./data/raft_meta";
        options.snapshot_uri = "local://./data/snapshot";
        melon::raft::Node node("unittest", peer);
        LOG(INFO) << "node init again";
        ASSERT_NE(0, node.init(options));
        node.shutdown(NULL);
        node.join();
    }

    server.Stop(200);
    server.Join();
}

TEST_P(NodeTest, shutting_leader_triggers_timeout_now) {
    google::SetCommandLineOption("raft_sync", "false");
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        peers.push_back(peer);
    }
    // start cluster
    Cluster cluster("unittest", peers, 1000);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(INFO) << "shutdown leader" << leader->node_id();
    leader->shutdown(NULL);
    leader->join();
    LOG(INFO) << "join";
    usleep(100 * 1000);
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    google::SetCommandLineOption("raft_sync", "true");
}

TEST_P(NodeTest, removing_leader_triggers_timeout_now) {
    google::SetCommandLineOption("raft_sync", "false");
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        peers.push_back(peer);
    }
    // start cluster
    Cluster cluster("unittest", peers, 1000);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    melon::raft::PeerId old_leader_id = leader->node_id().peer_id;
    LOG(WARNING) << "remove leader " << old_leader_id;
    fiber::CountdownEvent cond;
    leader->remove_peer(old_leader_id, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();
    usleep(100 * 1000);
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    ASSERT_NE(old_leader_id, leader->node_id().peer_id);
    google::SetCommandLineOption("raft_sync", "true");
}

TEST_P(NodeTest, transfer_should_work_after_install_snapshot) {
    std::vector<melon::raft::PeerId> peers;
    for (size_t i = 0; i < 3; ++i) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        peers.push_back(peer);
    }
    // start cluster
    Cluster cluster("unittest", peers, 1000);
    for (size_t i = 0; i < peers.size() - 1; i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(1, nodes.size());
    melon::raft::PeerId follower = nodes[0]->node_id().peer_id;
    leader->transfer_leadership_to(follower);
    usleep(2000 * 1000);
    leader = cluster.leader();
    ASSERT_EQ(follower, leader->node_id().peer_id);
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // Start the last peer which should be recover with snapshot
    melon::raft::PeerId last_peer = peers.back();
    cluster.start(last_peer.addr);
    usleep(5000 * 1000);

    ASSERT_EQ(0, leader->transfer_leadership_to(last_peer));
    usleep(2000 * 1000);
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    ASSERT_EQ(last_peer, leader->node_id().peer_id);
}

TEST_P(NodeTest, append_entries_when_follower_is_in_error_state) {
    std::vector<melon::raft::PeerId> peers;
    // five nodes
    for (int i = 0; i < 5; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // set the first Follower to Error state
    std::vector<melon::raft::Node*> nodes;
    cluster.followers(&nodes);
    ASSERT_EQ(nodes.size(), 4);
    mutil::EndPoint error_follower = nodes[0]->node_id().peer_id.addr;
    melon::raft::Node* error_follower_node = nodes[0];
    LOG(WARNING) << "set follower error " << nodes[0]->node_id();
    melon::raft::NodeImpl *node_impl = nodes[0]->_impl;
    node_impl->AddRef();
    melon::raft::Error e;
    e.set_type(melon::raft::ERROR_TYPE_STATE_MACHINE);
    e.status().set_error(EINVAL, "Follower has something wrong");
    node_impl->on_error(e);
    node_impl->Release();

    // increase term  by stopping leader and electing a new leader again
    mutil::EndPoint old_leader = leader->node_id().peer_id.addr;
    LOG(WARNING) << "stop leader " << leader->node_id();
    cluster.stop(old_leader);
    // elect new leader
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "elect new leader " << leader->node_id();

    // apply something again 
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    sleep(2);
    // stop error follower
    LOG(WARNING) << "stop wrong follower " << error_follower_node->node_id();
    cluster.stop(error_follower);
    
    sleep(5);
    // restart error follower
    ASSERT_EQ(0, cluster.start(error_follower));
    LOG(WARNING) << "restart error follower " << error_follower;

    // restart old leader
    ASSERT_EQ(0, cluster.start(old_leader));
    LOG(WARNING) << "restart old leader " << old_leader;

    cluster.ensure_same();

    cluster.stop_all();
}

TEST_P(NodeTest, on_start_following_and_on_stop_following) {
    std::vector<melon::raft::PeerId> peers;
    // five nodes
    for (int i = 0; i < 5; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader_first
    cluster.wait_leader();
    melon::raft::Node* leader_first = cluster.leader();
    ASSERT_TRUE(leader_first != NULL);
    LOG(WARNING) << "leader_first is " << leader_first->node_id()
                 << ", election_timeout is " 
                 << leader_first->_impl->_options.election_timeout_ms;

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader_first->apply(task);
    }
    cond.wait();
   
    // check _on_start_following_times and _on_stop_following_times
    std::vector<melon::raft::Node*> followers_first;
    cluster.followers(&followers_first);
    ASSERT_EQ(followers_first.size(), 4);
    // leader_first's _on_start_following_times and _on_stop_following_times should both be 0.
    ASSERT_EQ(static_cast<MockFSM*>(leader_first->_impl->_options.fsm)->_on_start_following_times, 0);
    ASSERT_EQ(static_cast<MockFSM*>(leader_first->_impl->_options.fsm)->_on_stop_following_times, 0);
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(static_cast<MockFSM*>(followers_first[i]->_impl->_options.fsm)->_on_start_following_times, 1);
        ASSERT_EQ(static_cast<MockFSM*>(followers_first[i]->_impl->_options.fsm)->_on_stop_following_times, 0);
    }

    // stop old leader and elect a new one
    mutil::EndPoint leader_first_endpoint = leader_first->node_id().peer_id.addr;
    LOG(WARNING) << "stop leader_first " << leader_first->node_id();
    cluster.stop(leader_first_endpoint);
    // elect new leader
    cluster.wait_leader();
    melon::raft::Node* leader_second = cluster.leader();
    ASSERT_TRUE(leader_second != NULL);
    LOG(WARNING) << "elect new leader " << leader_second->node_id();

    // apply something
    cond.reset(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader_second->apply(task);
    }
    cond.wait();
 
    // check _on_start_following_times and _on_stop_following_times again
    std::vector<melon::raft::Node*> followers_second;
    cluster.followers(&followers_second);
    ASSERT_EQ(followers_second.size(), 3);
    // leader_second's _on_start_following_times and _on_stop_following_times should both be 1.
    // When it was still in follower state, it would do handle_election_timeout and
    // trigger on_stop_following when not receiving heartbeat for a long
    // time(election_timeout_ms).
    ASSERT_GE(static_cast<MockFSM*>(leader_second->_impl->_options.fsm)->_on_start_following_times, 1);
    ASSERT_GE(static_cast<MockFSM*>(leader_second->_impl->_options.fsm)->_on_stop_following_times, 1);
    for (int i = 0; i < 3; i++) {
        // Firstly these followers have a leader, but it stops and a candidate
        // sends request_vote_request to them, which triggers on_stop_following.
        // When the candidate becomes new leader, on_start_following is triggled
        // again so _on_start_following_times increase by 1.
        ASSERT_GE(static_cast<MockFSM*>(followers_second[i]->_impl->_options.fsm)->_on_start_following_times, 2);
        ASSERT_GE(static_cast<MockFSM*>(followers_second[i]->_impl->_options.fsm)->_on_stop_following_times, 1);
    }

    // transfer leadership to a follower
    melon::raft::PeerId target = followers_second[0]->node_id().peer_id;
    ASSERT_EQ(0, leader_second->transfer_leadership_to(target));
    usleep(10 * 1000);
    cluster.wait_leader();
    melon::raft::Node* leader_third = cluster.leader();
    ASSERT_EQ(target, leader_third->node_id().peer_id);
    
    // apply something
    cond.reset(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader_third->apply(task);
    }
    cond.wait();
    
    // check _on_start_following_times and _on_stop_following_times again 
    std::vector<melon::raft::Node*> followers_third;
    cluster.followers(&followers_third);
    ASSERT_EQ(followers_second.size(), 3);
    // leader_third's _on_start_following_times and _on_stop_following_times should both be 2.
    // When it was still in follower state, it would do handle_timeout_now_request and
    // trigger on_stop_following when leader_second transferred leadership to it.
    ASSERT_GE(static_cast<MockFSM*>(leader_third->_impl->_options.fsm)->_on_start_following_times, 2);
    ASSERT_GE(static_cast<MockFSM*>(leader_third->_impl->_options.fsm)->_on_stop_following_times, 2);
    for (int i = 0; i < 3; i++) {
        // leader_second became follower when it transferred leadership to target, 
        // and when it receives leader_third's append_entries_request on_start_following is triggled.
        if (followers_third[i]->node_id().peer_id == leader_second->node_id().peer_id) {
            ASSERT_GE(static_cast<MockFSM*>(followers_third[i]->_impl->_options.fsm)->_on_start_following_times, 2);
            ASSERT_GE(static_cast<MockFSM*>(followers_third[i]->_impl->_options.fsm)->_on_stop_following_times, 1);
            continue;
        }
        // other followers just lose the leader_second and get leader_third, so _on_stop_following_times and 
        // _on_start_following_times both increase by 1. 
        ASSERT_GE(static_cast<MockFSM*>(followers_third[i]->_impl->_options.fsm)->_on_start_following_times, 3);
        ASSERT_GE(static_cast<MockFSM*>(followers_third[i]->_impl->_options.fsm)->_on_stop_following_times, 2);
    }

    cluster.ensure_same();
   
    cluster.stop_all();
}

TEST_P(NodeTest, read_committed_user_log) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;
        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader_first
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    sleep(2);
    
    // index == 1 is a CONFIGURATION log, so real_index will be 2 when returned.
    int64_t index = 1;
    melon::raft::UserLog* user_log = new melon::raft::UserLog();
    mutil::Status status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(0, status.error_code());
    ASSERT_EQ(2, user_log->log_index());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 

    // index == 5 is a DATA log(a user log)
    index = 5;
    user_log->reset();
    status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(0, status.error_code());
    ASSERT_EQ(5, user_log->log_index());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 
    
    // index == 15 is greater than last_committed_index
    index = 15;
    user_log->reset();
    status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(melon::raft::ENOMOREUSERLOG, status.error_code());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 
    
    // index == 0, invalid request index.
    index = 0;
    user_log->reset();
    status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(EINVAL, status.error_code());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 
   
    // trigger leader snapshot for the first time
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();
    
    // remove and add a peer to add two CONFIGURATION logs
    std::vector<melon::raft::Node*> followers;
    cluster.followers(&followers);
    melon::raft::PeerId follower_test = followers[0]->node_id().peer_id;
    cond.reset(1);
    leader->remove_peer(follower_test, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();
    std::vector<melon::raft::PeerId> new_peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        if (peer != follower_test) {
            new_peers.push_back(peer);
        }
    }
    cond.reset(1);
    leader->add_peer(follower_test, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();

    // apply something again
    cond.reset(10);
    for (int i = 10; i < 20; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    
    // trigger leader snapshot for the second time, after this the log of index 1~11 will be deleted.
    LOG(WARNING) << "trigger leader snapshot ";
    cond.reset(1);
    leader->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // index == 5 log has been deleted in log_storage.
    index = 5;
    user_log->reset();
    status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(melon::raft::ELOGDELETED, status.error_code());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 
    
    // index == 12 and index == 13 are 2 CONFIGURATION logs, so real_index will be 14 when returned.
    index = 12;
    user_log->reset();
    status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(0, status.error_code());
    ASSERT_EQ(14, user_log->log_index());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 
    
    // now index == 15 is a user log
    index = 15;
    user_log->reset();
    status = leader->read_committed_user_log(index, user_log);
    ASSERT_EQ(0, status.error_code());
    ASSERT_EQ(15, user_log->log_index());
    LOG(INFO) << "read local committed user log from leader:" << leader->node_id() << ", index:"
        << index << ", real_index:" << user_log->log_index() << ", data:" << user_log->log_data() 
        << ", status:" << status; 
   
    delete(user_log);

    cluster.ensure_same();
    cluster.stop_all();
}

TEST_P(NodeTest, boostrap_with_snapshot) {
    mutil::EndPoint addr;
    ASSERT_EQ(0, mutil::str2endpoint("127.0.0.1:5006", &addr));
    MockFSM fsm(addr);
    for (char c = 'a'; c <= 'z'; ++c) {
        mutil::IOBuf buf;
        buf.resize(100, c);
        fsm.logs.push_back(buf);
    }
    melon::raft::BootstrapOptions boptions;
    boptions.last_log_index = fsm.logs.size();
    boptions.log_uri = "local://./data/log";
    boptions.raft_meta_uri = "local://./data/raft_meta";
    boptions.snapshot_uri = "local://./data/snapshot";
    boptions.group_conf.add_peer(melon::raft::PeerId(addr));
    boptions.node_owns_fsm = false;
    boptions.fsm = &fsm;
    ASSERT_EQ(0, melon::raft::bootstrap(boptions));
    melon::Server server;
    ASSERT_EQ(0, melon::raft::add_service(&server, addr));
    ASSERT_EQ(0, server.Start(addr, NULL));
    melon::raft::Node node("test", melon::raft::PeerId(addr));
    melon::raft::NodeOptions options;
    options.log_uri = "local://./data/log";
    options.raft_meta_uri = "local://./data/raft_meta";
    options.snapshot_uri = "local://./data/snapshot";
    options.node_owns_fsm = false;
    options.fsm = &fsm;
    ASSERT_EQ(0, node.init(options));
    ASSERT_EQ(26u, fsm.logs.size());
    for (char c = 'a'; c <= 'z'; ++c) {
        std::string expected;
        expected.resize(100, c);
        ASSERT_TRUE(fsm.logs[c - 'a'].equals(expected));
    }
    while (!node.is_leader()) {
        usleep(1000);
    }
    node.shutdown(NULL);
    node.join();
}

TEST_P(NodeTest, boostrap_without_snapshot) {
    mutil::EndPoint addr;
    ASSERT_EQ(0, mutil::str2endpoint("127.0.0.1:5006", &addr));
    melon::raft::BootstrapOptions boptions;
    boptions.last_log_index = 0;
    boptions.log_uri = "local://./data/log";
    boptions.raft_meta_uri = "local://./data/raft_meta";
    boptions.snapshot_uri = "local://./data/snapshot";
    boptions.group_conf.add_peer(melon::raft::PeerId(addr));
    ASSERT_EQ(0, melon::raft::bootstrap(boptions));
    melon::Server server;
    ASSERT_EQ(0, melon::raft::add_service(&server, addr));
    ASSERT_EQ(0, server.Start(addr, NULL));
    melon::raft::Node node("test", melon::raft::PeerId(addr));
    melon::raft::NodeOptions options;
    options.log_uri = "local://./data/log";
    options.raft_meta_uri = "local://./data/raft_meta";
    options.snapshot_uri = "local://./data/snapshot";
    options.node_owns_fsm = false;
    MockFSM fsm(addr);
    options.fsm = &fsm;
    ASSERT_EQ(0, node.init(options));
    while (!node.is_leader()) {
        usleep(1000);
    }
    node.shutdown(NULL);
    node.join();
}

TEST_P(NodeTest, change_peers) {
    std::vector<melon::raft::PeerId> peers;
    melon::raft::PeerId peer0;
    peer0.addr.ip = mutil::my_ip();
    peer0.addr.port = 5006;
    peer0.idx = 0;

    // start cluster
    peers.push_back(peer0);
    Cluster cluster("unittest", peers);
    ASSERT_EQ(0, cluster.start(peer0.addr));
    LOG(INFO) << "start single cluster " << peer0;
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    for (int i = 1; i < 10; ++i) {
        melon::raft::PeerId peer = peer0;
        peer.addr.port += i;
        ASSERT_EQ(0, cluster.start(peer.addr, true));
    }
    for (int i = 0; i < 9; ++i) {
        cluster.wait_leader();
        melon::raft::Node* leader = cluster.leader();
        melon::raft::PeerId peer = peer0;
        peer.addr.port += i;
        ASSERT_EQ(leader->node_id().peer_id, peer);
        peer.addr.port += 1;
        melon::raft::Configuration conf;
        conf.add_peer(peer);
        melon::raft::SynchronizedClosure done;
        leader->change_peers(conf, &done);
        done.wait();
        ASSERT_TRUE(done.status().ok()) << done.status();
    }
    cluster.wait_leader();
    ASSERT_TRUE(cluster.ensure_same());
}

TEST_P(NodeTest, change_peers_add_multiple_node) {
    std::vector<melon::raft::PeerId> peers;
    melon::raft::PeerId peer0;
    peer0.addr.ip = mutil::my_ip();
    peer0.addr.port = 5006;
    peer0.idx = 0;

    // start cluster
    peers.push_back(peer0);
    Cluster cluster("unittest", peers);
    ASSERT_EQ(0, cluster.start(peer0.addr));
    LOG(INFO) << "start single cluster " << peer0;
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    melon::raft::Configuration conf;
    for (int i = 0; i < 3; ++i) {
        melon::raft::PeerId peer = peer0;
        peer.addr.port += i;
        conf.add_peer(peer);
    }
    cluster.wait_leader();
    melon::raft::SynchronizedClosure done;
    leader->change_peers(conf, &done);
    done.wait();
    ASSERT_EQ(melon::raft::ECATCHUP, done.status().error_code()) << done.status();
    melon::raft::PeerId peer = peer0;
    peer.addr.port++;
    cluster.start(peer.addr, false);
    done.reset();
    leader->change_peers(conf, &done);
    done.wait();
    ASSERT_EQ(melon::raft::ECATCHUP, done.status().error_code()) << done.status();
    peer.addr.port++;
    cluster.start(peer.addr, false);
    done.reset();
    leader->change_peers(conf, &done);
    done.wait();
    ASSERT_TRUE(done.status().ok()) << done.status();
    ASSERT_TRUE(cluster.ensure_same());
}

TEST_P(NodeTest, change_peers_steps_down_in_joint_consensus) {
    std::vector<melon::raft::PeerId> peers;
    melon::raft::PeerId peer0("127.0.0.1:5006");
    melon::raft::PeerId peer1("127.0.0.1:5007");
    melon::raft::PeerId peer2("127.0.0.1:5008");
    melon::raft::PeerId peer3("127.0.0.1:5009");

    // start cluster
    peers.push_back(peer0);
    Cluster cluster("unittest", peers);
    ASSERT_EQ(0, cluster.start(peer0.addr));
    LOG(INFO) << "start single cluster " << peer0;
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    fiber::CountdownEvent cond(10);
    for (int i = 0; i < 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
    cluster.start(peer1.addr, false);
    cluster.start(peer2.addr, false);
    cluster.start(peer3.addr, false);
    melon::raft::Configuration conf;
    conf.add_peer(peer0);
    conf.add_peer(peer1);
    conf.add_peer(peer2);
    conf.add_peer(peer3);
    melon::raft::SynchronizedClosure done;
    leader->change_peers(conf, &done);
    done.wait();
    ASSERT_TRUE(done.status().ok());
    ASSERT_EQ(0, cluster.stop(peer3.addr));
    conf.remove_peer(peer0);
    conf.remove_peer(peer1);

    // Change peers to [peer2, peer3], which must fail since peer3 is stopped
    done.reset();
    leader->change_peers(conf, &done);
    done.wait();
    ASSERT_EQ(EPERM, done.status().error_code());
    ASSERT_FALSE(leader->_impl->_conf.stable());
    LOG(INFO) << done.status();
    leader = cluster.leader();
    ASSERT_TRUE(leader == NULL);
    cluster.start(peer3.addr, false);
    usleep(1000 * 1000);  // Temporarily solution
    cluster.wait_leader();
    leader = cluster.leader();
    ASSERT_TRUE(leader->list_peers(&peers).ok());
    ASSERT_TRUE(conf.equals(peers));
    ASSERT_TRUE(leader->_impl->_conf.stable() || !leader->_impl->_conf_ctx.is_busy());
    int wait_count = 1000;
    while (leader->_impl->_conf_ctx.is_busy() && wait_count > 0) {
        LOG(WARNING) << "wait util stable stage finish";
        usleep(5 * 1000);
        --wait_count;
    }
    ASSERT_TRUE(!leader->_impl->_conf_ctx.is_busy());
}

struct ChangeArg {
    Cluster* c;
    std::vector<melon::raft::PeerId> peers;
    volatile bool stop;
    bool dont_remove_first_peer;
};

static void* change_routine(void* arg) {
    ChangeArg* ca = (ChangeArg*)arg;
    while (!ca->stop) {
        ca->c->wait_leader();
        melon::raft::Node* leader = ca->c->leader();
        if (!leader) {
            continue;
        }
        // Randomly select peers
        melon::raft::Configuration conf;
        if (ca->dont_remove_first_peer) {
            conf.add_peer(ca->peers[0]);
        }
        for (size_t i = 0; i < ca->peers.size(); ++i) {
            bool select = mutil::fast_rand_less_than(64) < 32;
            if (select) {
                conf.add_peer(ca->peers[i]);
            }
        }
        if (conf.empty()) {
            LOG(WARNING) << "No peer has been selected";
            // Bad luck here
            continue;
        }
        melon::raft::SynchronizedClosure done;
        leader->change_peers(conf, &done);
        done.wait();
        CHECK(done.status().ok());
    }
    return NULL;
}

TEST_P(NodeTest, change_peers_chaos_with_snapshot) {
    g_dont_print_apply_log = true;
    google::SetCommandLineOption("raft_sync", "false");
    ASSERT_FALSE(google::SetCommandLineOption("crash_on_fatal_log", "true").empty());
    google::SetCommandLineOption("minloglevel", "3");
    std::vector<melon::raft::PeerId> peers;
    // start cluster
    peers.push_back(melon::raft::PeerId("127.0.0.1:5006"));
    Cluster cluster("unittest", peers, 2000);
    cluster.start(peers.front().addr, false, 1);
    for (int i = 1; i < 10; ++i) {
        peers.push_back("127.0.0.1:" + std::to_string(5006 + i));
        cluster.start(peers.back().addr, true, 1);
    }
    ChangeArg arg;
    arg.c = &cluster;
    arg.peers = peers;
    arg.stop = false;
    arg.dont_remove_first_peer = false;
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, NULL, change_routine, &arg));
    for (int i = 0; i < 1000;) {
        cluster.wait_leader();
        melon::raft::Node* leader = cluster.leader();
        if (!leader) {
            continue;
        }
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        melon::raft::SynchronizedClosure done;
        task.data = &data;
        task.done = &done;
        leader->apply(task);
        done.wait();
        if (done.status().ok()) {
            ++i;
        } else {
            EXPECT_EQ(EPERM, done.status().error_code());
        }
    }
    arg.stop = true;
    pthread_join(tid, NULL);
    google::SetCommandLineOption("raft_sync", "true");
    google::SetCommandLineOption("minloglevel", "1");
}

TEST_P(NodeTest, change_peers_chaos_without_snapshot) {
    g_dont_print_apply_log = true;
    google::SetCommandLineOption("minloglevel", "3");
    google::SetCommandLineOption("raft_sync", "false");
    ASSERT_FALSE(google::SetCommandLineOption("crash_on_fatal_log", "true").empty());
    std::vector<melon::raft::PeerId> peers;
    // start cluster
    peers.push_back(melon::raft::PeerId("127.0.0.1:5006"));
    Cluster cluster("unittest", peers, 2000);
    cluster.start(peers.front().addr, false, 10000);
    for (int i = 1; i < 10; ++i) {
        peers.push_back("127.0.0.1:" + std::to_string(5006 + i));
        cluster.start(peers.back().addr, true, 10000);
    }
    ChangeArg arg;
    arg.c = &cluster;
    arg.peers = peers;
    arg.stop = false;
    arg.dont_remove_first_peer = true;
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, NULL, change_routine, &arg));
    for (int i = 0; i < 10000;) {
        cluster.wait_leader();
        melon::raft::Node* leader = cluster.leader();
        if (!leader) {
            continue;
        }
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i + 1);
        data.append(data_buf);
        melon::raft::Task task;
        melon::raft::SynchronizedClosure done;
        task.data = &data;
        task.done = &done;
        leader->apply(task);
        done.wait();
        if (done.status().ok()) {
            ++i;
        } else {
            EXPECT_EQ(EPERM, done.status().error_code());
        }
    }
    arg.stop = true;
    pthread_join(tid, NULL);
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    melon::raft::SynchronizedClosure done;
    leader->change_peers(melon::raft::Configuration(peers), &done);
    done.wait();
    ASSERT_TRUE(done.status().ok()) << done.status();
    cluster.ensure_same();
    std::cout << "Stopping cluster" << std::endl;
    cluster.stop_all();
    google::SetCommandLineOption("raft_sync", "true");
    google::SetCommandLineOption("minloglevel", "1");
}

class AppendEntriesSyncClosure : public google::protobuf::Closure {
public:
    AppendEntriesSyncClosure() {
        _cntl = new melon::Controller;
    }
    ~AppendEntriesSyncClosure() {
        if (_cntl) {
            delete _cntl;
        }
    }
    void Run() {
        _event.signal();
    }
    void wait() {
        _event.wait();
    }
    melon::raft::AppendEntriesRequest& request() { return _request; }
    melon::raft::AppendEntriesResponse& response() { return _response; }
    melon::Controller& cntl() {
        return *_cntl;
    }

private:
    fiber::CountdownEvent _event;
    melon::raft::AppendEntriesRequest _request;
    melon::raft::AppendEntriesResponse _response;
    melon::Controller* _cntl;
};

void follower_append_entries(
        const melon::raft::AppendEntriesRequest& request_template, int entry_size,
        int64_t prev_log_index, AppendEntriesSyncClosure& closure, melon::raft::Node* node) {
    melon::raft::AppendEntriesRequest& request = closure.request();
    request.CopyFrom(request_template);
    request.set_prev_log_index(prev_log_index);
    for (int i = 0; i < entry_size; ++i) {
        melon::raft::EntryMeta em;
        mutil::IOBuf data;
        data.append("hello");
        em.set_data_len(data.size());
        em.set_type(melon::raft::ENTRY_TYPE_DATA);
        em.set_term(request_template.term());
        request.add_entries()->Swap(&em);
        closure.cntl().request_attachment().append(data);
    }
    node->_impl->handle_append_entries_request(
            &closure.cntl(), &closure.request(), &closure.response(), &closure);
}

TEST_P(NodeTest, follower_handle_out_of_order_append_entries) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers, 3000);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    cluster.ensure_same();

    std::vector<melon::raft::Node*> followers;
    cluster.followers(&followers);
    
    while (true) {
        followers[0]->_impl->_mutex.lock();
        int64_t local_index = followers[0]->_impl->_log_manager->last_log_index();
        followers[0]->_impl->_mutex.unlock();
        if (local_index == 0) {
            fiber_usleep(1000);
            continue;
        } else {
            break;
        }
    }

    followers[0]->_impl->_mutex.lock();
    int64_t local_index = followers[0]->_impl->_log_manager->last_log_index();
    int64_t term = followers[0]->_impl->_current_term;
    std::string group_id = followers[0]->_impl->_group_id;
    std::string server_id = followers[0]->_impl->_leader_id.to_string();
    std::string peer_id = "";
    int64_t committed_index = followers[0]->_impl->_ballot_box->last_committed_index();
    followers[0]->_impl->_mutex.unlock();
    int32_t max_append_entries_cache_size = melon::raft::FLAGS_raft_max_append_entries_cache_size;
    if (!melon::raft::FLAGS_raft_enable_append_entries_cache) {
        max_append_entries_cache_size = 0;
    }

    // Create a template
    melon::raft::AppendEntriesRequest request_template;
    request_template.set_term(term);
    request_template.set_group_id(group_id);
    request_template.set_server_id(server_id);
    request_template.set_peer_id(peer_id);
    // request_template.set_prev_log_index(local_index);
    request_template.set_prev_log_term(term);
    request_template.set_committed_index(committed_index);

    // Fill the entire cache
    std::deque<AppendEntriesSyncClosure*> out_of_order_closures;
    for (int32_t i = 0; i < max_append_entries_cache_size / 2; ++i) {
        out_of_order_closures.push_back(new AppendEntriesSyncClosure());
        AppendEntriesSyncClosure& closure = *out_of_order_closures.back();
        follower_append_entries(
                request_template, 1, local_index + 1 + i,
                closure, followers[0]);
    }
    for (int32_t i = max_append_entries_cache_size - 1; i >= max_append_entries_cache_size / 2; --i) {
        out_of_order_closures.push_back(new AppendEntriesSyncClosure());
        AppendEntriesSyncClosure& closure = *out_of_order_closures.back();
        follower_append_entries(
                request_template, 1, local_index + 1 + i,
                closure, followers[0]);
    }
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_log_manager->last_log_index(), local_index);
    ASSERT_TRUE(followers[0]->_impl->_append_entries_cache == NULL ||
                followers[0]->_impl->_append_entries_cache->_rpc_map.size() ==
                size_t(max_append_entries_cache_size));
    followers[0]->_impl->_mutex.unlock();

    // Fill another out-of-order request, be rejected
    AppendEntriesSyncClosure closure1;
    follower_append_entries(
            request_template, 1, local_index + 1 + max_append_entries_cache_size,
            closure1, followers[0]);
    closure1.wait();
    ASSERT_FALSE(closure1.response().success());

    // Let all out-of-order entries be handled
    AppendEntriesSyncClosure closure2;
    follower_append_entries(
            request_template, 1, local_index, closure2, followers[0]);
    closure2.wait();
    ASSERT_TRUE(closure2.response().success());
    for (auto& c : out_of_order_closures) {
        c->wait();
        ASSERT_TRUE(c->response().success());
    }
    out_of_order_closures.clear();
    local_index += max_append_entries_cache_size + 1;
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_log_manager->last_log_index(), local_index);
    ASSERT_TRUE(followers[0]->_impl->_append_entries_cache == NULL);
    followers[0]->_impl->_mutex.unlock();

    if (max_append_entries_cache_size <= 1) {
        LOG(WARNING) << "cluster stop";
        cluster.stop_all();
        return;
    }

    // Overlap out-of-order requests
    AppendEntriesSyncClosure closure3;
    follower_append_entries(
            request_template, 3, local_index + 5,
            closure3, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 1);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 5 + 1);
    followers[0]->_impl->_mutex.unlock();

    AppendEntriesSyncClosure closure4;
    follower_append_entries(
            request_template, 2, local_index + 5,
            closure4, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 1);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 5 + 1);
    followers[0]->_impl->_mutex.unlock();
    closure3.wait();
    ASSERT_FALSE(closure3.response().success());

    AppendEntriesSyncClosure closure5;
    follower_append_entries(
            request_template, 2, local_index + 6,
            closure5, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 1);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 6 + 1);
    followers[0]->_impl->_mutex.unlock();
    closure4.wait();
    ASSERT_FALSE(closure4.response().success());

    AppendEntriesSyncClosure closure6;
    follower_append_entries(
            request_template, 3, local_index + 4,
            closure6, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 1);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 4 + 1);
    followers[0]->_impl->_mutex.unlock();
    closure5.wait();
    ASSERT_FALSE(closure5.response().success());

    // Wait until timeout
    closure6.wait();
    ASSERT_FALSE(closure6.response().success());
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_log_manager->last_log_index(), local_index);
    ASSERT_TRUE(followers[0]->_impl->_append_entries_cache == NULL);
    followers[0]->_impl->_mutex.unlock();

    // Part of cache continuous
    AppendEntriesSyncClosure closure7;
    follower_append_entries(
            request_template, 3, local_index + 5,
            closure7, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 1);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 5 + 1);
    followers[0]->_impl->_mutex.unlock();

    AppendEntriesSyncClosure closure8;
    follower_append_entries(
            request_template, 2, local_index + 2,
            closure8, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 2);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 2 + 1);
    followers[0]->_impl->_mutex.unlock();

    AppendEntriesSyncClosure closure9;
    follower_append_entries(
            request_template, 1, local_index + 1,
            closure9, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 3);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 1 + 1);
    followers[0]->_impl->_mutex.unlock();

    AppendEntriesSyncClosure closure10;
    follower_append_entries(
            request_template, 1, local_index,
            closure10, followers[0]);
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->_rpc_map.size(), 1);
    ASSERT_EQ(followers[0]->_impl->_append_entries_cache->first_index(), local_index + 5 + 1);
    followers[0]->_impl->_mutex.unlock();
    
    closure10.wait();
    closure9.wait();
    closure8.wait();
    ASSERT_TRUE(closure10.response().success());
    ASSERT_TRUE(closure9.response().success());
    ASSERT_TRUE(closure8.response().success());
    local_index += 2 + 2;
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_log_manager->last_log_index(), local_index);
    followers[0]->_impl->_mutex.unlock();

    // Wait util timeout
    closure7.wait();
    ASSERT_FALSE(closure7.response().success());
    followers[0]->_impl->_mutex.lock();
    ASSERT_EQ(followers[0]->_impl->_log_manager->last_log_index(), local_index);
    ASSERT_TRUE(followers[0]->_impl->_append_entries_cache == NULL);
    followers[0]->_impl->_mutex.unlock();

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

TEST_P(NodeTest, readonly) {
    std::vector<melon::raft::PeerId> peers;
    for (int i = 0; i < 3; i++) {
        melon::raft::PeerId peer;
        peer.addr.ip = mutil::my_ip();
        peer.addr.port = 5006 + i;
        peer.idx = 0;

        peers.push_back(peer);
    }

    // start cluster
    Cluster cluster("unittest", peers);
    for (size_t i = 0; i < peers.size(); i++) {
        ASSERT_EQ(0, cluster.start(peers[i].addr));
    }

    // elect leader
    cluster.wait_leader();
    melon::raft::Node* leader = cluster.leader();
    ASSERT_TRUE(leader != NULL);
    LOG(WARNING) << "leader is " << leader->node_id();

    // apply something
    fiber::CountdownEvent cond(10);
    int start_index = 0;
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // let leader enter readonly mode, reject user logs
    leader->enter_readonly_mode();
    ASSERT_TRUE(leader->readonly());
    cond.reset(10);
    start_index += 10;
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, melon::raft::EREADONLY);
        leader->apply(task);
    }
    cond.wait();

    // let leader leave readonly mode, accept user logs
    leader->leave_readonly_mode();
    ASSERT_FALSE(leader->readonly());
    cond.reset(10);
    start_index += 10;
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();
  
    std::vector<melon::raft::Node*> followers;
    cluster.followers(&followers);
    ASSERT_EQ(2, followers.size());

    // Let follower 0 enter readonly mode, still can accept user logs
    followers[0]->enter_readonly_mode();
    fiber_usleep(2000 * 1000); // wait a while for heartbeat
    cond.reset(10);
    start_index += 10;
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();

    // Let follower 1 enter readonly mode, majority readonly, reject user logs
    followers[1]->enter_readonly_mode();
    int retry = 5;
    while (!leader->readonly() && --retry >= 0) {
        fiber_usleep(1000 * 1000);
    }
    ASSERT_TRUE(leader->readonly());
    cond.reset(10);
    start_index += 10;
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, melon::raft::EREADONLY);
        leader->apply(task);
    }
    cond.wait();  

    // Add a new follower
    melon::raft::PeerId peer3;
    peer3.addr.ip = mutil::my_ip();
    peer3.addr.port = 5006 + 3;
    peer3.idx = 0;
    ASSERT_EQ(0, cluster.start(peer3.addr, true));
    fiber_usleep(1000* 1000);
    cond.reset(1);
    leader->add_peer(peer3, NEW_ADDPEERCLOSURE(&cond, 0));
    cond.wait();

    // Trigger follower 0 do snapshot
    cond.reset(1);
    followers[0]->snapshot(NEW_SNAPSHOTCLOSURE(&cond, 0));
    cond.wait();

    // 2/4 readonly, leader still in readonly
    retry = 5;
    while (!leader->readonly() && --retry >= 0) {
        fiber_usleep(1000 * 1000);
    }
    ASSERT_TRUE(leader->readonly());
    start_index += 10;
    cond.reset(10);
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, melon::raft::EREADONLY);
        leader->apply(task);
    }
    cond.wait();  

    // Remove follower 0
    cond.reset(1);
    leader->remove_peer(followers[0]->node_id().peer_id, NEW_REMOVEPEERCLOSURE(&cond, 0));
    cond.wait();
    cluster.stop(followers[0]->node_id().peer_id.addr);

    // 1/3 readonly, leader leave Readonly
    retry = 5;
    while (leader->readonly() && --retry >= 0) {
        fiber_usleep(1000 * 1000);
    }
    ASSERT_TRUE(!leader->readonly());
    cond.reset(10);
    start_index += 10;
    for (int i = start_index; i < start_index + 10; i++) {
        mutil::IOBuf data;
        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello: %d", i);
        data.append(data_buf);

        melon::raft::Task task;
        task.data = &data;
        task.done = NEW_APPLYCLOSURE(&cond, 0);
        leader->apply(task);
    }
    cond.wait();  

    // Follower 1 leave readonly, catch up logs
    followers[1]->leave_readonly_mode();
    cluster.ensure_same();

    LOG(WARNING) << "cluster stop";
    cluster.stop_all();
}

INSTANTIATE_TEST_CASE_P(NodeTestWithoutPipelineReplication,
                        NodeTest,
                        ::testing::Values("NoReplcation"));

INSTANTIATE_TEST_CASE_P(NodeTestWithPipelineReplication,
                        NodeTest,
                        ::testing::Values("NoCache", "HasCache"));

int main(int argc, char* argv[]) {
    ::testing::AddGlobalTestEnvironment(new TestEnvironment());
    ::testing::InitGoogleTest(&argc, argv);
    google::SetCommandLineOption("minloglevel", "1");
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
