#ifndef ABEL_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_
#define ABEL_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_

#include <abel/strings/internal/str_format/extension.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace str_format_internal {

bool ConvertFloatImpl(float v, const ConversionSpec &conv,
                      FormatSinkImpl *sink);

bool ConvertFloatImpl(double v, const ConversionSpec &conv,
                      FormatSinkImpl *sink);

bool ConvertFloatImpl(long double v, const ConversionSpec &conv,
                      FormatSinkImpl *sink);

}  // namespace str_format_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_
