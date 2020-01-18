//

#include <abel/strings/internal/str_format/output.h>

#include <errno.h>
#include <cstring>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace str_format_internal {

namespace {
struct ClearErrnoGuard {
  ClearErrnoGuard() : old_value(errno) { errno = 0; }
  ~ClearErrnoGuard() {
    if (!errno) errno = old_value;
  }
  int old_value;
};
}  // namespace

void BufferRawSink::Write(string_view v) {
  size_t to_write = std::min(v.size(), size_);
  std::memcpy(buffer_, v.data(), to_write);
  buffer_ += to_write;
  size_ -= to_write;
  total_written_ += v.size();
}

void FILERawSink::Write(string_view v) {
  while (!v.empty() && !error_) {
    // Reset errno to zero in case the libc implementation doesn't set errno
    // when a failure occurs.
    ClearErrnoGuard guard;

    if (size_t result = std::fwrite(v.data(), 1, v.size(), output_)) {
      // Some progress was made.
      count_ += result;
      v.remove_prefix(result);
    } else {
      if (errno == EINTR) {
        continue;
      } else if (errno) {
        error_ = errno;
      } else if (std::ferror(output_)) {
        // Non-POSIX compliant libc implementations may not set errno, so we
        // have check the streams error indicator.
        error_ = EBADF;
      } else {
        // We're likely on a non-POSIX system that encountered EINTR but had no
        // way of reporting it.
        continue;
      }
    }
  }
}

}  // namespace str_format_internal
ABEL_NAMESPACE_END
}  // namespace abel
