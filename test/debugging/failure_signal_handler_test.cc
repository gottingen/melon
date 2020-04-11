//
//

#include <abel/debugging/failure_signal_handler.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <abel/log/abel_logging.h>
#include <abel/debugging/stacktrace.h>
#include <abel/debugging/symbolize.h>
#include <abel/strings/str_cat.h>

namespace {

    using testing::StartsWith;

#if GTEST_HAS_DEATH_TEST
/*
// For the parameterized death tests. GetParam() returns the signal number.
    using FailureSignalHandlerDeathTest = ::testing::TestWithParam<int>;

// This function runs in a fork()ed process on most systems.
    void InstallHandlerAndRaise(int signo) {
        abel::InstallFailureSignalHandler(abel::FailureSignalHandlerOptions());
        raise(signo);
    }

    TEST_P(FailureSignalHandlerDeathTest, AbelFailureSignal) {
        const int signo = GetParam();
        std::string exit_regex = abel::string_cat(
                "\\*\\*\\* ", abel::debugging_internal::FailureSignalToString(signo),
                " received at time=");
#ifndef _WIN32
        EXPECT_EXIT(InstallHandlerAndRaise(signo), testing::KilledBySignal(signo),
                    exit_regex);
#else
        // Windows doesn't have testing::KilledBySignal().
        EXPECT_DEATH(InstallHandlerAndRaise(signo), exit_regex);
#endif
    }

    ABEL_CONST_INIT FILE *error_file = nullptr;

    void WriteToErrorFile(const char *msg) {
        if (msg != nullptr) {
            ABEL_RAW_CHECK(fwrite(msg, strlen(msg), 1, error_file) == 1,
                           "fwrite() failed");
        }
        ABEL_RAW_CHECK(fflush(error_file) == 0, "fflush() failed");
    }

    std::string GetTmpDir() {
        // TEST_TMPDIR is set by Bazel. Try the others when not running under Bazel.
        static const char *const kTmpEnvVars[] = {"TEST_TMPDIR", "TMPDIR", "TEMP",
                                                  "TEMPDIR", "TMP"};
        for (const char *const var : kTmpEnvVars) {
            const char *tmp_dir = std::getenv(var);
            if (tmp_dir != nullptr) {
                return tmp_dir;
            }
        }

        // Try something reasonable.
        return "/tmp";
    }

// This function runs in a fork()ed process on most systems.
    void InstallHandlerWithWriteToFileAndRaise(const char *file, int signo) {
        error_file = fopen(file, "w");
        ABEL_RAW_CHECK(error_file != nullptr, "Failed create error_file");
        abel::FailureSignalHandlerOptions options;
        options.writerfn = WriteToErrorFile;
        abel::InstallFailureSignalHandler(options);
        raise(signo);
    }

    TEST_P(FailureSignalHandlerDeathTest, AbelFatalSignalsWithWriterFn) {
        const int signo = GetParam();
        std::string tmp_dir = GetTmpDir();
        std::string file = abel::string_cat(tmp_dir, "/signo_", signo);

        std::string exit_regex = abel::string_cat(
                "\\*\\*\\* ", abel::debugging_internal::FailureSignalToString(signo),
                " received at time=");
#ifndef _WIN32
        EXPECT_EXIT(InstallHandlerWithWriteToFileAndRaise(file.c_str(), signo),
                    testing::KilledBySignal(signo), exit_regex);
#else
        // Windows doesn't have testing::KilledBySignal().
        EXPECT_DEATH(InstallHandlerWithWriteToFileAndRaise(file.c_str(), signo),
                     exit_regex);
#endif

        // Open the file in this process and check its contents.
        std::fstream error_output(file);
        ASSERT_TRUE(error_output.is_open()) << file;
        std::string error_line;
        std::getline(error_output, error_line);
        EXPECT_THAT(
                error_line,
                StartsWith(abel::string_cat(
                        "*** ", abel::debugging_internal::FailureSignalToString(signo),
                        " received at ")));

        if (abel::debugging_internal::StackTraceWorksForTest()) {
            std::getline(error_output, error_line);
            EXPECT_THAT(error_line, StartsWith("PC: "));
        }
    }

    constexpr int kFailureSignals[] = {
            SIGSEGV, SIGILL, SIGFPE, SIGABRT, SIGTERM,
#ifndef _WIN32
            SIGBUS, SIGTRAP,
#endif
    };

    std::string SignalParamToString(const ::testing::TestParamInfo<int> &info) {
        std::string result =
                abel::debugging_internal::FailureSignalToString(info.param);
        if (result.empty()) {
            result = abel::string_cat(info.param);
        }
        return result;
    }

    INSTANTIATE_TEST_SUITE_P(AbelDeathTest, FailureSignalHandlerDeathTest,
                             ::testing::ValuesIn(kFailureSignals),
                             SignalParamToString);
*/
#endif  // GTEST_HAS_DEATH_TEST

}  // namespace

int main(int argc, char **argv) {
    abel::InitializeSymbolizer(argv[0]);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
