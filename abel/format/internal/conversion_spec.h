
#ifndef ABEL_FORMAT_INTERNAL_CONVERSION_SPEC_H_
#define ABEL_FORMAT_INTERNAL_CONVERSION_SPEC_H_
#include <abel/format/internal/conversion_char.h>
#include <abel/format/internal/format_flags.h>
#include <abel/format/internal/length_mod.h>
namespace abel {
namespace format_internal {


class conversion_spec {
public:
    format_flags flags () const { return flags_; }
    length_mod length_mod () const { return length_mod_; }
    conversion_char conv () const {
        // Keep this field first in the struct . It generates better code when
        // accessing it when conversion_spec is passed by value in registers.
        static_assert(offsetof(conversion_spec, conv_) == 0, "");
        return conv_;
    }

    // Returns the specified width. If width is unspecfied, it returns a negative
    // value.
    int width () const { return width_; }
    // Returns the specified precision. If precision is unspecfied, it returns a
    // negative value.
    int precision () const { return precision_; }

    void set_flags (format_flags f) { flags_ = f; }
    void set_length_mod (struct length_mod lm) { length_mod_ = lm; }
    void set_conv (conversion_char c) { conv_ = c; }
    void set_width (int w) { width_ = w; }
    void set_precision (int p) { precision_ = p; }
    void set_left (bool b) { flags_.left = b; }

private:
    conversion_char conv_;
    format_flags flags_;
    struct length_mod length_mod_;
    int width_;
    int precision_;
};

} //namespace format_internal
} //namespace abel
#endif //ABEL_FORMAT_INTERNAL_CONVERSION_SPEC_H_
