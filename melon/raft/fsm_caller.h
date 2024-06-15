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

#include <melon/utility/macros.h>                        // MELON_CACHELINE_ALIGNMENT
#include <melon/fiber/fiber.h>
#include <melon/fiber/execution_queue.h>
#include <melon/raft/ballot_box.h>
#include <melon/raft/closure_queue.h>
#include <melon/raft/macros.h>
#include <melon/raft/log_entry.h>
#include <melon/raft/lease.h>

namespace melon::raft {

    class NodeImpl;

    class LogManager;

    class StateMachine;

    class SnapshotMeta;

    class OnErrorClousre;

    struct LogEntry;

    class LeaderChangeContext;

// Backing implementation of Iterator
    class IteratorImpl {
        DISALLOW_COPY_AND_ASSIGN(IteratorImpl);

    public:
        // Move to the next
        void next();

        LogEntry *entry() const { return _cur_entry; }

        bool is_good() const { return _cur_index <= _committed_index && !has_error(); }

        Closure *done() const;

        void set_error_and_rollback(size_t ntail, const mutil::Status *st);

        bool has_error() const { return _error.type() != ERROR_TYPE_NONE; }

        const Error &error() const { return _error; }

        int64_t index() const { return _cur_index; }

        void run_the_rest_closure_with_error();

    private:
        IteratorImpl(StateMachine *sm, LogManager *lm,
                     std::vector<Closure *> *closure,
                     int64_t first_closure_index,
                     int64_t last_applied_index,
                     int64_t committed_index,
                     std::atomic<int64_t> *applying_index);

        ~IteratorImpl() {}

        friend class FSMCaller;

        StateMachine *_sm;
        LogManager *_lm;
        std::vector<Closure *> *_closure;
        int64_t _first_closure_index;
        int64_t _cur_index;
        int64_t _committed_index;
        LogEntry *_cur_entry;
        std::atomic<int64_t> *_applying_index;
        Error _error;
    };

    struct FSMCallerOptions {
        FSMCallerOptions()
                : log_manager(NULL), fsm(NULL), after_shutdown(NULL), closure_queue(NULL), node(NULL),
                  usercode_in_pthread(false), bootstrap_id() {}

        LogManager *log_manager;
        StateMachine *fsm;
        google::protobuf::Closure *after_shutdown;
        ClosureQueue *closure_queue;
        NodeImpl *node;
        bool usercode_in_pthread;
        LogId bootstrap_id;
    };

    class SaveSnapshotClosure : public Closure {
    public:
        // TODO: comments
        virtual SnapshotWriter *start(const SnapshotMeta &meta) = 0;
    };

    class LoadSnapshotClosure : public Closure {
    public:
        // TODO: comments
        virtual SnapshotReader *start() = 0;
    };

    class MELON_CACHELINE_ALIGNMENT FSMCaller {
    public:
        FSMCaller();

        BRAFT_MOCK ~FSMCaller();

        int init(const FSMCallerOptions &options);

        int shutdown();

        BRAFT_MOCK int on_committed(int64_t committed_index);

        BRAFT_MOCK int on_snapshot_load(LoadSnapshotClosure *done);

        BRAFT_MOCK int on_snapshot_save(SaveSnapshotClosure *done);

        int on_leader_stop(const mutil::Status &status);

        int on_leader_start(int64_t term, int64_t lease_epoch);

        int on_start_following(const LeaderChangeContext &start_following_context);

        int on_stop_following(const LeaderChangeContext &stop_following_context);

        BRAFT_MOCK int on_error(const Error &e);

        int64_t last_applied_index() const {
            return _last_applied_index.load(std::memory_order_relaxed);
        }

        int64_t applying_index() const;

        void describe(std::ostream &os, bool use_html);

        void join();

    private:

        friend class IteratorImpl;

        enum TaskType {
            IDLE,
            COMMITTED,
            SNAPSHOT_SAVE,
            SNAPSHOT_LOAD,
            LEADER_STOP,
            LEADER_START,
            START_FOLLOWING,
            STOP_FOLLOWING,
            ERROR,
        };

        struct LeaderStartContext {
            LeaderStartContext(int64_t term_, int64_t lease_epoch_)
                    : term(term_), lease_epoch(lease_epoch_) {}

            int64_t term;
            int64_t lease_epoch;
        };

        struct ApplyTask {
            TaskType type;
            union {
                // For applying log entry (including configuration change)
                int64_t committed_index;

                // For on_leader_start
                LeaderStartContext *leader_start_context;

                // For on_leader_stop
                mutil::Status *status;

                // For on_start_following and on_stop_following
                LeaderChangeContext *leader_change_context;

                // For other operation
                Closure *done;
            };
        };

        static double get_cumulated_cpu_time(void *arg);

        static int run(void *meta, fiber::TaskIterator<ApplyTask> &iter);

        void do_shutdown(); //Closure* done);
        void do_committed(int64_t committed_index);

        void do_cleared(int64_t log_index, Closure *done, int error_code);

        void do_snapshot_save(SaveSnapshotClosure *done);

        void do_snapshot_load(LoadSnapshotClosure *done);

        void do_on_error(OnErrorClousre *done);

        void do_leader_stop(const mutil::Status &status);

        void do_leader_start(const LeaderStartContext &leader_start_context);

        void do_start_following(const LeaderChangeContext &start_following_context);

        void do_stop_following(const LeaderChangeContext &stop_following_context);

        void set_error(const Error &e);

        bool pass_by_status(Closure *done);

        fiber::ExecutionQueueId<ApplyTask> _queue_id;
        LogManager *_log_manager;
        StateMachine *_fsm;
        ClosureQueue *_closure_queue;
        std::atomic<int64_t> _last_applied_index;
        int64_t _last_applied_term;
        google::protobuf::Closure *_after_shutdown;
        NodeImpl *_node;
        TaskType _cur_task;
        std::atomic<int64_t> _applying_index;
        Error _error;
        bool _queue_started;
    };

};
