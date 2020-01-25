//

#include <abel/format/internal/output.h>
#include <errno.h>
#include <cstring>

namespace abel {

namespace format_internal {

namespace {
struct ClearErrnoGuard {
    ClearErrnoGuard () : old_value(errno) { errno = 0; }
    ~ClearErrnoGuard () {
        if (!errno)
            errno = old_value;
    }
    int old_value;
};
}  // namespace

void buffer_raw_sink::write (string_view v) {
    size_t to_write = std::min(v.size(), _size);
    std::memcpy(_buffer, v.data(), to_write);
    _buffer += to_write;
    _size -= to_write;
    _total_written += v.size();
}

void file_raw_sink::write (string_view v) {
    while (!v.empty() && !_error) {
        // Reset errno to zero in case the libc implementation doesn't set errno
        // when a failure occurs.
        ClearErrnoGuard guard;

        if (size_t result = std::fwrite(v.data(), 1, v.size(), _output)) {
            // Some progress was made.
            _count += result;
            v.remove_prefix(result);
        } else {
            if (errno == EINTR) {
                continue;
            } else if (errno) {
                _error = errno;
            } else if (std::ferror(_output)) {
                // Non-POSIX compliant libc implementations may not set errno, so we
                // have check the streams error indicator.
                _error = EBADF;
            } else {
                // We're likely on a non-POSIX system that encountered EINTR but had no
                // way of reporting it.
                continue;
            }
        }
    }
}

}  // namespace format_internal

}  // namespace abel
