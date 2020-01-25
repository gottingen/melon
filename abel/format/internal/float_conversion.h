#ifndef ABEL_FORMAT_INTERNAL_FLOAT_CONVERSION_H_
#define ABEL_FORMAT_INTERNAL_FLOAT_CONVERSION_H_

#include <abel/format/internal/format_conv.h>
#include <abel/format/internal/sink_impl.h>
#include <abel/format/internal/conversion_spec.h>

namespace abel {

namespace format_internal {

bool ConvertFloatImpl(float v, const conversion_spec &conv,
                      format_sink_impl *sink);

bool ConvertFloatImpl(double v, const conversion_spec &conv,
                      format_sink_impl *sink);

bool ConvertFloatImpl(long double v, const conversion_spec &conv,
                      format_sink_impl *sink);

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_STRINGS_FORMAT_FLOAT_CONVERSION_H_
