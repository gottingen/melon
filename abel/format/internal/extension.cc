//
//

#include <abel/format/internal/extension.h>
#include <cerrno>
#include <algorithm>
#include <string>

namespace abel {

namespace format_internal {
namespace {
// clang-format off
#define ABEL_LENGTH_MODS_EXPAND_ \
  X_VAL(h) X_SEP \
  X_VAL(hh) X_SEP \
  X_VAL(l) X_SEP \
  X_VAL(ll) X_SEP \
  X_VAL(L) X_SEP \
  X_VAL(j) X_SEP \
  X_VAL(z) X_SEP \
  X_VAL(t) X_SEP \
  X_VAL(q)
// clang-format on
}  // namespace

const LengthMod::Spec LengthMod::kSpecs[] = {
#define X_VAL(id) { LengthMod::id, #id, strlen(#id) }
#define X_SEP ,
    ABEL_LENGTH_MODS_EXPAND_, {LengthMod::none, "", 0}
#undef X_VAL
#undef X_SEP
};

const ConversionChar::Spec ConversionChar::kSpecs[] = {
#define X_VAL(id) { ConversionChar::id, #id[0] }
#define X_SEP ,
    ABEL_CONVERSION_CHARS_EXPAND_(X_VAL, X_SEP),
    {ConversionChar::none, '\0'},
#undef X_VAL
#undef X_SEP
};


const size_t LengthMod::kNumValues;

const size_t ConversionChar::kNumValues;



}  // namespace format_internal

}  // namespace abel
