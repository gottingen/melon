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



#include "melon/raft/cli.h"

#include <melon/rpc/channel.h>          // melon::Channel
#include <melon/rpc/controller.h>       // melon::Controller
#include "melon/proto/raft/cli.pb.h"                // CliService_Stub
#include "melon/raft/util.h"

namespace melon::raft {
    namespace cli {

        static mutil::Status get_leader(const GroupId &group_id, const Configuration &conf,
                                        PeerId *leader_id) {
            if (conf.empty()) {
                return mutil::Status(EINVAL, "Empty group configuration");
            }
            // Construct a melon naming service to access all the nodes in this group
            mutil::Status st(-1, "Fail to get leader of group %s", group_id.c_str());
            leader_id->reset();
            for (Configuration::const_iterator
                         iter = conf.begin(); iter != conf.end(); ++iter) {
                melon::Channel channel;
                if (channel.Init(iter->addr, NULL) != 0) {
                    return mutil::Status(-1, "Fail to init channel to %s",
                                         iter->to_string().c_str());
                }
                CliService_Stub stub(&channel);
                GetLeaderRequest request;
                GetLeaderResponse response;
                melon::Controller cntl;
                request.set_group_id(group_id);
                request.set_peer_id(iter->to_string());
                stub.get_leader(&cntl, &request, &response, NULL);
                if (cntl.Failed()) {
                    if (st.ok()) {
                        st.set_error(cntl.ErrorCode(), "[%s] %s",
                                     mutil::endpoint2str(cntl.remote_side()).c_str(),
                                     cntl.ErrorText().c_str());
                    } else {
                        std::string saved_et = st.error_str();
                        st.set_error(cntl.ErrorCode(), "%s, [%s] %s", saved_et.c_str(),
                                     mutil::endpoint2str(cntl.remote_side()).c_str(),
                                     cntl.ErrorText().c_str());
                    }
                    continue;
                }
                leader_id->parse(response.leader_id());
            }
            if (leader_id->is_empty()) {
                return st;
            }
            return mutil::Status::OK();
        }

        mutil::Status add_peer(const GroupId &group_id, const Configuration &conf,
                               const PeerId &peer_id, const CliOptions &options) {
            PeerId leader_id;
            mutil::Status st = get_leader(group_id, conf, &leader_id);
            BRAFT_RETURN_IF(!st.ok(), st);
            melon::Channel channel;
            if (channel.Init(leader_id.addr, NULL) != 0) {
                return mutil::Status(-1, "Fail to init channel to %s",
                                     leader_id.to_string().c_str());
            }
            AddPeerRequest request;
            request.set_group_id(group_id);
            request.set_leader_id(leader_id.to_string());
            request.set_peer_id(peer_id.to_string());
            AddPeerResponse response;
            melon::Controller cntl;
            cntl.set_timeout_ms(options.timeout_ms);
            cntl.set_max_retry(options.max_retry);

            CliService_Stub stub(&channel);
            stub.add_peer(&cntl, &request, &response, NULL);
            if (cntl.Failed()) {
                return mutil::Status(cntl.ErrorCode(), cntl.ErrorText());
            }
            Configuration old_conf;
            for (int i = 0; i < response.old_peers_size(); ++i) {
                old_conf.add_peer(response.old_peers(i));
            }
            Configuration new_conf;
            for (int i = 0; i < response.new_peers_size(); ++i) {
                new_conf.add_peer(response.new_peers(i));
            }
            MLOG(INFO) << "Configuration of replication group `" << group_id
                      << "' changed from " << old_conf
                      << " to " << new_conf;
            return mutil::Status::OK();
        }

        mutil::Status remove_peer(const GroupId &group_id, const Configuration &conf,
                                  const PeerId &peer_id, const CliOptions &options) {
            PeerId leader_id;
            mutil::Status st = get_leader(group_id, conf, &leader_id);
            BRAFT_RETURN_IF(!st.ok(), st);
            melon::Channel channel;
            if (channel.Init(leader_id.addr, NULL) != 0) {
                return mutil::Status(-1, "Fail to init channel to %s",
                                     leader_id.to_string().c_str());
            }
            RemovePeerRequest request;
            request.set_group_id(group_id);
            request.set_leader_id(leader_id.to_string());
            request.set_peer_id(peer_id.to_string());
            RemovePeerResponse response;
            melon::Controller cntl;
            cntl.set_timeout_ms(options.timeout_ms);
            cntl.set_max_retry(options.max_retry);

            CliService_Stub stub(&channel);
            stub.remove_peer(&cntl, &request, &response, NULL);
            if (cntl.Failed()) {
                return mutil::Status(cntl.ErrorCode(), cntl.ErrorText());
            }
            Configuration old_conf;
            for (int i = 0; i < response.old_peers_size(); ++i) {
                old_conf.add_peer(response.old_peers(i));
            }
            Configuration new_conf;
            for (int i = 0; i < response.new_peers_size(); ++i) {
                new_conf.add_peer(response.new_peers(i));
            }
            MLOG(INFO) << "Configuration of replication group `" << group_id
                      << "' changed from " << old_conf
                      << " to " << new_conf;
            return mutil::Status::OK();
        }

