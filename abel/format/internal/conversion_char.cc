

#include <abel/format/internal/conversion_char.h>

namespace abel {
namespace format_internal {

const conversion_char::Spec conversion_char::kSpecs[] = {
#define X_VAL(id) { conversion_char::id, #id[0] }
#define X_SEP ,
    ABEL_CONVERSION_CHARS_EXPAND_(X_VAL, X_SEP),
    {conversion_char::none, '\0'},
#undef X_VAL
#undef X_SEP
};

const size_t conversion_char::kNumValues;

} //format_internal
} //abel