//
// Output extension hooks for the Format library.
// `internal::InvokeFlush` calls the appropriate flush function for the
// specified output argument.
// `BufferRawSink` is a simple output sink for a char buffer. Used by SnprintF.
// `FILERawSink` is a std::FILE* based sink. Used by printf and FprintF.

#ifndef ABEL_FORMAT_INTERNAL_OUTPUT_H_
#define ABEL_FORMAT_INTERNAL_OUTPUT_H_

#include <abel/base/profile.h>
#include <abel/strings/string_view.h>
#include <cstdio>
#include <ostream>
#include <string>

namespace abel {

class Cord;

namespace format_internal {

class buffer_raw_sink {
public:
    buffer_raw_sink (char *buffer, size_t size) : _buffer(buffer), _size(size) { }

    size_t total_written () const { return _total_written; }
    void write (string_view v);

private:
    char *_buffer;
    size_t _size;
    size_t _total_written = 0;
};

class file_raw_sink {
public:
    explicit file_raw_sink (std::FILE *output) : _output(output) { }

    void write (string_view v);

    size_t count () const { return _count; }
    int error () const { return _error; }

private:
    std::FILE *_output;
    int _error = 0;
    size_t _count = 0;
};

// Provide RawSink integration with common types from the STL.
ABEL_FORCE_INLINE void abel_format_flush (std::string *out, string_view s) {
    out->append(s.data(), s.size());
}
ABEL_FORCE_INLINE void abel_format_flush (std::ostream *out, string_view s) {
    out->write(s.data(), s.size());
}

template<class AbelCord, typename = typename std::enable_if<
    std::is_same<AbelCord, abel::Cord>::value>::type>
ABEL_FORCE_INLINE void abel_format_flush (AbelCord *out, string_view s) {
    out->append(s);
}

ABEL_FORCE_INLINE void abel_format_flush (file_raw_sink *sink, string_view v) {
    sink->write(v);
}

ABEL_FORCE_INLINE void abel_format_flush (buffer_raw_sink *sink, string_view v) {
    sink->write(v);
}

template<typename T>
auto invoke_flush (T *out, string_view s)
-> decltype(format_internal::abel_format_flush(out, s)) {
    format_internal::abel_format_flush(out, s);
}

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_FORMAT_INTERNAL_OUTPUT_H_