        mutil::Status reset_peer(const GroupId &group_id, const PeerId &peer_id,
                                 const Configuration &new_conf,
                                 const CliOptions &options) {
            if (new_conf.empty()) {
                return mutil::Status(EINVAL, "new_conf is empty");
            }
            melon::Channel channel;
            if (channel.Init(peer_id.addr, NULL) != 0) {
                return mutil::Status(-1, "Fail to init channel to %s",
                                     peer_id.to_string().c_str());
            }
            melon::Controller cntl;
            cntl.set_timeout_ms(options.timeout_ms);
            cntl.set_max_retry(options.max_retry);
            ResetPeerRequest request;
            request.set_group_id(group_id);
            request.set_peer_id(peer_id.to_string());
            for (Configuration::const_iterator
                         iter = new_conf.begin(); iter != new_conf.end(); ++iter) {
                request.add_new_peers(iter->to_string());
            }
            ResetPeerResponse response;
            CliService_Stub stub(&channel);
            stub.reset_peer(&cntl, &request, &response, NULL);
            if (cntl.Failed()) {
                return mutil::Status(cntl.ErrorCode(), cntl.ErrorText());
            }
            return mutil::Status::OK();
        }

        mutil::Status snapshot(const GroupId &group_id, const PeerId &peer_id,
                               const CliOptions &options) {
            melon::Channel channel;
            if (channel.Init(peer_id.addr, NULL) != 0) {
                return mutil::Status(-1, "Fail to init channel to %s",
                                     peer_id.to_string().c_str());
            }
            melon::Controller cntl;
            cntl.set_timeout_ms(options.timeout_ms);
            cntl.set_max_retry(options.max_retry);
            SnapshotRequest request;
            request.set_group_id(group_id);
            request.set_peer_id(peer_id.to_string());
            SnapshotResponse response;
            CliService_Stub stub(&channel);
            stub.snapshot(&cntl, &request, &response, NULL);
            if (cntl.Failed()) {
                return mutil::Status(cntl.ErrorCode(), cntl.ErrorText());
            }
            return mutil::Status::OK();
        }

        mutil::Status change_peers(const GroupId &group_id, const Configuration &conf,
                                   const Configuration &new_peers,
                                   const CliOptions &options) {
            PeerId leader_id;
            mutil::Status st = get_leader(group_id, conf, &leader_id);
            BRAFT_RETURN_IF(!st.ok(), st);
            MLOG(INFO) << "conf=" << conf << " leader=" << leader_id
                      << " new_peers=" << new_peers;
            melon::Channel channel;
            if (channel.Init(leader_id.addr, NULL) != 0) {
                return mutil::Status(-1, "Fail to init channel to %s",
                                     leader_id.to_string().c_str());
            }

            ChangePeersRequest request;
            request.set_group_id(group_id);
            request.set_leader_id(leader_id.to_string());
            for (Configuration::const_iterator
                         iter = new_peers.begin(); iter != new_peers.end(); ++iter) {
                request.add_new_peers(iter->to_string());
            }
            ChangePeersResponse response;
            melon::Controller cntl;
            cntl.set_timeout_ms(options.timeout_ms);
            cntl.set_max_retry(options.max_retry);

            CliService_Stub stub(&channel);
            stub.change_peers(&cntl, &request, &response, NULL);
            if (cntl.Failed()) {
                return mutil::Status(cntl.ErrorCode(), cntl.ErrorText());
            }
            Configuration old_conf;
            for (int i = 0; i < response.old_peers_size(); ++i) {
                old_conf.add_peer(response.old_peers(i));
            }
            Configuration new_conf;
            for (int i = 0; i < response.new_peers_size(); ++i) {
                new_conf.add_peer(response.new_peers(i));
            }
            MLOG(INFO) << "Configuration of replication group `" << group_id
                      << "' changed from " << old_conf
                      << " to " << new_conf;
            return mutil::Status::OK();
        }

        mutil::Status transfer_leader(const GroupId &group_id, const Configuration &conf,
                                      const PeerId &peer, const CliOptions &options) {
            PeerId leader_id;
            mutil::Status st = get_leader(group_id, conf, &leader_id);
            BRAFT_RETURN_IF(!st.ok(), st);
            if (leader_id == peer) {
                MLOG(INFO) << "peer " << peer << " is already the leader";
                return mutil::Status::OK();
            }
            melon::Channel channel;
            if (channel.Init(leader_id.addr, NULL) != 0) {
                return mutil::Status(-1, "Fail to init channel to %s",
                                     leader_id.to_string().c_str());
            }
            TransferLeaderRequest request;
            request.set_group_id(group_id);
            request.set_leader_id(leader_id.to_string());
            if (!peer.is_empty()) {
                request.set_peer_id(peer.to_string());
            }
            TransferLeaderResponse response;
            melon::Controller cntl;
            cntl.set_timeout_ms(options.timeout_ms);
            cntl.set_max_retry(options.max_retry);
            CliService_Stub stub(&channel);
            stub.transfer_leader(&cntl, &request, &response, NULL);
            if (cntl.Failed()) {
                return mutil::Status(cntl.ErrorCode(), cntl.ErrorText());
            }
            return mutil::Status::OK();
        }

    }  // namespace cli
}  //  namespace melon::raft
