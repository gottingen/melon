// Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved



#include <gtest/gtest.h>
#include <melon/utility/string_printf.h>
#include <melon/utility/memory/scoped_ptr.h>
#include "melon/raft/fsm_caller.h"
#include "melon/raft/raft.h"
#include "melon/raft/log.h"
#include "melon/raft/configuration.h"
#include "melon/raft/log_manager.h"
#include <cinttypes>

class FSMCallerTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

class OrderedStateMachine : public melon::raft::StateMachine {
public:
    OrderedStateMachine() 
        : _expected_next(0)
        , _stopped(false)
        , _on_leader_start_times(0)
        , _on_leader_stop_times(0)
        , _on_snapshot_save_times(0)
        , _on_snapshot_load_times(0)
    {}
    void on_apply(melon::raft::Iterator& iter) {
        for (; iter.valid(); iter.next()) {
            std::string expected;
            mutil::string_printf(&expected, "hello_%" PRIu64, _expected_next++);
            ASSERT_EQ(expected, iter.data().to_string());
            if (iter.done()) {
                ASSERT_TRUE(iter.done()->status().ok()) << "index=" << iter.index();
                iter.done()->Run();
            }
        }
    }
    void on_shutdown() {
        _stopped = true;
    }
    void on_snapshot_save(melon::raft::SnapshotWriter* /*writer*/, melon::raft::Closure* done) {
        done->Run();
        ++_on_snapshot_save_times;
    }
    int on_snapshot_load(melon::raft::SnapshotReader* /*reader*/) {
        ++_on_snapshot_load_times;
        return 0;
    }
    void on_leader_start(int64_t term) {
        _on_leader_start_times++;
    }
    virtual void on_leader_stop(const mutil::Status& status) {
        _on_leader_stop_times++;
    }
    void join() {
        while (!_stopped) {
            fiber_usleep(100);
        }
    }
private:
    uint64_t _expected_next;
    bool _stopped;
    int _on_leader_start_times;
    int _on_leader_stop_times;
    int _on_snapshot_save_times;
    int _on_snapshot_load_times;
};

class SyncClosure : public melon::raft::LogManager::StableClosure {
public:
    SyncClosure() {
        _butex = fiber::butex_create_checked<mutil::atomic<int> >();
        *_butex = 0;
    }
    ~SyncClosure() {
        fiber::butex_destroy(_butex);
    }
    void Run() {
        _butex->store(1);
        fiber::butex_wake(_butex);
    }
    void reset() {
        status().reset();
        *_butex = 0;
    }
    void join() {
        while (*_butex != 1) {
            fiber::butex_wait(_butex, 0, NULL);
        }
    }
private:
    mutil::atomic<int> *_butex;
};

TEST_F(FSMCallerTest, sanity) {
    system("rm -rf ./data");
    scoped_ptr<melon::raft::ConfigurationManager> cm(
                                new melon::raft::ConfigurationManager);
    scoped_ptr<melon::raft::SegmentLogStorage> storage(
                                new melon::raft::SegmentLogStorage("./data"));
    scoped_ptr<melon::raft::LogManager> lm(new melon::raft::LogManager());
    melon::raft::LogManagerOptions log_opt;
    log_opt.log_storage = storage.get();
    log_opt.configuration_manager = cm.get();
    ASSERT_EQ(0, lm->init(log_opt));

    melon::raft::ClosureQueue cq(false);

    OrderedStateMachine fsm;
    fsm._expected_next = 0;

    melon::raft::FSMCallerOptions opt;
    opt.log_manager = lm.get();
    opt.after_shutdown = NULL;
    opt.fsm = &fsm;
    opt.closure_queue = &cq;

    melon::raft::FSMCaller caller;
    ASSERT_EQ(0, caller.init(opt));

    const size_t N = 1000;

    for (size_t i = 0; i < N; ++i) {
        std::vector<melon::raft::LogEntry*> entries;
        melon::raft::LogEntry* entry = new melon::raft::LogEntry;
        entry->AddRef();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        std::string buf;
        mutil::string_printf(&buf, "hello_%lld", (long long)i);
        entry->data.append(buf);
        entry->id.index = i + 1;
        entry->id.term = i;
        entries.push_back(entry);
        SyncClosure c;
        lm->append_entries(&entries, &c);
        c.join();
        ASSERT_TRUE(c.status().ok()) << c.status();
    }
    ASSERT_EQ(0, caller.on_committed(N));
    ASSERT_EQ(0, caller.shutdown());
    fsm.join();
    ASSERT_EQ(fsm._expected_next, N);
}

TEST_F(FSMCallerTest, on_leader_start_and_stop) {
    scoped_ptr<melon::raft::LogManager> lm(new melon::raft::LogManager());
    OrderedStateMachine fsm;
    fsm._expected_next = 0;
    melon::raft::ClosureQueue cq(false);
    melon::raft::FSMCallerOptions opt;
    opt.log_manager = lm.get();
    opt.after_shutdown = NULL;
    opt.fsm = &fsm;
    opt.closure_queue = &cq;
    melon::raft::FSMCaller caller;
    ASSERT_EQ(0, caller.init(opt));
    mutil::Status status;
    caller.on_leader_stop(status);
    caller.shutdown();
    fsm.join();
    ASSERT_EQ(0, fsm._on_leader_start_times);
    ASSERT_EQ(1, fsm._on_leader_stop_times);
}

