//
// Created by liyinbin on 2020/1/25.
//

#include <abel/format/internal/length_mod.h>

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

const length_mod::Spec length_mod::kSpecs[] = {
#define X_VAL(id) { length_mod::id, #id, strlen(#id) }
#define X_SEP ,
    ABEL_LENGTH_MODS_EXPAND_, {length_mod::none, "", 0}
#undef X_VAL
#undef X_SEP
};


const size_t length_mod::kNumValues;

} //namespace format_internal
} //namespace abel