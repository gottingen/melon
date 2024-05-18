// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/basictypes.h"
#include "melon/utility/logging.h"
#include "melon/utility/gperftools_profiler.h"
#include "melon/utility/files/temp_file.h"
#include "melon/utility/popen.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

#if !MELON_WITH_GLOG

namespace logging {
DECLARE_bool(crash_on_fatal_log);
DECLARE_int32(v);
DECLARE_bool(log_func_name);
DECLARE_bool(async_log);

namespace {

// Needs to be global since log assert handlers can't maintain state.
int log_sink_call_count = 0;

#if !defined(OFFICIAL_BUILD) || defined(DMCHECK_ALWAYS_ON) || !defined(NDEBUG)
void LogSink(const std::string& str) {
  ++log_sink_call_count;
}
#endif

// Class to make sure any manipulations we do to the min log level are
// contained (i.e., do not affect other unit tests).
class LogStateSaver {
 public:
  LogStateSaver() : old_min_log_level_(GetMinLogLevel()) {}

  ~LogStateSaver() {
    SetMinLogLevel(old_min_log_level_);
    SetLogAssertHandler(NULL);
    log_sink_call_count = 0;
  }

 private:
  int old_min_log_level_;

  DISALLOW_COPY_AND_ASSIGN(LogStateSaver);
};

class LoggingTest : public testing::Test {
public:
    virtual void SetUp() {
        _old_crash_on_fatal_log = ::logging::FLAGS_crash_on_fatal_log;
        ::logging::FLAGS_crash_on_fatal_log = true;
    }
    virtual void TearDown() {
        ::logging::FLAGS_crash_on_fatal_log = _old_crash_on_fatal_log;
        if (::logging::FLAGS_v != 0) {
            // Clear -verbose to avoid affecting other tests.
            ASSERT_FALSE(google::SetCommandLineOption("v", "0").empty());
            ASSERT_FALSE(google::SetCommandLineOption("vmodule", "").empty());
        }
    }
private:
  bool _old_crash_on_fatal_log;
  LogStateSaver log_state_saver_;
};

TEST_F(LoggingTest, LogIsOn) {
#if defined(NDEBUG)
  const bool kDfatalIsFatal = false;
#else  // defined(NDEBUG)
  const bool kDfatalIsFatal = true;
#endif  // defined(NDEBUG)

  SetMinLogLevel(BLOG_INFO);
  EXPECT_TRUE(LOG_IS_ON(INFO));
  EXPECT_TRUE(LOG_IS_ON(WARNING));
  EXPECT_TRUE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(FATAL));
  EXPECT_TRUE(LOG_IS_ON(DFATAL));

  SetMinLogLevel(BLOG_WARNING);
  EXPECT_FALSE(LOG_IS_ON(INFO));
  EXPECT_TRUE(LOG_IS_ON(WARNING));
  EXPECT_TRUE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(FATAL));
  EXPECT_TRUE(LOG_IS_ON(DFATAL));

  SetMinLogLevel(BLOG_ERROR);
  EXPECT_FALSE(LOG_IS_ON(INFO));
  EXPECT_FALSE(LOG_IS_ON(WARNING));
  EXPECT_TRUE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(FATAL));
  EXPECT_TRUE(LOG_IS_ON(DFATAL));

  // LOG_IS_ON(FATAL) should always be true.
  SetMinLogLevel(BLOG_FATAL + 1);
  EXPECT_FALSE(LOG_IS_ON(INFO));
  EXPECT_FALSE(LOG_IS_ON(WARNING));
  EXPECT_FALSE(LOG_IS_ON(ERROR));
  EXPECT_TRUE(LOG_IS_ON(FATAL));
  EXPECT_TRUE(kDfatalIsFatal == LOG_IS_ON(DFATAL));
}

TEST_F(LoggingTest, DebugLoggingReleaseBehavior) {
#if !defined(NDEBUG)
  int debug_only_variable = 1;
#endif
  // These should avoid emitting references to |debug_only_variable|
  // in release mode.
  DMLOG_IF(INFO, debug_only_variable) << "test";
  DMLOG_ASSERT(debug_only_variable) << "test";
  DPMLOG_IF(INFO, debug_only_variable) << "test";
  DVMLOG_IF(1, debug_only_variable) << "test";
}

