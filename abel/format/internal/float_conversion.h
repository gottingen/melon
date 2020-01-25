#ifndef ABEL_FORMAT_INTERNAL_FLOAT_CONVERSION_H_
#define ABEL_FORMAT_INTERNAL_FLOAT_CONVERSION_H_

#include <abel/format/internal/extension.h>
#include <abel/format/internal/sink_impl.h>

namespace abel {

namespace format_internal {

bool ConvertFloatImpl(float v, const ConversionSpec &conv,
                      format_sink_impl *sink);

bool ConvertFloatImpl(double v, const ConversionSpec &conv,
                      format_sink_impl *sink);

bool ConvertFloatImpl(long double v, const ConversionSpec &conv,
                      format_sink_impl *sink);

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_STRINGS_FORMAT_FLOAT_CONVERSION_H_
