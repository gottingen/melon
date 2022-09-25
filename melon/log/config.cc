
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "melon/log/config.h"
#include "melon/log/severity.h"
#include <gflags/gflags.h>

static bool BoolFromEnv(const char *varname, bool defval) {
    const char *const valstr = getenv(varname);
    if (!valstr) {
        return defval;
    }
    return memchr("tTyY1\0", valstr[0], 6) != NULL;
}

MELON_LOG_DEFINE_bool(melon_timestamp_in_logfile_name,
                      BoolFromEnv("GOOGLE_TIMESTAMP_IN_LOGFILE_NAME", true),
                      "put a timestamp at the end of the log file name");
MELON_LOG_DEFINE_bool(melon_logtostderr, BoolFromEnv("GOOGLE_LOGTOSTDERR", false),
                      "log messages go to stderr instead of logfiles");
MELON_LOG_DEFINE_bool(melon_also_logtostderr, BoolFromEnv("GOOGLE_ALSOLOGTOSTDERR", false),
                      "log messages go to stderr in addition to logfiles");
MELON_LOG_DEFINE_bool(melon_colorlogtostderr, false,
                      "color messages logged to stderr (if supported by terminal)");
#ifdef MELON_PLATFORM_LINUX
MELON_LOG_DEFINE_bool(drop_log_memory, true, "Drop in-memory buffers of log contents. "
                 "Logs can grow very quickly and they are rarely read before they "
                 "need to be evicted from memory. Instead, drop them from memory "
                 "as soon as they are flushed to disk.");
#endif

// By default, errors (including fatal errors) get logged to stderr as
// well as the file.
//
// The default is ERROR instead of FATAL so that users can see problems
// when they run a program without having to look in another file.
DEFINE_int32(melon_stderrthreshold,
             melon::log::MELON_ERROR,
             "log messages at or above this level are copied to stderr in "
             "addition to logfiles.  This flag obsoletes --melon_also_logtostderr.");

MELON_LOG_DEFINE_string(melon_also_log_to_email, "",
                        "log messages go to these email addresses "
                        "in addition to logfiles");
MELON_LOG_DEFINE_bool(melon_log_prefix, true,
                      "Prepend the log prefix to the start of each log line");
MELON_LOG_DEFINE_int32(melon_minloglevel, 2, "Messages logged at a lower level than this don't "
                                       "actually get logged anywhere");
MELON_LOG_DEFINE_int32(melon_logbuflevel, 2,
                       "Buffer log messages logged at this level or lower"
                       " (-1 means don't buffer; 0 means buffer INFO only;"
                       " ...)");
MELON_LOG_DEFINE_int32(melon_logbufsecs, 30,
                       "Buffer log messages for at most this many seconds");
MELON_LOG_DEFINE_int32(melon_log_email_level, 999,
                       "Email log messages logged at this level or higher"
                       " (0 means email all; 3 means email FATAL only;"
                       " ...)");
MELON_LOG_DEFINE_string(melon_log_mailer, "",
                        "Mailer used to send logging email");


// Compute the default value for --melon_log_dir
static const char *DefaultLogDir() {
    const char *env;
    env = getenv("GOOGLE_LOG_DIR");
    if (env != NULL && env[0] != '\0') {
        return env;
    }
    env = getenv("TEST_TMPDIR");
    if (env != NULL && env[0] != '\0') {
        return env;
    }
    return "";
}

DEFINE_int32(melon_log_save_days, 7, "log keep days, default is 7 days");

MELON_LOG_DEFINE_int32(melon_logfile_mode, 0664, "Log file mode/permissions.");

MELON_LOG_DEFINE_string(melon_log_dir, DefaultLogDir(),
                        "If specified, logfiles are written into this directory instead "
                        "of the default logging directory.");
MELON_LOG_DEFINE_string(melon_log_link, "", "Put additional links to the log "
                                      "files in this directory");

MELON_LOG_DEFINE_int32(melon_max_log_size, 1800,
                       "approx. maximum log file size (in MB). A value of 0 will "
                       "be silently overridden to 1.");

MELON_LOG_DEFINE_bool(melon_stop_logging_if_full_disk, false,
                      "Stop attempting to log to disk if the disk is full.");

MELON_LOG_DEFINE_string(melonlog_backtrace_at, "",
                        "Emit a backtrace when logging at file:linenum.");

MELON_LOG_DEFINE_bool(melon_log_utc_time, false,
                      "Use UTC time for logging.");

DEFINE_bool(melon_log_as_json, false, "Print log as a valid JSON");
DEFINE_bool(melon_crash_on_fatal_log, false, "crash on fatal log");