TEST_F(LoggingTest, Dcheck) {
#if defined(NDEBUG) && !defined(DMCHECK_ALWAYS_ON)
  // Release build.
  EXPECT_FALSE(DMCHECK_IS_ON());
  EXPECT_FALSE(DMLOG_IS_ON(DMCHECK));
#elif defined(NDEBUG) && defined(DMCHECK_ALWAYS_ON)
  // Release build with real DCHECKS.
  SetLogAssertHandler(&LogSink);
  EXPECT_TRUE(DMCHECK_IS_ON());
  EXPECT_FALSE(DMLOG_IS_ON(DMCHECK));
#else
  // Debug build.
  SetLogAssertHandler(&LogSink);
  EXPECT_TRUE(DMCHECK_IS_ON());
  EXPECT_TRUE(DMLOG_IS_ON(DMCHECK));
#endif

  EXPECT_EQ(0, log_sink_call_count);
  DMCHECK(false);
  EXPECT_EQ(DMCHECK_IS_ON() ? 1 : 0, log_sink_call_count);
  DPMCHECK(false);
  EXPECT_EQ(DMCHECK_IS_ON() ? 2 : 0, log_sink_call_count);
  DMCHECK_EQ(0, 1);
  EXPECT_EQ(DMCHECK_IS_ON() ? 3 : 0, log_sink_call_count);
}

TEST_F(LoggingTest, DcheckReleaseBehavior) {
  int some_variable = 1;
  // These should still reference |some_variable| so we don't get
  // unused variable warnings.
  DMCHECK(some_variable) << "test";
  DPMCHECK(some_variable) << "test";
  DMCHECK_EQ(some_variable, 1) << "test";
}

TEST_F(LoggingTest, streaming_log_sanity) {
    ::logging::FLAGS_crash_on_fatal_log = false;

    MLOG(WARNING) << 1 << 1.1f << 2l << "apple" << noflush;
    MLOG(WARNING) << " orange" << noflush;
    ASSERT_EQ("11.12apple orange", LOG_STREAM(WARNING).content_str());
    ASSERT_EQ("", LOG_STREAM(WARNING).content_str());
    
    MLOG(FATAL) << 1 << 1.1f << 2l << "apple" << noflush;
    MLOG(FATAL) << " orange" << noflush;
    ASSERT_EQ("11.12apple orange", LOG_STREAM(FATAL).content_str());
    ASSERT_EQ("", LOG_STREAM(FATAL).content_str());

    MLOG(TRACE) << 1 << 1.1f << 2l << "apple" << noflush;
    MLOG(TRACE) << " orange" << noflush;
    ASSERT_EQ("11.12apple orange", LOG_STREAM(TRACE).content_str());
    ASSERT_EQ("", LOG_STREAM(TRACE).content_str());
    
    MLOG(NOTICE) << 1 << 1.1f << 2l << "apple" << noflush;
    MLOG(DEBUG) << 1 << 1.1f << 2l << "apple" << noflush;

    MLOG(FATAL) << 1 << 1.1f << 2l << "apple";
    MLOG(ERROR) << 1 << 1.1f << 2l << "apple";
    MLOG(WARNING) << 1 << 1.1f << 2l << "apple";
    MLOG(INFO) << 1 << 1.1f << 2l << "apple";
    MLOG(TRACE) << 1 << 1.1f << 2l << "apple";
    MLOG(NOTICE) << 2 << 2.2f << 3l << "orange" << noflush;
    ASSERT_EQ("11.12apple22.23orange", LOG_STREAM(NOTICE).content_str());
    MLOG(DEBUG) << 1 << 1.1f << 2l << "apple";

    errno = EINVAL;
    PMLOG(FATAL) << "Error occurred" << noflush;
    ASSERT_EQ("Error occurred: Invalid argument", PMLOG_STREAM(FATAL).content_str());
    
    errno = 0;
    PMLOG(FATAL) << "Error occurred" << noflush;
#if defined(OS_LINUX)
    ASSERT_EQ("Error occurred: Success", PMLOG_STREAM(FATAL).content_str());
#else
    ASSERT_EQ("Error occurred: Undefined error: 0", PMLOG_STREAM(FATAL).content_str());
#endif

    errno = EINTR;
    PMLOG(FATAL) << "Error occurred" << noflush;
    ASSERT_EQ("Error occurred: Interrupted system call",
              PMLOG_STREAM(FATAL).content_str());
}

