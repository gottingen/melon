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


#include <pthread.h>
#include <unistd.h>
#include <melon/utility/string_printf.h>
#include <melon/base/class_name.h>
#include <melon/raft/raft.h>
#include <melon/raft/node.h>
#include <melon/raft/storage.h>
#include <melon/raft/node_manager.h>
#include <melon/raft/log.h>
#include <melon/raft/memory_log.h>
#include <melon/raft/raft_meta.h>
#include <melon/raft/snapshot.h>
#include <melon/raft/fsm_caller.h>            // IteratorImpl

namespace melon::raft {

    static void print_revision(std::ostream &os, void *) {
#if defined(BRAFT_REVISION)
        os << BRAFT_REVISION;
#else
        os << "undefined";
#endif
    }

    static melon::var::PassiveStatus<std::string> s_raft_revision(
            "raft_revision", print_revision, NULL);


    static pthread_once_t global_init_once = PTHREAD_ONCE_INIT;

    struct GlobalExtension {
        SegmentLogStorage local_log;
        MemoryLogStorage memory_log;

        // manage only one raft instance
        FileBasedSingleMetaStorage single_meta;
        // manage a batch of raft instances
        KVBasedMergedMetaStorage merged_meta;
        // mix two types for double write when upgrade and downgrade
        MixedMetaStorage mixed_meta;

        LocalSnapshotStorage local_snapshot;
    };

    static void global_init_or_die_impl() {
        static GlobalExtension s_ext;

        log_storage_extension()->RegisterOrDie("local", &s_ext.local_log);
        log_storage_extension()->RegisterOrDie("memory", &s_ext.memory_log);

        // uri = local://{single_path}
        // |single_path| usually ends with `/meta'
        // NOTICE: not change "local" to "local-single" because of compatibility
        meta_storage_extension()->RegisterOrDie("local", &s_ext.single_meta);
        // uri = local-merged://{merged_path}
        // |merged_path| usually ends with `/merged_meta'
        meta_storage_extension()->RegisterOrDie("local-merged", &s_ext.merged_meta);
        // uri = local-mixed://merged_path={merged_path}&&single_path={single_path}
        meta_storage_extension()->RegisterOrDie("local-mixed", &s_ext.mixed_meta);

        snapshot_storage_extension()->RegisterOrDie("local", &s_ext.local_snapshot);
    }

// Non-static for unit test
    void global_init_once_or_die() {
        if (pthread_once(&global_init_once, global_init_or_die_impl) != 0) {
            PLOG(FATAL) << "Fail to pthread_once";
            exit(-1);
        }
    }

    int add_service(melon::Server *server, const mutil::EndPoint &listen_addr) {
        global_init_once_or_die();
        return global_node_manager->add_service(server, listen_addr);
    }

    int add_service(melon::Server *server, int port) {
        mutil::EndPoint addr(mutil::IP_ANY, port);
        return add_service(server, addr);
    }

    int add_service(melon::Server *server, const char *listen_ip_and_port) {
        mutil::EndPoint addr;
        if (mutil::str2endpoint(listen_ip_and_port, &addr) != 0) {
            LOG(ERROR) << "Fail to parse `" << listen_ip_and_port << "'";
            return -1;
        }
        return add_service(server, addr);
    }

// GC 
    int gc_raft_data(const GCOptions &gc_options) {
        const VersionedGroupId vgid = gc_options.vgid;
        const std::string log_uri = gc_options.log_uri;
        const std::string raft_meta_uri = gc_options.raft_meta_uri;
        const std::string snapshot_uri = gc_options.snapshot_uri;
        bool is_success = true;

        mutil::Status status = LogStorage::destroy(log_uri);
        if (!status.ok()) {
            is_success = false;
            LOG(WARNING) << "Group " << vgid << " failed to gc raft log, uri " << log_uri;
        }
        // TODO encode vgid into raft_meta_uri ?
        status = RaftMetaStorage::destroy(raft_meta_uri, vgid);
        if (!status.ok()) {
            is_success = false;
            LOG(WARNING) << "Group " << vgid << " failed to gc raft stable, uri " << raft_meta_uri;
        }
        status = SnapshotStorage::destroy(snapshot_uri);
        if (!status.ok()) {
            is_success = false;
            LOG(WARNING) << "Group " << vgid << " failed to gc raft snapshot, uri " << snapshot_uri;
        }
        return is_success ? 0 : -1;
    }

// ------------- Node
    Node::Node(const GroupId &group_id, const PeerId &peer_id) {
        _impl = new NodeImpl(group_id, peer_id);
    }

    Node::~Node() {
        if (_impl) {
            _impl->shutdown(NULL);
            _impl->join();
            _impl->Release();
            _impl = NULL;
        }
    }

    NodeId Node::node_id() {
        return _impl->node_id();
    }

    PeerId Node::leader_id() {
        return _impl->leader_id();
    }

    bool Node::is_leader() {
        return _impl->is_leader();
    }

    bool Node::is_leader_lease_valid() {
        return _impl->is_leader_lease_valid();
    }

