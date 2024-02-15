#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <melon/utility/status.h>
#include "melon/raft/raft.h"
#include "melon/raft/raft_meta.h"

namespace melon::raft {
extern void global_init_once_or_die();
};

class TestUsageSuits : public testing::Test {
protected:
    void SetUp() {
        melon::raft::global_init_once_or_die();
    }
    void TearDown() {}
};

TEST_F(TestUsageSuits, single_stable_storage) {
    system("rm -rf stable");
    melon::raft::FileBasedSingleMetaStorage* storage =
                        new melon::raft::FileBasedSingleMetaStorage("./stable");
    int64_t term;
    melon::raft::PeerId any_peer;
    mutil::Status st;
    // not init
    {   
        term = 10;
        melon::raft::PeerId candidate;
        ASSERT_EQ(0, candidate.parse("1.1.1.1:1000:0"));
        ASSERT_NE(0, candidate.parse("1.1.1.1,1000,0"));
        st = storage->set_term_and_votedfor(term, candidate, "");
        ASSERT_FALSE(st.ok());
        int64_t term_bak = 0;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, "");
        ASSERT_FALSE(st.ok());
        ASSERT_EQ(0, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
    }
 
    ASSERT_TRUE(storage->init().ok());
    ASSERT_TRUE(storage->init().ok());
    {
        term = 10;
        st = storage->set_term_and_votedfor(term, any_peer, "");
        ASSERT_TRUE(st.ok());
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, "");
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(10, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
        
        melon::raft::PeerId candidate;
        ASSERT_EQ(0, candidate.parse("1.1.1.1:1000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate, "").ok());
        ASSERT_TRUE(storage->
                    get_term_and_votedfor(&term_bak, &peer_bak, "").ok());
        ASSERT_EQ(peer_bak.addr, candidate.addr);
        ASSERT_EQ(peer_bak.idx, candidate.idx);
        
        term = 11;
        melon::raft::PeerId candidate2;
        ASSERT_EQ(0, candidate2.parse("2.2.2.2:2000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate2, "").ok());
    }
    delete storage;

    storage = new melon::raft::FileBasedSingleMetaStorage("./stable");
    ASSERT_TRUE(storage->init().ok());
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, "");
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);
        
        mutil::ip_t ip;
        mutil::str2ip("2.2.2.2", &ip);
        ASSERT_EQ(peer_bak.addr.ip, ip);
        ASSERT_EQ(peer_bak.addr.port, 2000);
        ASSERT_EQ(peer_bak.idx, 0);
    }
    delete storage;
}

TEST_F(TestUsageSuits, merged_stable_storage) {
    system("rm -rf merged_stable");
    melon::raft::KVBasedMergedMetaStorage* storage =
                    new melon::raft::KVBasedMergedMetaStorage("./merged_stable");
    // group_id = "pool_ssd_0", index = 0
    std::string v_group_id = "pool_ssd_0_0";
    int64_t term;
    melon::raft::PeerId any_peer;
    mutil::Status st;
    // not init
    {   
        term = 10;
        melon::raft::PeerId candidate;
        ASSERT_EQ(0, candidate.parse("1.1.1.1:1000:0"));
        ASSERT_NE(0, candidate.parse("1.1.1.1,1000,0"));
        st = storage->set_term_and_votedfor(term, candidate, v_group_id);
        ASSERT_FALSE(st.ok());
        int64_t term_bak = 0;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_FALSE(st.ok());
        ASSERT_EQ(0, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
    }
 
    ASSERT_TRUE(storage->init().ok());
    ASSERT_TRUE(storage->init().ok());
    {
        term = 10;
        st = storage->set_term_and_votedfor(term, any_peer, v_group_id);
        ASSERT_TRUE(st.ok());
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(10, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
        
        melon::raft::PeerId candidate;
        ASSERT_EQ(0, candidate.parse("1.1.1.1:1000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate, v_group_id).ok());
        ASSERT_TRUE(storage->
                    get_term_and_votedfor(&term_bak, &peer_bak, v_group_id).ok());
        ASSERT_EQ(peer_bak.addr, candidate.addr);
        ASSERT_EQ(peer_bak.idx, candidate.idx);
        
        term = 11;
        melon::raft::PeerId candidate2;
        ASSERT_EQ(0, candidate2.parse("2.2.2.2:2000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate2, v_group_id).ok());
    }
    delete storage;
 
    storage = new melon::raft::KVBasedMergedMetaStorage("./merged_stable");
    ASSERT_TRUE(storage->init().ok());
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);
        
        mutil::ip_t ip;
        mutil::str2ip("2.2.2.2", &ip);
        ASSERT_EQ(peer_bak.addr.ip, ip);
        ASSERT_EQ(peer_bak.addr.port, 2000);
        ASSERT_EQ(peer_bak.idx, 0);
    }
    delete storage;
}

TEST_F(TestUsageSuits, mixed_stable_storage_upgrade) {
    const std::string uri = "local://./disk1/replica_pool_ssd_0_0/stable";
    const std::string uri_mixed = "local-mixed://merged_path=./disk1/merged_stable"
                                  "&&single_path=./disk1/replica_pool_ssd_0_0/stable";
    const std::string uri_merged = "local-merged://./disk1/merged_stable";
    // group_id = "pool_ssd_0", index = 0
    std::string v_group_id = "pool_ssd_0_0";
    melon::raft::RaftMetaStorage::destroy(uri_merged, v_group_id);
    system("rm -rf ./disk1");
    
    // check init with only single_stable_storage  
    melon::raft::RaftMetaStorage* storage = melon::raft::RaftMetaStorage::create(uri);
    {
        ASSERT_TRUE(storage->init().ok());
        melon::raft::FileBasedSingleMetaStorage* tmp =
                        dynamic_cast<melon::raft::FileBasedSingleMetaStorage*>(storage);
        ASSERT_TRUE(tmp);
    }
    int64_t term;
    melon::raft::PeerId any_peer;
    mutil::Status st;
 
    // test default value
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(1, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
    }

    // test single stable storage alone
    {
        term = 10;
        st = storage->set_term_and_votedfor(term, any_peer, v_group_id);
        ASSERT_TRUE(st.ok());
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(10, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
        
        melon::raft::PeerId candidate;
        ASSERT_EQ(0, candidate.parse("1.1.1.1:1000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate, v_group_id).ok());
        ASSERT_TRUE(storage->
                    get_term_and_votedfor(&term_bak, &peer_bak, v_group_id).ok());
        ASSERT_EQ(peer_bak.addr, candidate.addr);
        ASSERT_EQ(peer_bak.idx, candidate.idx);
        
        term = 11;
        melon::raft::PeerId candidate2;
        ASSERT_EQ(0, candidate2.parse("2.2.2.2:2000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate2, v_group_id).ok());
    }
    delete storage;
    
    // test reload with only single stable storage
    storage = melon::raft::RaftMetaStorage::create(uri);
    ASSERT_TRUE(storage->init().ok());
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);
        
        mutil::ip_t ip;
        mutil::str2ip("2.2.2.2", &ip);
        ASSERT_EQ(peer_bak.addr.ip, ip);
        ASSERT_EQ(peer_bak.addr.port, 2000);
        ASSERT_EQ(peer_bak.idx, 0);
    }
    delete storage;

    // test upgrade stable storage from Single to Merged   
    // stage1: use mixed stable storage 

    // test init state with both 
    storage = melon::raft::RaftMetaStorage::create(uri_mixed);
    ASSERT_TRUE(storage->init().ok());
    melon::raft::MixedMetaStorage* tmp =
                        dynamic_cast<melon::raft::MixedMetaStorage*>(storage);
    ASSERT_TRUE(tmp);
    ASSERT_FALSE(tmp->is_bad());
    ASSERT_TRUE(tmp->_single_impl); 
    ASSERT_TRUE(tmp->_merged_impl);
    
    // test _merged_impl catch up data
    {
        // initial data of _merged_impl
        int64_t term_bak = 0;
        melon::raft::PeerId peer_bak;
        st = tmp->_merged_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(1, term_bak); 
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
    }
    {
        // _merged_impl catch up data when Mixed first load
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("2.2.2.2:2000:0:0", peer_bak.to_string());
    }
    {
        // _merged_impl already catch up data after Mixed first load
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = tmp->_merged_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("2.2.2.2:2000:0:0", peer_bak.to_string());
    }

    // test double write 
    {
        melon::raft::PeerId candidate3;
        term = 12;
        ASSERT_EQ(0, candidate3.parse("3.3.3.3:3000:3"));
        st = storage->set_term_and_votedfor(term, candidate3, v_group_id);
        ASSERT_TRUE(st.ok());

        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = tmp->_single_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("3.3.3.3:3000:3:0", peer_bak.to_string());
        
        term_bak = 0;
        peer_bak.reset();
        st = tmp->_merged_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("3.3.3.3:3000:3:0", peer_bak.to_string());
    }
    delete storage;


    // test change type of stable storage   
    // stage2: use merged stable storage   

    // test init state with only merged 
    storage = melon::raft::RaftMetaStorage::create(uri_merged);
    {
        ASSERT_TRUE(storage->init().ok());
        melon::raft::KVBasedMergedMetaStorage* tmp =
                        dynamic_cast<melon::raft::KVBasedMergedMetaStorage*>(storage);
        ASSERT_TRUE(tmp);
    }
    
    // test reload with only merged stable storage
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("3.3.3.3:3000:3:0", peer_bak.to_string());
    }
    // test merged stable storage alone 
    {
        melon::raft::PeerId candidate4;
        term = 13;
        ASSERT_EQ(0, candidate4.parse("4.4.4.4:4000:4"));
        st = storage->set_term_and_votedfor(term, candidate4, v_group_id);
        ASSERT_TRUE(st.ok());

        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("4.4.4.4:4000:4:0", peer_bak.to_string());
    }
    delete storage; 
}

TEST_F(TestUsageSuits, mixed_stable_storage_downgrade) {
    const std::string uri_single = "local://./disk1/replica_pool_ssd_0_0/stable";
    const std::string uri_mixed = "local-mixed://merged_path=./disk1/merged_stable"
                                  "&&single_path=./disk1/replica_pool_ssd_0_0/stable";
    const std::string uri_merged = "local-merged://./disk1/merged_stable";
    // group_id = "pool_ssd_0", index = 0
    std::string v_group_id = "pool_ssd_0_0";
    melon::raft::RaftMetaStorage::destroy(uri_merged, v_group_id);
    system("rm -rf ./disk1");
    
    // check init with only merged_stable_storage  
    melon::raft::RaftMetaStorage* storage = melon::raft::RaftMetaStorage::create(uri_merged);
    {
        ASSERT_TRUE(storage->init().ok());
        melon::raft::KVBasedMergedMetaStorage* tmp =
                        dynamic_cast<melon::raft::KVBasedMergedMetaStorage*>(storage);
        ASSERT_TRUE(tmp);
    }
    int64_t term;
    melon::raft::PeerId any_peer;
    mutil::Status st;

    // test default value
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(1, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
    }

    // test merged stable storage alone
    {
        term = 10;
        st = storage->set_term_and_votedfor(term, any_peer, v_group_id);
        ASSERT_TRUE(st.ok());
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(10, term_bak);
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
        
        melon::raft::PeerId candidate;
        ASSERT_EQ(0, candidate.parse("1.1.1.1:1000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate, v_group_id).ok());
        ASSERT_TRUE(storage->
                    get_term_and_votedfor(&term_bak, &peer_bak, v_group_id).ok());
        ASSERT_EQ(peer_bak.addr, candidate.addr);
        ASSERT_EQ(peer_bak.idx, candidate.idx);
        
        term = 11;
        melon::raft::PeerId candidate2;
        ASSERT_EQ(0, candidate2.parse("2.2.2.2:2000:0"));
        ASSERT_TRUE(storage->
                    set_term_and_votedfor(term, candidate2, v_group_id).ok());
    }
    delete storage;
    
    // test reload with only merged stable storage
    storage = melon::raft::RaftMetaStorage::create(uri_merged);
    ASSERT_TRUE(storage->init().ok());
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);
        
        mutil::ip_t ip;
        mutil::str2ip("2.2.2.2", &ip);
        ASSERT_EQ(peer_bak.addr.ip, ip);
        ASSERT_EQ(peer_bak.addr.port, 2000);
        ASSERT_EQ(peer_bak.idx, 0);
    }
    delete storage;

    // test downgrade stable storage from Merged to Single   
    // stage1: use mixed stable storage 

    // test init state with both 
    storage = melon::raft::RaftMetaStorage::create(uri_mixed);
    ASSERT_TRUE(storage->init().ok());
    melon::raft::MixedMetaStorage* tmp =
                        dynamic_cast<melon::raft::MixedMetaStorage*>(storage);
    ASSERT_TRUE(tmp);
    ASSERT_FALSE(tmp->is_bad());
    ASSERT_TRUE(tmp->_single_impl); 
    ASSERT_TRUE(tmp->_merged_impl);
    
    // test _single_impl catch up data
    {
        // initial data of _single_impl
        int64_t term_bak = 0;
        melon::raft::PeerId peer_bak;
        st = tmp->_single_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(1, term_bak); 
        ASSERT_EQ(melon::raft::ANY_PEER, peer_bak);
    }
    {
        // _single_impl catch up data when Mixed first load
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("2.2.2.2:2000:0:0", peer_bak.to_string());
    }
    {
        // _single_impl already catch up data after Mixed first load
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = tmp->_single_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("2.2.2.2:2000:0:0", peer_bak.to_string());
    }

    // test double write 
    {
        melon::raft::PeerId candidate3;
        term = 12;
        ASSERT_EQ(0, candidate3.parse("3.3.3.3:3000:3"));
        st = storage->set_term_and_votedfor(term, candidate3, v_group_id);
        ASSERT_TRUE(st.ok());

        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = tmp->_single_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("3.3.3.3:3000:3:0", peer_bak.to_string());
        
        term_bak = 0;
        peer_bak.reset();
        st = tmp->_merged_impl->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("3.3.3.3:3000:3:0", peer_bak.to_string());
    }
    delete storage;


    // test change type of stable storage   
    // stage2: use single stable storage   

    // test init state with only single
    storage = melon::raft::RaftMetaStorage::create(uri_single);
    {
        ASSERT_TRUE(storage->init().ok());
        melon::raft::FileBasedSingleMetaStorage* tmp =
                        dynamic_cast<melon::raft::FileBasedSingleMetaStorage*>(storage);
        ASSERT_TRUE(tmp);
    }
    
    // test reload with only single stable storage
    {
        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("3.3.3.3:3000:3:0", peer_bak.to_string());
    }
    // test single stable storage alone 
    {
        melon::raft::PeerId candidate4;
        term = 13;
        ASSERT_EQ(0, candidate4.parse("4.4.4.4:4000:4"));
        st = storage->set_term_and_votedfor(term, candidate4, v_group_id);
        ASSERT_TRUE(st.ok());

        int64_t term_bak;
        melon::raft::PeerId peer_bak;
        st = storage->get_term_and_votedfor(&term_bak, &peer_bak, v_group_id);
        ASSERT_TRUE(st.ok());
        ASSERT_EQ(term, term_bak);  
        ASSERT_EQ("4.4.4.4:4000:4:0", peer_bak.to_string());
    }
    delete storage; 
}