TEST_F(LoggingTest, log_at) {
    ::logging::StringSink log_str;
    ::logging::LogSink* old_sink = ::logging::SetLogSink(&log_str);
    MLOG_AT(WARNING, "specified_file.cc", 12345) << "file/line is specified";
    // the file:line part should be using the argument given by us.
    ASSERT_NE(std::string::npos, log_str.find("specified_file.cc:12345"));
    // restore the old sink.
    ::logging::SetLogSink(old_sink);
}

#define VMLOG_NE(verbose_level) VMLOG(verbose_level) << noflush

#define VMLOG2_NE(virtual_path, verbose_level)           \
    VMLOG2(virtual_path, verbose_level) << noflush

TEST_F(LoggingTest, vlog_sanity) {
    ::logging::FLAGS_crash_on_fatal_log = false;

    EXPECT_FALSE(google::SetCommandLineOption("v", "1").empty());
    
    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
                                               "logging_unittest=1").empty());
    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
                                               "logging_UNITTEST=2").empty());

    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("vlog 1vlog 2", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());
    
    VMLOG_NE(-1) << "nothing";
    EXPECT_EQ("", LOG_STREAM(VERBOSE).content_str());

    // VMLOG(0) is MLOG(INFO)
    VMLOG_NE(0) << "always on";
    EXPECT_EQ("always on", LOG_STREAM(INFO).content_str());

    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
                                              "logging_unittest=0").empty());
    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());

    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
                     "logging_unittest=0,logging_unittest=1").empty());
    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("vlog 1", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());

    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
                     "logging_unittest=1,logging_unittest=0").empty());
    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());

    EXPECT_FALSE(google::SetCommandLineOption("vmodule", "").empty());
    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("vlog 1", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());

    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
                                               "logg?ng_*=2").empty());
    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("vlog 1vlog 2", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());

    EXPECT_FALSE(google::SetCommandLineOption("vmodule",
        "foo=3,logging_unittest=3, logg?ng_*=2 , logging_*=1 ").empty());
    for (int i = 0; i < 10; ++i) {
        VMLOG_NE(i) << "vlog " << i;
    }
    EXPECT_EQ("vlog 1vlog 2vlog 3", LOG_STREAM(VERBOSE).content_str());
    EXPECT_EQ("vlog 0", LOG_STREAM(INFO).content_str());

    for (int i = 0; i < 10; ++i) {
        VMLOG_IF(i, i % 2 == 1) << "vlog " << i << noflush;
    }
    EXPECT_EQ("vlog 1vlog 3", LOG_STREAM(VERBOSE).content_str());

    EXPECT_FALSE(google::SetCommandLineOption(
                     "vmodule",
                     "foo/bar0/0=2,foo/bar/1=3, 2=4, foo/*/3=5, */ba?/4=6,"
                     "/5=7,/foo/bar/6=8,foo2/bar/7=9,foo/bar/8=9").empty());
    VMLOG2_NE("foo/bar/0", 2) << " vlog0";
    VMLOG2_NE("foo/bar0/0", 2) << " vlog0.0";
    VMLOG2_NE("foo/bar/1", 3) << " vlog1";
    VMLOG2_NE("foo/bar/2", 4) << " vlog2";
    VMLOG2_NE("foo/bar2/2", 4) << " vlog2.2";
    VMLOG2_NE("foo/bar/3", 5) << " vlog3";
    VMLOG2_NE("foo/bar/4", 6) << " vlog4";
    VMLOG2_NE("foo/bar/5", 7) << " vlog5";
    VMLOG2_NE("foo/bar/6", 8) << " vlog6";
    VMLOG2_NE("foo/bar/7", 9) << " vlog7";
    VMLOG2_NE("foo/bar/8", 10) << " vlog8";
    VMLOG2_NE("foo/bar/9", 11) << " vlog9";
    EXPECT_EQ(" vlog0.0 vlog1 vlog2 vlog2.2 vlog3 vlog4",
              LOG_STREAM(VERBOSE).content_str());

    // Make sure verbose log is not flushed to other levels.
    ASSERT_TRUE(LOG_STREAM(FATAL).content_str().empty());
    ASSERT_TRUE(LOG_STREAM(ERROR).content_str().empty());
    ASSERT_TRUE(LOG_STREAM(WARNING).content_str().empty());
    ASSERT_TRUE(LOG_STREAM(NOTICE).content_str().empty());
    ASSERT_TRUE(LOG_STREAM(INFO).content_str().empty());
}

