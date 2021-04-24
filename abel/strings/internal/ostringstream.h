// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#ifndef ABEL_STRINGS_INTERNAL_OSTRINGSTREAM_H_
#define ABEL_STRINGS_INTERNAL_OSTRINGSTREAM_H_

#include <cassert>
#include <ostream>
#include <streambuf>
#include <string>

#include "abel/base/profile.h"

namespace abel {

namespace strings_internal {

// The same as std::ostringstream but appends to a user-specified std::string,
// and is faster. It is ~70% faster to create, ~50% faster to write to, and
// completely free to extract the result std::string.
//
//   std::string s;
//   OStringStream strm(&s);
//   strm << 42 << ' ' << 3.14;  // appends to `s`
//
// The stream object doesn't have to be named. Starting from C++11 operator<<
// works with rvalues of std::ostream.
//
//   std::string s;
//   OStringStream(&s) << 42 << ' ' << 3.14;  // appends to `s`
//
// OStringStream is faster to create than std::ostringstream but it's still
// relatively slow. Avoid creating multiple streams where a single stream will
// do.
//
// Creates unnecessary instances of OStringStream: slow.
//
//   std::string s;
//   OStringStream(&s) << 42;
//   OStringStream(&s) << ' ';
//   OStringStream(&s) << 3.14;
//
// Creates a single instance of OStringStream and reuses it: fast.
//
//   std::string s;
//   OStringStream strm(&s);
//   strm << 42;
//   strm << ' ';
//   strm << 3.14;
//
// Note: flush() has no effect. No reason to call it.
class OStringStream : private std::basic_streambuf<char>, public std::ostream {
  public:
    // The argument can be null, in which case you'll need to call str(p) with a
    // non-null argument before you can write to the stream.
    //
    // The destructor of OStringStream doesn't use the std::string. It's OK to
    // destroy the std::string before the stream.
    explicit OStringStream(std::string *s) : std::ostream(this), s_(s) {}

    std::string *str() { return s_; }

    const std::string *str() const { return s_; }

    void str(std::string *s) { s_ = s; }

  private:
    using Buf = std::basic_streambuf<char>;

    Buf::int_type overflow(int c) override;

    std::streamsize xsputn(const char *s, std::streamsize n) override;

    std::string *s_;
};

}  // namespace strings_internal

}  // namespace abel

#endif  // ABEL_STRINGS_INTERNAL_OSTRINGSTREAM_H_
