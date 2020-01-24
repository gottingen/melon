//
// Output extension hooks for the Format library.
// `internal::InvokeFlush` calls the appropriate flush function for the
// specified output argument.
// `BufferRawSink` is a simple output sink for a char buffer. Used by SnprintF.
// `FILERawSink` is a std::FILE* based sink. Used by printf and FprintF.

#ifndef ABEL_STRINGS_FORMAT_OUTPUT_H_
#define ABEL_STRINGS_FORMAT_OUTPUT_H_

#include <cstdio>
#include <ostream>
#include <string>

#include <abel/base/profile.h>
#include <abel/strings/string_view.h>

namespace abel {


class Cord;

namespace format_internal {

// RawSink implementation that writes into a char* buffer.
// It will not overflow the buffer, but will keep the total count of chars
// that would have been written.
class BufferRawSink {
 public:
  BufferRawSink(char* buffer, size_t size) : buffer_(buffer), size_(size) {}

  size_t total_written() const { return total_written_; }
  void Write(string_view v);

 private:
  char* buffer_;
  size_t size_;
  size_t total_written_ = 0;
};

// RawSink implementation that writes into a FILE*.
// It keeps track of the total number of bytes written and any error encountered
// during the writes.
class FILERawSink {
 public:
  explicit FILERawSink(std::FILE* output) : output_(output) {}

  void Write(string_view v);

  size_t count() const { return count_; }
  int error() const { return error_; }

 private:
  std::FILE* output_;
  int error_ = 0;
  size_t count_ = 0;
};

// Provide RawSink integration with common types from the STL.
ABEL_FORCE_INLINE void AbelFormatFlush(std::string* out, string_view s) {
  out->append(s.data(), s.size());
}
ABEL_FORCE_INLINE void AbelFormatFlush(std::ostream* out, string_view s) {
  out->write(s.data(), s.size());
}

template <class AbelCord, typename = typename std::enable_if<
                              std::is_same<AbelCord, abel::Cord>::value>::type>
ABEL_FORCE_INLINE void AbelFormatFlush(AbelCord* out, string_view s) {
  out->Append(s);
}

ABEL_FORCE_INLINE void AbelFormatFlush(FILERawSink* sink, string_view v) {
  sink->Write(v);
}

ABEL_FORCE_INLINE void AbelFormatFlush(BufferRawSink* sink, string_view v) {
  sink->Write(v);
}

template <typename T>
auto InvokeFlush(T* out, string_view s)
    -> decltype(format_internal::AbelFormatFlush(out, s)) {
  format_internal::AbelFormatFlush(out, s);
}

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_STRINGS_INTERNAL_STR_FORMAT_OUTPUT_H_
