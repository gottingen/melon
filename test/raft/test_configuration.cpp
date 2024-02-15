/*
 * =====================================================================================
 *
 *       Filename:  test_configuration.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年10月22日 15时16分31秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <melon/utility/logging.h>
#include "melon/raft/raft.h"
#include "melon/raft/configuration_manager.h"

class TestUsageSuits : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(TestUsageSuits, PeerId) {
    melon::raft::PeerId id1;
    ASSERT_TRUE(id1.is_empty());

    ASSERT_NE(0, id1.parse("1.1.1.1::"));
    ASSERT_TRUE(id1.is_empty());

    ASSERT_EQ(0, id1.parse("1.1.1.1:1000:"));
    LOG(INFO) << "id:" << id1.to_string();
    LOG(INFO) << "id:" << id1;

    ASSERT_EQ(0, id1.parse("1.1.1.1:1000:0"));
    LOG(INFO) << "id:" << id1.to_string();
    LOG(INFO) << "id:" << id1;

    ASSERT_EQ(0, id1.parse("1.1.1.1:1000:0:0"));
    LOG(INFO) << "id:" << id1.to_string();
    LOG(INFO) << "id:" << id1;
    ASSERT_FALSE(id1.is_witness());

    ASSERT_EQ(0, id1.parse("1.1.1.1:1000:0:1"));
    LOG(INFO) << "id:" << id1.to_string();
    LOG(INFO) << "id:" << id1;
    ASSERT_TRUE(id1.is_witness());

    ASSERT_EQ(-1, id1.parse("1.1.1.1:1000:0:2"));

    ASSERT_EQ(0, id1.parse("1.1.1.1:1000"));
    LOG(INFO) << "id:" << id1.to_string();
    LOG(INFO) << "id:" << id1;

    melon::raft::PeerId id2(id1);
    LOG(INFO) << "id:" << id2;

    melon::raft::PeerId id3("1.2.3.4:1000:0");
    LOG(INFO) << "id:" << id3;
}

TEST_F(TestUsageSuits, Configuration) {
    melon::raft::Configuration conf1;
    ASSERT_TRUE(conf1.empty());
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:0"));
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:1"));
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:2"));
    conf1 = peers;
    LOG(INFO) << conf1;

    ASSERT_TRUE(conf1.contains(melon::raft::PeerId("1.1.1.1:1000:0")));
    ASSERT_FALSE(conf1.contains(melon::raft::PeerId("1.1.1.1:2000:0")));

    std::vector<melon::raft::PeerId> peers2;
    peers2.push_back(melon::raft::PeerId("1.1.1.1:1000:0"));
    peers2.push_back(melon::raft::PeerId("1.1.1.1:1000:1"));
    ASSERT_TRUE(conf1.contains(peers2));
    peers2.push_back(melon::raft::PeerId("1.1.1.1:2000:1"));
    ASSERT_FALSE(conf1.contains(peers2));

    ASSERT_FALSE(conf1.equals(peers2));
    ASSERT_TRUE(conf1.equals(peers));

    melon::raft::Configuration conf2(peers);
    conf2.remove_peer(melon::raft::PeerId("1.1.1.1:1000:1"));
    conf2.add_peer(melon::raft::PeerId("1.1.1.1:1000:3"));
    ASSERT_FALSE(conf2.contains(melon::raft::PeerId("1.1.1.1:1000:1")));
    ASSERT_TRUE(conf2.contains(melon::raft::PeerId("1.1.1.1:1000:3")));

    std::set<melon::raft::PeerId> peer_set;
    conf2.list_peers(&peer_set);
    ASSERT_EQ(peer_set.size(), 3);
    std::vector<melon::raft::PeerId> peer_vector;
    conf2.list_peers(&peer_vector);
    ASSERT_EQ(peer_vector.size(), 3);
}

TEST_F(TestUsageSuits, ConfigurationManager) {
    melon::raft::ConfigurationManager conf_manager;

    melon::raft::ConfigurationEntry it1;
    conf_manager.get(10, &it1);
    ASSERT_EQ(it1.id, melon::raft::LogId(0, 0));
    ASSERT_TRUE(it1.conf.empty());
    ASSERT_EQ(melon::raft::LogId(0, 0), conf_manager.last_configuration().id);
    melon::raft::ConfigurationEntry entry;
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:0"));
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:1"));
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:2"));
    entry.conf = peers;
    entry.id = melon::raft::LogId(8, 1);
    conf_manager.add(entry);
    ASSERT_EQ(melon::raft::LogId(8, 1), conf_manager.last_configuration().id);

    conf_manager.get(10, &it1);
    ASSERT_EQ(it1.id, entry.id);

    conf_manager.truncate_suffix(7);
    ASSERT_EQ(melon::raft::LogId(0, 0), conf_manager.last_configuration().id);

    entry.id = melon::raft::LogId(10, 1);
    entry.conf = peers;
    conf_manager.add(entry);
    peers.push_back(melon::raft::PeerId("1.1.1.1:1000:3"));
    entry.id = melon::raft::LogId(20, 1);
    entry.conf = peers;
    conf_manager.add(entry);
    ASSERT_EQ(melon::raft::LogId(20, 1), conf_manager.last_configuration().id);

    conf_manager.truncate_prefix(15);
    ASSERT_EQ(melon::raft::LogId(20, 1), conf_manager.last_configuration().id);

    conf_manager.truncate_prefix(25);
    ASSERT_EQ(melon::raft::LogId(0, 0), conf_manager.last_configuration().id);

}
