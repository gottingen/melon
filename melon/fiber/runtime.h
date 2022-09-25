
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_FIBER_RUNTIME_H_
#define MELON_FIBER_RUNTIME_H_

namespace melon {


    // ---------------------------------------------
    // Functions for scheduling control.
    // ---------------------------------------------

    // Get number of worker pthreads
    int fiber_getconcurrency(void);

    // Set number of worker pthreads to `num'. After a successful call,
    // fiber_getconcurrency() shall return new set number, but workers may
    // take some time to quit or create.
    // NOTE: currently concurrency cannot be reduced after any fiber created.
    int fiber_setconcurrency(int num);

}  // namespace melon
#endif // MELON_FIBER_RUNTIME_H_
