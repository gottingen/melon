// Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved


#include <algorithm>
#include <gtest/gtest.h>
#include <melon/utility/string_printf.h>
#include "melon/raft/ballot_box.h"
#include "melon/raft/configuration.h"
#include "melon/raft/fsm_caller.h"

class BallotBoxTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

void benchmark_vector_set(int num_peers) {
    std::set<melon::raft::PeerId> peer_set;
    std::vector<melon::raft::PeerId> peer_vector;
    for (int i = 0; i < num_peers; ++i) {
        std::string peer_desc;
        mutil::string_printf(&peer_desc, "192.168.1.%d:9876", i);
        melon::raft::PeerId peer(peer_desc);
        peer_set.insert(peer);
        peer_vector.push_back(peer);
    }
    std::vector<melon::raft::PeerId> find_list(peer_vector);
    std::random_shuffle(find_list.begin(), find_list.end());
    const size_t N = 100000;
    size_t counter = 0;
    mutil::Timer timer;
    timer.start();
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < find_list.size(); ++j) {
            std::vector<melon::raft::PeerId>::iterator it;
            for (it = peer_vector.begin(); 
                    it < peer_vector.end() && *it != find_list[j]; ++it) {}
            counter += (it != peer_vector.end());
        }
    }
    timer.stop();
    const long elp_vector = timer.n_elapsed();
    ASSERT_EQ(counter, N * num_peers);
    counter = 0;
    timer.start();
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < find_list.size(); ++j) {
            //std::find is slower on small vector
            counter += std::find(peer_vector.begin(), peer_vector.end(),
                                find_list[j]) != peer_vector.end();
        }
    }
    timer.stop();
    ASSERT_EQ(counter, N * num_peers);
    const long elp_vector_std_find = timer.n_elapsed();
    counter = 0;
    timer.start();
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < find_list.size(); ++j) {
            counter += peer_set.find(find_list[j]) != peer_set.end();
        }
    }
    timer.stop();
    ASSERT_EQ(counter, N * num_peers);
    const long elp_set = timer.n_elapsed();
    LOG(INFO) << "num_peers=" << num_peers
              << " vector=" << elp_vector / counter
              << " vector_std_find=" << elp_vector_std_find / counter
              << " set=" << elp_set / counter;
}

TEST_F(BallotBoxTest, benchmark_vector_set) {
    for (int i = 1; i < 30; ++i) {
        benchmark_vector_set(i);
    }
}

class DummyCaller : public melon::raft::FSMCaller {
public:
    DummyCaller() : _committed_index(0) {}
    virtual int on_committed(int64_t committed_index) { 
        _committed_index = committed_index; 
        return 0;
    }
    int64_t committed_index() const { return _committed_index; }
private:
    int64_t _committed_index;
};

TEST_F(BallotBoxTest, odd_cluster) {
    DummyCaller caller;
    melon::raft::ClosureQueue cq(false);
    melon::raft::BallotBoxOptions opt;
    opt.waiter = &caller;
    opt.closure_queue = &cq;
    melon::raft::BallotBox cm;
    ASSERT_EQ(0, cm.init(opt));
    ASSERT_EQ(0, cm.reset_pending_index(1));
    std::vector<melon::raft::PeerId> peers;
    for (int i = 1; i <= 3; ++i) {
        std::string peer_addr;
        mutil::string_printf(&peer_addr, "192.168.1.%d:8888", i);
        peers.push_back(melon::raft::PeerId(peer_addr));
    }
    melon::raft::Configuration conf(peers);
    const int num_tasks = 10000;
    for (int i = 0; i < num_tasks; ++i) {
        ASSERT_EQ(0, cm.append_pending_task(conf, NULL, NULL));
    }

    ASSERT_EQ(0, cm.commit_at(1, 100, peers[0]));
    ASSERT_EQ(0, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 100, peers[0]));
    ASSERT_EQ(0, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 50, peers[1]));
    ASSERT_EQ(50, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 100, peers[2]));
    ASSERT_EQ(100, caller.committed_index());
    ASSERT_NE(0, cm.commit_at(
                        num_tasks + 100, num_tasks + 100, peers[0]));
}

TEST_F(BallotBoxTest, even_cluster) {
    DummyCaller caller;
    melon::raft::ClosureQueue cq(false);
    melon::raft::BallotBoxOptions opt;
    opt.waiter = &caller;
    opt.closure_queue = &cq;
    melon::raft::BallotBox cm;
    ASSERT_EQ(0, cm.init(opt));
    ASSERT_EQ(0, cm.reset_pending_index(1));
    std::vector<melon::raft::PeerId> peers;
    for (int i = 1; i <= 4; ++i) {
        std::string peer_addr;
        mutil::string_printf(&peer_addr, "192.168.1.%d:8888", i);
        peers.push_back(melon::raft::PeerId(peer_addr));
    }
    melon::raft::Configuration conf(peers);
    const int num_tasks = 10000;
    for (int i = 0; i < num_tasks; ++i) {
        ASSERT_EQ(0, cm.append_pending_task(conf, NULL, NULL));
    }

    ASSERT_EQ(0, cm.commit_at(1, 100, peers[0]));
    ASSERT_EQ(0, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 100, peers[0]));
    ASSERT_EQ(0, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 50, peers[1]));
    ASSERT_EQ(0, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 100, peers[2]));
    ASSERT_EQ(50, caller.committed_index());
    ASSERT_EQ(0, cm.commit_at(1, 100, peers[3]));
    ASSERT_EQ(100, caller.committed_index());
}