    void Node::get_leader_lease_status(LeaderLeaseStatus *status) {
        return _impl->get_leader_lease_status(status);
    }

    int Node::init(const NodeOptions &options) {
        return _impl->init(options);
    }

    void Node::shutdown(Closure *done) {
        _impl->shutdown(done);
    }

    void Node::join() {
        _impl->join();
    }

    void Node::apply(const Task &task) {
        _impl->apply(task);
    }

    mutil::Status Node::list_peers(std::vector<PeerId> *peers) {
        return _impl->list_peers(peers);
    }

    void Node::add_peer(const PeerId &peer, Closure *done) {
        _impl->add_peer(peer, done);
    }

    void Node::remove_peer(const PeerId &peer, Closure *done) {
        _impl->remove_peer(peer, done);
    }

    void Node::change_peers(const Configuration &new_peers, Closure *done) {
        _impl->change_peers(new_peers, done);
    }

    mutil::Status Node::reset_peers(const Configuration &new_peers) {
        return _impl->reset_peers(new_peers);
    }

    void Node::snapshot(Closure *done) {
        _impl->snapshot(done);
    }

    mutil::Status Node::vote(int election_timeout) {
        return _impl->vote(election_timeout);
    }

    mutil::Status Node::reset_election_timeout_ms(int election_timeout_ms) {
        return _impl->reset_election_timeout_ms(election_timeout_ms);
    }

    void Node::reset_election_timeout_ms(int election_timeout_ms, int max_clock_drift_ms) {
        _impl->reset_election_timeout_ms(election_timeout_ms, max_clock_drift_ms);
    }

    int Node::transfer_leadership_to(const PeerId &peer) {
        return _impl->transfer_leadership_to(peer);
    }

    mutil::Status Node::read_committed_user_log(const int64_t index, UserLog *user_log) {
        return _impl->read_committed_user_log(index, user_log);
    }

    void Node::get_status(NodeStatus *status) {
        return _impl->get_status(status);
    }

    void Node::enter_readonly_mode() {
        return _impl->enter_readonly_mode();
    }

    void Node::leave_readonly_mode() {
        return _impl->leave_readonly_mode();
    }

    bool Node::readonly() {
        return _impl->readonly();
    }

// ------------- Iterator
    void Iterator::next() {
        if (valid()) {
            _impl->next();
        }
    }

    bool Iterator::valid() const {
        return _impl->is_good() && _impl->entry()->type == ENTRY_TYPE_DATA;
    }

    int64_t Iterator::index() const { return _impl->index(); }

    int64_t Iterator::term() const { return _impl->entry()->id.term; }

    const mutil::IOBuf &Iterator::data() const {
        return _impl->entry()->data;
    }

    Closure *Iterator::done() const {
        return _impl->done();
    }

    void Iterator::set_error_and_rollback(size_t ntail, const mutil::Status *st) {
        return _impl->set_error_and_rollback(ntail, st);
    }

// ------------- Default Implementation of StateMachine
    StateMachine::~StateMachine() {}

    void StateMachine::on_shutdown() {}

    void StateMachine::on_snapshot_save(SnapshotWriter *writer, Closure *done) {
        (void) writer;
        CHECK(done);
        LOG(ERROR) << mutil::class_name_str(*this)
                   << " didn't implement on_snapshot_save";
        done->status().set_error(-1, "%s didn't implement on_snapshot_save",
                                 mutil::class_name_str(*this).c_str());
        done->Run();
    }

    int StateMachine::on_snapshot_load(SnapshotReader *reader) {
        (void) reader;
        LOG(ERROR) << mutil::class_name_str(*this)
                   << " didn't implement on_snapshot_load"
                   << " while a snapshot is saved in " << reader->get_path();
        return -1;
    }

    void StateMachine::on_leader_start(int64_t) {}

    void StateMachine::on_leader_stop(const mutil::Status &) {}

    void StateMachine::on_error(const Error &e) {
        LOG(ERROR) << "Encountered an error=" << e << " on StateMachine "
                   << mutil::class_name_str(*this)
                   << ", it's highly recommended to implement this interface"
                      " as raft stops working since some error ocurrs,"
                      " you should figure out the cause and repair or remove this node";
    }

    void StateMachine::on_configuration_committed(const Configuration &conf) {
        (void) conf;
        return;
    }

    void StateMachine::on_configuration_committed(const Configuration &conf, int64_t index) {
        (void) index;
        return on_configuration_committed(conf);
    }

    void StateMachine::on_stop_following(const LeaderChangeContext &) {}

    void StateMachine::on_start_following(const LeaderChangeContext &) {}

    BootstrapOptions::BootstrapOptions()
            : last_log_index(0), fsm(NULL), node_owns_fsm(false), usercode_in_pthread(false) {}

    int bootstrap(const BootstrapOptions &options) {
        global_init_once_or_die();
        NodeImpl *node = new NodeImpl();
        const int rc = node->bootstrap(options);
        node->shutdown(NULL);
        node->join();
        node->Release();  // node acquired an additional reference in ctro.
        return rc;
    }

}
