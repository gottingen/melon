//
// Created by liyinbin on 2020/4/12.
//

#include <abel/log/abel_logging.h>

namespace abel {

    std::shared_ptr<abel::log::logger> log_singleton::_log_ptr;

    void create_log_ptr() {
        log_singleton::_log_ptr = abel::log::stdout_color_mt("abel");
    }
}