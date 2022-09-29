
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef  MELON_FIBER_INTERNAL_FIBER_COND_H_
#define  MELON_FIBER_INTERNAL_FIBER_COND_H_

#include "melon/times/time.h"
#include "melon/fiber/internal/mutex.h"

__BEGIN_DECLS
extern int fiber_cond_init(fiber_cond_t *__restrict cond,
                           const fiber_condattr_t *__restrict cond_attr);
extern int fiber_cond_destroy(fiber_cond_t *cond);
extern int fiber_cond_signal(fiber_cond_t *cond);
extern int fiber_cond_broadcast(fiber_cond_t *cond);
extern int fiber_cond_wait(fiber_cond_t *__restrict cond,
                           fiber_mutex_t *__restrict mutex);
extern int fiber_cond_timedwait(
        fiber_cond_t *__restrict cond,
        fiber_mutex_t *__restrict mutex,
        const struct timespec *__restrict abstime);
__END_DECLS

#endif  // MELON_FIBER_INTERNAL_FIBER_COND_H_
