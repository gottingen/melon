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

#pragma once

#include <melon/rpc/controller.h>

#include <melon/raft/raft.h>
#include <melon/raft/util.h>
#include <melon/raft/snapshot.h>
#include <melon/raft/storage.h>
#include <melon/proto/raft/raft.pb.h>
#include <melon/raft/fsm_caller.h>
#include <melon/raft/log_manager.h>

namespace melon::raft {
    class NodeImpl;

    class SnapshotHook;

    class FileSystemAdaptor;

    struct SnapshotExecutorOptions {
        SnapshotExecutorOptions();

        // URI of SnapshotStorage
        std::string uri;

        FSMCaller *fsm_caller;
        NodeImpl *node;
        LogManager *log_manager;
        int64_t init_term;
        mutil::EndPoint addr;
        bool filter_before_copy_remote;
        bool usercode_in_pthread;
        bool copy_file = true;
        scoped_refptr<FileSystemAdaptor> file_system_adaptor;
        scoped_refptr<SnapshotThrottle> snapshot_throttle;
    };

// Executing Snapshot related stuff
    class MELON_CACHELINE_ALIGNMENT SnapshotExecutor {
        DISALLOW_COPY_AND_ASSIGN(SnapshotExecutor);

    public:
        SnapshotExecutor();

        ~SnapshotExecutor();

        int init(const SnapshotExecutorOptions &options);

        // Return the owner NodeImpl
        NodeImpl *node() const { return _node; }

        // Start to snapshot StateMachine, and |done| is called after the execution
        // finishes or fails.
        void do_snapshot(Closure *done);

        // Install snapshot according to the very RPC from leader
        // After the installing succeeds (StateMachine is reset with the snapshot)
        // or fails, done will be called to respond
        //
        // Errors:
        //  - Term dismatches: which happens interrupt_downloading_snapshot was
        //    called before install_snapshot, indicating that this RPC was issued by
        //    the old leader.
        //  - Interrupted: happens when interrupt_downloading_snapshot is called or
        //    a new RPC with the same or newer snapshot arrives
        //  - Busy: the state machine is saving or loading snapshot
        void install_snapshot(melon::Controller *controller,
                              const InstallSnapshotRequest *request,
                              InstallSnapshotResponse *response,
                              google::protobuf::Closure *done);

        // Interrupt the downloading if possible.
        // This is called when the term of node increased to |new_term|, which
        // happens when receiving RPC from new peer. In this case, it's hard to
        // determine whether to keep downloading snapshot as the new leader
        // possibly contains the missing logs and is going to send AppendEntries. To
        // make things simplicity and leader changing during snapshot installing is
        // very rare. So we interrupt snapshot downloading when leader changes, and
        // let the new leader decide whether to install a new snapshot or continue
        // appending log entries.
        //
        // NOTE: we can't interrupt the snapshot insalling which has finsihed
        // downloading and is reseting the State Machine.
        void interrupt_downloading_snapshot(int64_t new_term);

        // Return true if this is currently installing a snapshot, either
        // downloading or loading.
        bool is_installing_snapshot() const {
            // 1: acquire fence makes this thread sees the latest change when seeing
            //    the lastest _loading_snapshot
            // _downloading_snapshot is NULL when then downloading was successfully
            // interrupted or installing has finished
            return _downloading_snapshot.load(mutil::memory_order_acquire/*1*/);
        }

        // Return the backing snapshot storage
        SnapshotStorage *snapshot_storage() { return _snapshot_storage; }

        void describe(std::ostream &os, bool use_html);

        // Shutdown the SnapshotExecutor and all the following jobs would be refused
        void shutdown();

        // Block the current thread until all the running job finishes (including
        // failure)
        void join();

    private:
        friend class SaveSnapshotDone;

        friend class FirstSnapshotLoadDone;

        friend class InstallSnapshotDone;

        void on_snapshot_load_done(const mutil::Status &st);

        int on_snapshot_save_done(const mutil::Status &st,
                                  const SnapshotMeta &meta,
                                  SnapshotWriter *writer);

        struct DownloadingSnapshot {
            const InstallSnapshotRequest *request;
            InstallSnapshotResponse *response;
            melon::Controller *cntl;
            google::protobuf::Closure *done;
        };

        int register_downloading_snapshot(DownloadingSnapshot *ds);

        int parse_install_snapshot_request(
                const InstallSnapshotRequest *request,
                SnapshotMeta *meta);

        void load_downloading_snapshot(DownloadingSnapshot *ds,
                                       const SnapshotMeta &meta);

        void report_error(int error_code, const char *fmt, ...);

        raft_mutex_t _mutex;
        int64_t _last_snapshot_term;
        int64_t _last_snapshot_index;
        int64_t _term;
        bool _saving_snapshot;
        bool _loading_snapshot;
        bool _stopped;
        bool _usercode_in_pthread;
        SnapshotStorage *_snapshot_storage;
        SnapshotCopier *_cur_copier;
        FSMCaller *_fsm_caller;
        NodeImpl *_node;
        LogManager *_log_manager;
        // The ownership of _downloading_snapshot is a little messy:
        // - Before we start to replace the FSM with the downloaded one. The
        //   ownership belongs with the downloding thread
        // - After we push the load task to FSMCaller, the ownership belongs to the
        //   closure which is called after the Snapshot replaces FSM
        mutil::atomic<DownloadingSnapshot *> _downloading_snapshot;
        SnapshotMeta _loading_snapshot_meta;
        fiber::CountdownEvent _running_jobs;
        scoped_refptr<SnapshotThrottle> _snapshot_throttle;
    };

    inline SnapshotExecutorOptions::SnapshotExecutorOptions()
            : fsm_caller(NULL), node(NULL), log_manager(NULL), init_term(0), filter_before_copy_remote(false),
              usercode_in_pthread(false) {}

}  //  namespace melon::raft
