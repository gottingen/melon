// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_IO_SCOPED_FP_H_
#define ABEL_IO_SCOPED_FP_H_

#include <cstdio>
#include "abel/base/profile.h"

namespace abel {

class scoped_fp {
  public:
    scoped_fp() : _fp(NULL) {}

    // Open file at |path| with |mode|.
    // If fopen failed, operator FILE* returns NULL and errno is set.
    scoped_fp(const char *path, const char *mode) {
        _fp = fopen(path, mode);
    }

    // |fp| must be the return value of fopen as we use fclose in the
    // destructor, otherwise the behavior is undefined
    explicit scoped_fp(FILE *fp) : _fp(fp) {}

    scoped_fp(scoped_fp &&rvalue) {
        _fp = rvalue._fp;
        rvalue._fp = NULL;
    }

    ~scoped_fp() {
        if (_fp != NULL) {
            fclose(_fp);
            _fp = NULL;
        }
    }

    // Close current opened file and open another file at |path| with |mode|
    void reset(const char *path, const char *mode) {
        reset(fopen(path, mode));
    }

    void reset() { reset(NULL); }

    void reset(FILE *fp) {
        if (_fp != NULL) {
            fclose(_fp);
            _fp = NULL;
        }
        _fp = fp;
    }

    // Set internal FILE* to NULL and return previous value.
    FILE *release() {
        FILE *const prev_fp = _fp;
        _fp = NULL;
        return prev_fp;
    }

    operator FILE *() const { return _fp; }

    FILE *get() { return _fp; }

  private:
    FILE *_fp;ABEL_NON_COPYABLE(scoped_fp);

};

}  // namespace abel

#endif  // ABEL_IO_SCOPED_FP_H_
