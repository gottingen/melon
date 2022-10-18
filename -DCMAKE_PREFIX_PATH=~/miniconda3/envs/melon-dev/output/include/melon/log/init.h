
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_LOG_INIT_H_
#define MELON_LOG_INIT_H_


class melon_initializer {
public:
    typedef void (*void_function)(void);
    melon_initializer(const char*, void_function f) {
        f();
    }
};

#define REGISTER_MODULE_INITIALIZER(name, body)                 \
  namespace {                                                   \
    static void melon_init_module_##name () { body; }          \
    melon_initializer melon_initializer_module_##name(#name,   \
            melon_init_module_##name);                         \
  }


#endif  // MELON_LOG_INIT_H_