class DummySnapshotReader : public melon::raft::SnapshotReader {
public:
    DummySnapshotReader(melon::raft::SnapshotMeta* meta)
        : _meta(meta)
    {
    };
    ~DummySnapshotReader() {}
    std::string generate_uri_for_copy() { return std::string(); }
    void list_files(std::vector<std::string>*) {}
    int get_file_meta(const std::string&, google::protobuf::Message*) { return 0; }
    std::string get_path() { return std::string(); }
    int load_meta(melon::raft::SnapshotMeta* meta) {
        *meta = *_meta;
        return 0;
    }
private:
    melon::raft::SnapshotMeta* _meta;
};

class DummySnapshoWriter : public melon::raft::SnapshotWriter {
public:
    DummySnapshoWriter() {}
    ~DummySnapshoWriter() {}
    int save_meta(const melon::raft::SnapshotMeta&) {
        EXPECT_TRUE(false) << "Should never be called";
        return 0;
    }
    std::string get_path() { return std::string(); }
    int add_file(const std::string&, const google::protobuf::Message*) { return 0;}
    int remove_file(const std::string&) { return 0; }
    void list_files(std::vector<std::string>*) {}
    int get_file_meta(const std::string&, google::protobuf::Message*) { return 0; }
private:
};

class MockSaveSnapshotClosure : public melon::raft::SaveSnapshotClosure {
public:
    MockSaveSnapshotClosure(melon::raft::SnapshotWriter* writer,
                            melon::raft::SnapshotMeta *expected_meta)
        : _start_times(0)
        , _writer(writer)
        , _expected_meta(expected_meta)
    {
    }
    ~MockSaveSnapshotClosure() {}
    void Run() {
        ASSERT_TRUE(status().ok()) << status();
    }
    melon::raft::SnapshotWriter* start(const melon::raft::SnapshotMeta& meta) {
        EXPECT_EQ(meta.last_included_index(), 
                    _expected_meta->last_included_index());
        EXPECT_EQ(meta.last_included_term(), 
                    _expected_meta->last_included_term());
        ++_start_times;
        return _writer;
    }
private:
    int _start_times;
    melon::raft::SnapshotWriter* _writer;
    melon::raft::SnapshotMeta* _expected_meta;
};

class MockLoadSnapshotClosure : public melon::raft::LoadSnapshotClosure {
public:
    MockLoadSnapshotClosure(melon::raft::SnapshotReader* reader)
        : _start_times(0)
        , _reader(reader)
    {}
    ~MockLoadSnapshotClosure() {}
    void Run() {
        ASSERT_TRUE(status().ok()) << status();
    }
    melon::raft::SnapshotReader* start() {
        ++_start_times;
        return _reader;
    }
private:
    int _start_times;
    melon::raft::SnapshotReader* _reader;
};

TEST_F(FSMCallerTest, snapshot) {
    melon::raft::SnapshotMeta snapshot_meta;
    snapshot_meta.set_last_included_index(0);
    snapshot_meta.set_last_included_term(0);
    DummySnapshotReader dummy_reader(&snapshot_meta);
    DummySnapshoWriter dummy_writer;
    MockSaveSnapshotClosure save_snapshot_done(&dummy_writer, &snapshot_meta);
    system("rm -rf ./data");
    scoped_ptr<melon::raft::ConfigurationManager> cm(
                                new melon::raft::ConfigurationManager);
    scoped_ptr<melon::raft::SegmentLogStorage> storage(
                                new melon::raft::SegmentLogStorage("./data"));
    scoped_ptr<melon::raft::LogManager> lm(new melon::raft::LogManager());
    melon::raft::LogManagerOptions log_opt;
    log_opt.log_storage = storage.get();
    log_opt.configuration_manager = cm.get();
    ASSERT_EQ(0, lm->init(log_opt));

    OrderedStateMachine fsm;
    fsm._expected_next = 0;
    melon::raft::ClosureQueue cq(false);
    melon::raft::FSMCallerOptions opt;
    opt.log_manager = lm.get();
    opt.after_shutdown = NULL;
    opt.fsm = &fsm;
    opt.closure_queue = &cq;
    melon::raft::FSMCaller caller;
    ASSERT_EQ(0, caller.init(opt));
    ASSERT_EQ(0, caller.on_snapshot_save(&save_snapshot_done));
    MockLoadSnapshotClosure load_snapshot_done(&dummy_reader);
    ASSERT_EQ(0, caller.on_snapshot_load(&load_snapshot_done));
    ASSERT_EQ(0, caller.shutdown());
    fsm.join();
    ASSERT_EQ(1, fsm._on_snapshot_save_times);
    ASSERT_EQ(1, fsm._on_snapshot_load_times);
    ASSERT_EQ(1, save_snapshot_done._start_times);
    ASSERT_EQ(1, load_snapshot_done._start_times);
}

