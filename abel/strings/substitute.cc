//

#include <abel/strings/substitute.h>

#include <algorithm>

#include <abel/log/raw_logging.h>
#include <abel/strings/ascii.h>
#include <abel/strings/escaping.h>
#include <abel/strings/string_view.h>

namespace abel {

namespace substitute_internal {

void SubstituteAndAppendArray(std::string* output, abel::string_view format,
                              const abel::string_view* args_array,
                              size_t num_args) {
  // Determine total size needed.
  size_t size = 0;
  for (size_t i = 0; i < format.size(); i++) {
    if (format[i] == '$') {
      if (i + 1 >= format.size()) {
#ifndef NDEBUG
        ABEL_RAW_LOG(FATAL,
                     "Invalid abel::Substitute() format std::string: \"%s\".",
                     abel::escape(format).c_str());
#endif
        return;
      } else if (abel::ascii::is_digit(format[i + 1])) {
        int index = format[i + 1] - '0';
        if (static_cast<size_t>(index) >= num_args) {
#ifndef NDEBUG
          ABEL_RAW_LOG(
              FATAL,
              "Invalid abel::Substitute() format std::string: asked for \"$"
              "%d\", but only %d args were given.  Full format std::string was: "
              "\"%s\".",
              index, static_cast<int>(num_args), abel::escape(format).c_str());
#endif
          return;
        }
        size += args_array[index].size();
        ++i;  // Skip next char.
      } else if (format[i + 1] == '$') {
        ++size;
        ++i;  // Skip next char.
      } else {
#ifndef NDEBUG
        ABEL_RAW_LOG(FATAL,
                     "Invalid abel::Substitute() format std::string: \"%s\".",
                     abel::escape(format).c_str());
#endif
        return;
      }
    } else {
      ++size;
    }
  }

  if (size == 0) return;

  // Build the std::string.
  size_t original_size = output->size();
    string_resize_uninitialized(output, original_size + size);
  char* target = &(*output)[original_size];
  for (size_t i = 0; i < format.size(); i++) {
    if (format[i] == '$') {
      if (abel::ascii::is_digit(format[i + 1])) {
        const abel::string_view src = args_array[format[i + 1] - '0'];
        target = std::copy(src.begin(), src.end(), target);
        ++i;  // Skip next char.
      } else if (format[i + 1] == '$') {
        *target++ = '$';
        ++i;  // Skip next char.
      }
    } else {
      *target++ = format[i];
    }
  }

  assert(target == output->data() + output->size());
}

Arg::Arg(const void* value) {
  static_assert(sizeof(scratch_) >= sizeof(value) * 2 + 2,
                "fix sizeof(scratch_)");
  if (value == nullptr) {
    piece_ = "NULL";
  } else {
    char* ptr = scratch_ + sizeof(scratch_);
    uintptr_t num = reinterpret_cast<uintptr_t>(value);
    do {
      *--ptr = abel::numbers_internal::kHexChar[num & 0xf];
      num >>= 4;
    } while (num != 0);
    *--ptr = 'x';
    *--ptr = '0';
    piece_ = abel::string_view(ptr, scratch_ + sizeof(scratch_) - ptr);
  }
}

// TODO(jorg): Don't duplicate so much code between here and str_cat.cc
Arg::Arg(hex h) {
  char* const end = &scratch_[numbers_internal::kFastToBufferSize];
  char* writer = end;
  uint64_t value = h.value;
  do {
    *--writer = abel::numbers_internal::kHexChar[value & 0xF];
    value >>= 4;
  } while (value != 0);

  char* beg;
  if (end - writer < h.width) {
    beg = end - h.width;
    std::fill_n(beg, writer - beg, h.fill);
  } else {
    beg = writer;
  }

  piece_ = abel::string_view(beg, end - beg);
}

// TODO(jorg): Don't duplicate so much code between here and str_cat.cc
Arg::Arg(dec d) {
  assert(d.width <= numbers_internal::kFastToBufferSize);
  char* const end = &scratch_[numbers_internal::kFastToBufferSize];
  char* const minfill = end - d.width;
  char* writer = end;
  uint64_t value = d.value;
  bool neg = d.neg;
  while (value > 9) {
    *--writer = '0' + (value % 10);
    value /= 10;
  }
  *--writer = '0' + value;
  if (neg) *--writer = '-';

  ptrdiff_t fillers = writer - minfill;
  if (fillers > 0) {
    // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
    // But...: if the fill character is '0', then it's <+/-><fill><digits>
    bool add_sign_again = false;
    if (neg && d.fill == '0') {  // If filling with '0',
      ++writer;                    // ignore the sign we just added
      add_sign_again = true;       // and re-add the sign later.
    }
    writer -= fillers;
    std::fill_n(writer, fillers, d.fill);
    if (add_sign_again) *--writer = '-';
  }

  piece_ = abel::string_view(writer, end - writer);
}

}  // namespace substitute_internal

}  // namespace abel