TEST_F(LoggingTest, check) {
    ::logging::FLAGS_crash_on_fatal_log = false;

    MCHECK(1 < 2);
    MCHECK(1 > 2);
    int a = 1;
    int b = 2;
    MCHECK(a > b) << "bad! a=" << a << " b=" << b;

    MCHECK_EQ(a, b) << "a=" << a << " b=" << b;
    MCHECK_EQ(1, 1) << "a=" << a << " b=" << b;

    MCHECK_NE(2, 1);
    MCHECK_NE(1, 2) << "blah0";
    MCHECK_NE(2, 2) << "blah1";
    
    MCHECK_LT(2, 3);
    MCHECK_LT(3, 2) << "blah2";
    MCHECK_LT(3, 3) << "blah3";

    MCHECK_LE(2, 3);
    MCHECK_LE(3, 2) << "blah4";
    MCHECK_LE(3, 3);

    MCHECK_GT(3, 2);
    MCHECK_GT(1, 2) << "1 can't be greater than 2";
    MCHECK_GT(3, 3) << "blah5";

    MCHECK_GE(3, 2);
    MCHECK_GE(2, 3) << "blah6";
    MCHECK_GE(3, 3);
}

int foo(int* p) {
    return ++*p;
}

TEST_F(LoggingTest, debug_level) {
    ::logging::FLAGS_crash_on_fatal_log = false;

    int run_foo = 0;
    MLOG(DEBUG) << foo(&run_foo) << noflush;
    MLOG(DEBUG) << foo(&run_foo);
    
    DMLOG(FATAL) << foo(&run_foo);
    DMLOG(WARNING) << foo(&run_foo);
    DMLOG(TRACE) << foo(&run_foo);
    DMLOG(NOTICE) << foo(&run_foo);
    DMLOG(DEBUG) << foo(&run_foo);

    EXPECT_FALSE(google::SetCommandLineOption("vmodule", "").empty());
    EXPECT_FALSE(google::SetCommandLineOption("v", "1").empty());
    DVMLOG(1) << foo(&run_foo);
    DVMLOG2("a/b/c", 1) << foo(&run_foo);

#ifdef NDEBUG
    ASSERT_EQ(0, run_foo);
#else
    ASSERT_EQ(9, run_foo);
#endif    
}

static void need_ostream(std::ostream& os, const char* s) {
    os << s;
}

TEST_F(LoggingTest, as_ostream) {
    ::logging::FLAGS_crash_on_fatal_log = false;

    need_ostream(LOG_STREAM(WARNING) << noflush, "hello");
    ASSERT_EQ("hello", LOG_STREAM(WARNING).content_str());

    need_ostream(LOG_STREAM(WARNING), "hello");
    ASSERT_EQ("", LOG_STREAM(WARNING).content_str());

    need_ostream(LOG_STREAM(INFO) << noflush, "world");
    ASSERT_EQ("world", LOG_STREAM(INFO).content_str());

    need_ostream(LOG_STREAM(INFO), "world");
    ASSERT_EQ("", LOG_STREAM(INFO).content_str());

    MLOG(WARNING) << 1.123456789;
    const std::streamsize saved_prec = LOG_STREAM(WARNING).precision(2);
    MLOG(WARNING) << 1.123456789;
    LOG_STREAM(WARNING).precision(saved_prec);
    MLOG(WARNING) << 1.123456789;
}

