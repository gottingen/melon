
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_LOG_INIT_H_
#define MELON_LOG_INIT_H_


class log_initializer {
public:
    typedef void (*void_function)(void);
    log_initializer(const char*, void_function f) {
        f();
    }
};

#define REGISTER_MODULE_INITIALIZER(name, body)                 \
  namespace {                                                   \
    static void log_init_module_##name () { body; }          \
    log_initializer log_initializer_module_##name(#name,   \
            log_init_module_##name);                         \
  }


#endif  // MELON_LOG_INIT_H_
