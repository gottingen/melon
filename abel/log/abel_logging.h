//
// Created by liyinbin on 2020/4/12.
//

#ifndef ABEL_LOG_ABEL_LOGGING_H_
#define ABEL_LOG_ABEL_LOGGING_H_

#include <abel/log/log.h>
#include <abel/log/sinks/stdout_color_sinks.h>
#include <memory>
#include <abel/asl/functional.h>

namespace abel {

    void create_log_ptr();

    class log_singleton {
    public:
        friend void create_log_ptr();

        static void set_logger(std::shared_ptr<abel::log::logger> &log_ptr) {
            _log_ptr = log_ptr;
        }

        static std::shared_ptr<abel::log::logger> get_logger() {
            if (!_log_ptr) {
                static abel::once_flag log_once;
                abel::call_once(log_once, create_log_ptr);
            }
            return _log_ptr;
        }

    private:
        static std::shared_ptr<abel::log::logger> _log_ptr;

    };

} //namespace abel

#define ABEL_RAW_TRACE(...)                                   \
    do {                                                     \
    abel::log_singleton::get_logger()->trace("[ " __FILE__ "(" ABEL_STRINGIFY(__LINE__) ") ] " __VA_ARGS__);    \
    }while(0)

#define ABEL_RAW_DEBUG(...)                                   \
    do {                                                     \
    abel::log_singleton::get_logger()->debug("[ " __FILE__ "(" ABEL_STRINGIFY(__LINE__) ") ] " __VA_ARGS__);    \
    }while(0)

#define ABEL_RAW_INFO(...)                                   \
    do {                                                     \
    abel::log_singleton::get_logger()->info(__VA_ARGS__);    \
    }while(0)

#define ABEL_RAW_WARN(...)                                   \
    do {                                                     \
    abel::log_singleton::get_logger()->warn(__VA_ARGS__);    \
    }while(0)

#define ABEL_RAW_ERROR(...)                                   \
    do {                                                     \
    abel::log_singleton::get_logger()->error("[ " __FILE__ "(" ABEL_STRINGIFY(__LINE__) ") ] " __VA_ARGS__);   \
    }while(0)

#define ABEL_RAW_CRITICAL(...)                                   \
    do {                                                     \
    abel::log_singleton::get_logger()->critical("[ " __FILE__ "(" ABEL_STRINGIFY(__LINE__) ") ] " __VA_ARGS__);   \
    ::exit(1);                                               \
    }while(0)

#define ABEL_CHECK(condition, message)                             \
  do {                                                                 \
    if (ABEL_UNLIKELY(!(condition))) {                            \
      ABEL_RAW_CRITICAL("Check {} failed: {}", #condition, message); \
    }                                                                  \
  } while (0)

#endif //ABEL_LOG_ABEL_LOGGING_H_