TEST_F(LoggingTest, limited_logging) {
    for (int i = 0; i < 100000; ++i) {
        MLOG_ONCE(INFO) << "HEHE1";
        MLOG_ONCE(INFO) << "HEHE2";
        VMLOG_ONCE(1) << "VHEHE3";
        VMLOG_ONCE(1) << "VHEHE4";
        MLOG_EVERY_N(INFO, 10000) << "i1=" << i;
        MLOG_EVERY_N(INFO, 5000) << "i2=" << i;
        VMLOG_EVERY_N(1, 10000) << "vi3=" << i;
        VMLOG_EVERY_N(1, 5000) << "vi4=" << i;
    }
    for (int i = 0; i < 300; ++i) {
        MLOG_EVERY_SECOND(INFO) << "i1=" << i;
        MLOG_EVERY_SECOND(INFO) << "i2=" << i;
        VMLOG_EVERY_SECOND(1) << "vi3=" << i;
        VMLOG_EVERY_SECOND(1) << "vi4=" << i;
        usleep(10000);
    }
}

void CheckFunctionName() {
    const char* func_name = __func__;
    DMCHECK(1) << "test";
    ASSERT_EQ(func_name, LOG_STREAM(DMCHECK).func());

    MLOG(DEBUG) << "test" << noflush;
    ASSERT_EQ(func_name, LOG_STREAM(DEBUG).func());
    MLOG(INFO) << "test" << noflush;
    ASSERT_EQ(func_name, LOG_STREAM(INFO).func());
    MLOG(WARNING) << "test" << noflush;
    ASSERT_EQ(func_name, LOG_STREAM(WARNING).func());
    MLOG(WARNING) << "test" << noflush;
    ASSERT_EQ(func_name, LOG_STREAM(WARNING).func());
    MLOG(ERROR) << "test" << noflush;
    ASSERT_EQ(func_name, LOG_STREAM(ERROR).func());
    MLOG(FATAL) << "test" << noflush;
    ASSERT_EQ(func_name, LOG_STREAM(FATAL).func());

    errno = EINTR;
    PMLOG(DEBUG) << "test" << noflush;
    ASSERT_EQ(func_name, PMLOG_STREAM(DEBUG).func());
    PMLOG(INFO) << "test" << noflush;
    ASSERT_EQ(func_name, PMLOG_STREAM(INFO).func());
    PMLOG(WARNING) << "test" << noflush;
    ASSERT_EQ(func_name, PMLOG_STREAM(WARNING).func());
    PMLOG(WARNING) << "test" << noflush;
    ASSERT_EQ(func_name, PMLOG_STREAM(WARNING).func());
    PMLOG(ERROR) << "test" << noflush;
    ASSERT_EQ(func_name, PMLOG_STREAM(ERROR).func());
    PMLOG(FATAL) << "test" << noflush;
    ASSERT_EQ(func_name, PMLOG_STREAM(FATAL).func());

    ::logging::StringSink log_str;
    ::logging::LogSink* old_sink = ::logging::SetLogSink(&log_str);
    MLOG_AT(WARNING, "specified_file.cc", 12345, "log_at") << "file/line is specified";
    // the file:line part should be using the argument given by us.
    ASSERT_NE(std::string::npos, log_str.find("specified_file.cc:12345 log_at"));
    ::logging::SetLogSink(old_sink);

    EXPECT_FALSE(google::SetCommandLineOption("v", "1").empty());
    VMLOG(100) << "test" << noflush;
    ASSERT_EQ(func_name, VMLOG_STREAM(100).func());
}

TEST_F(LoggingTest, log_func) {
    bool old_crash_on_fatal_log = ::logging::FLAGS_crash_on_fatal_log;
    ::logging::FLAGS_crash_on_fatal_log = false;

    ::logging::FLAGS_log_func_name = true;
    CheckFunctionName();
    ::logging::FLAGS_log_func_name = false;

    ::logging::FLAGS_crash_on_fatal_log = old_crash_on_fatal_log;
}

bool g_started = false;
bool g_stopped = false;
int g_prof_name_counter = 0;
mutil::atomic<uint64_t> test_logging_count(0);

void* test_async_log(void* arg) {
    if (arg == NULL) {
        return NULL;
    }
    auto log = (std::string*)(arg);
    while (!g_stopped) {
        MLOG(INFO) << *log;
        test_logging_count.fetch_add(1);
    }

    return NULL;
}

