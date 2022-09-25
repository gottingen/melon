
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_GTEST_WRAP_H
#define MELON_GTEST_WRAP_H

#ifdef private
#undef private
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define private public
#else
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#endif

#endif //MELON_GTEST_WRAP_H
