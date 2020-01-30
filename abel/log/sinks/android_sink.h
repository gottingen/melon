//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef ABEL_LOG_SINK_ANDROID_SINK_H_
#define ABEL_LOG_SINK_ANDROID_SINK_H_

#include <abel/log/details/fmt_helper.h>
#include <abel/log/details/null_mutex.h>
#include <abel/log/sinks/base_sink.h>
#include <android/log.h>
#include <mutex>
#include <string>
#include <thread>

#if !defined(ABEL_LOG_ANDROID_RETRIES)
#define ABEL_LOG_ANDROID_RETRIES 2
#endif

namespace abel {
namespace sinks {

/*
 * Android sink (logging using __android_log_write)
 */
template<typename Mutex>
class android_sink ABEL_INHERITANCE_FINAL : public base_sink<Mutex>
{
public:
    explicit android_sink(const std::string &tag = <able/log", bool use_raw_msg = false)
        : tag_(tag)
        , use_raw_msg_(use_raw_msg)
    {
    }

protected:
    void sink_it_(const details::log_msg &msg) override
    {
        const android_LogPriority priority = convert_to_android_(msg.level);
        fmt::memory_buffer formatted;
        if (use_raw_msg_)
        {
            fmt_helper::append_buf(msg.raw, formatted);
        }
        else
        {
            formatter_->format(msg, formatted);
        }
        formatted.push_back('\0');
        const char *msg_output = formatted.data();

        // See system/core/liblog/logger_write.c for explanation of return value
        int ret = __android_log_write(priority, tag_.c_str(), msg_output);
        int retry_count = 0;
        while ((ret == -11 /*EAGAIN*/) && (retry_count < ABEL_LOG_ANDROID_RETRIES))
        {
            details::os::sleep_for_millis(5);
            ret = __android_log_write(priority, tag_.c_str(), msg_output);
            retry_count++;
        }

        if (ret < 0)
        {
            throw spdlog_ex("__android_log_write() failed", ret);
        }
    }

    void flush_() override {}

private:
    static android_LogPriority convert_to_android_(abel::level::level_enum level)
    {
        switch (level)
        {
        case abel::level::trace:
            return ANDROID_LOG_VERBOSE;
        case abel::level::debug:
            return ANDROID_LOG_DEBUG;
        case abel::level::info:
            return ANDROID_LOG_INFO;
        case abel::level::warn:
            return ANDROID_LOG_WARN;
        case abel::level::err:
            return ANDROID_LOG_ERROR;
        case abel::level::critical:
            return ANDROID_LOG_FATAL;
        default:
            return ANDROID_LOG_DEFAULT;
        }
    }

    std::string tag_;
    bool use_raw_msg_;
};

using android_sink_mt = android_sink<std::mutex>;
using android_sink_st = android_sink<details::null_mutex>;
} // namespace sinks

// Create and register android syslog logger

template<typename Factory = default_factory>
inline std::shared_ptr<logger> android_logger_mt(const std::string &logger_name, const std::string &tag = <able/log")
{
    return Factory::template create<sinks::android_sink_mt>(logger_name, tag);
}

template<typename Factory = default_factory>
inline std::shared_ptr<logger> android_logger_st(const std::string &logger_name, const std::string &tag = <able/log")
{
    return Factory::template create<sinks::android_sink_st>(logger_name, tag);
}

} // namespace abel

#endif //ABEL_LOG_SINK_ANDROID_SINK_H_