TEST_F(LoggingTest, async_log) {
    bool saved_async_log = FLAGS_async_log;
    FLAGS_async_log = true;
    mutil::TempFile temp_file;
    LoggingSettings settings;
    settings.logging_dest = LOG_TO_FILE;
    settings.log_file = temp_file.fname();
    settings.delete_old = DELETE_OLD_LOG_FILE;
    InitLogging(settings);

    std::string log = "135792468";
    int thread_num = 8;
    pthread_t threads[thread_num];
    for (int i = 0; i < thread_num; ++i) {
        ASSERT_EQ(0, pthread_create(&threads[i], NULL, test_async_log, &log));
    }

    sleep(5);

    g_stopped = true;
    for (int i = 0; i < thread_num; ++i) {
        pthread_join(threads[i], NULL);
    }
    // Wait for async log thread to flush all logs to file.
    sleep(10);

    std::ostringstream oss;
    std::string cmd = mutil::string_printf("grep -c %s %s",
        log.c_str(), temp_file.fname());
    ASSERT_LE(0, mutil::read_command_output(oss, cmd.c_str()));
    uint64_t log_count = std::strtol(oss.str().c_str(), NULL, 10);
    ASSERT_EQ(log_count, test_logging_count.load());

    FLAGS_async_log = saved_async_log;
}

struct MELON_CACHELINE_ALIGNMENT PerfArgs {
    const std::string* log;
    int64_t counter;
    int64_t elapse_ns;
    bool ready;

    PerfArgs() : log(NULL), counter(0), elapse_ns(0), ready(false) {}
};

void* test_log(void* void_arg) {
    auto args = (PerfArgs*)void_arg;
    args->ready = true;
    mutil::Timer t;
    while (!g_stopped) {
        if (g_started) {
            break;
        }
        usleep(10);
    }
    t.start();
    while (!g_stopped) {
        {
            MLOG(INFO) << *args->log;
            test_logging_count.fetch_add(1, mutil::memory_order_relaxed);
        }
        ++args->counter;
    }
    t.stop();
    args->elapse_ns = t.n_elapsed();
    return NULL;
}

void PerfTest(int thread_num, const std::string& log, bool async) {
    FLAGS_async_log = async;

    g_started = false;
    g_stopped = false;
    pthread_t threads[thread_num];
    std::vector<PerfArgs> args(thread_num);
    for (int i = 0; i < thread_num; ++i) {
        args[i].log = &log;
        ASSERT_EQ(0, pthread_create(&threads[i], NULL, test_log, &args[i]));
    }
    while (true) {
        bool all_ready = true;
        for (int i = 0; i < thread_num; ++i) {
            if (!args[i].ready) {
                all_ready = false;
                break;
            }
        }
        if (all_ready) {
            break;
        }
        usleep(1000);
    }
    g_started = true;
    char prof_name[32];
    snprintf(prof_name, sizeof(prof_name), "logging_%d.prof", ++g_prof_name_counter);
    ProfilerStart(prof_name);
    sleep(5);
    ProfilerStop();
    g_stopped = true;
    int64_t wait_time = 0;
    int64_t count = 0;
    for (int i = 0; i < thread_num; ++i) {
        pthread_join(threads[i], NULL);
        wait_time += args[i].elapse_ns;
        count += args[i].counter;
    }
    std::cout << " thread_num=" << thread_num
              << " log_type=" << (async ? "async" : "sync")
              << " log_size=" << log.size()
              << " count=" << count
              << " average_time=" << wait_time / (double)count
              << std::endl;
}

TEST_F(LoggingTest, performance) {
    bool saved_async_log = FLAGS_async_log;

    LoggingSettings settings;
    settings.logging_dest = LOG_TO_FILE;
    InitLogging(settings);
    std::string log(64, 'a');
    int thread_num = 1;
    PerfTest(thread_num, log, true);
    sleep(10);
    PerfTest(thread_num, log, false);

    thread_num = 2;
    PerfTest(thread_num, log, true);
    sleep(10);
    PerfTest(thread_num, log, false);

    thread_num = 4;
    PerfTest(thread_num, log, true);
    sleep(10);
    PerfTest(thread_num, log, false);

    FLAGS_async_log = saved_async_log;
}

}  // namespace

}  // namespace logging
#endif
