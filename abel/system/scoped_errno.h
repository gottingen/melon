//
// Created by liyinbin on 2020/3/30.
//

#ifndef ABEL_SYSTEM_SCOPED_ERRNO_H_
#define ABEL_SYSTEM_SCOPED_ERRNO_H_


#include <abel/base/profile.h>

namespace abel {
    class scoped_errno {
    public:
        scoped_errno() : _saved_errno(errno) {
            errno = 0;
        }

        ~scoped_errno() {
            if (errno == 0)
                errno = _saved_errno;
        }

    private:
        const int _saved_errno;
        ABEL_NON_COPYABLE(scoped_errno);
    };
}

#endif //ABEL_SYSTEM_SCOPED_ERRNO_H_
