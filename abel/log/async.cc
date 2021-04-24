// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)


#include "abel/log/async.h"
#include "abel/log/async_logger_inl.h"
#include "abel/log/details/periodic_worker_inl.h"
#include "abel/log/details/thread_pool_inl.h"

template
class ABEL_API abel::details::mpmc_blocking_queue<abel::details::async_msg>;
