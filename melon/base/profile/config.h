
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef MELON_BASE_INTERNAL_CONFIG_H_
#define MELON_BASE_INTERNAL_CONFIG_H_

#if defined(_MSC_VER)
#if defined(MELON_BUILD_DLL)
#define MELON_EXPORT __declspec(dllexport)
#elif defined(MELON_CONSUME_DLL)
#define MELON_EXPORT __declspec(dllimport)
#else
#define MELON_EXPORT
#endif
#else
#define MELON_EXPORT
#endif  // defined(_MSC_VER)

#endif  // MELON_BASE_INTERNAL_CONFIG_H_
