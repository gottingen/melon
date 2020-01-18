//
//
#include <testing/mocking_bit_gen.h>

#include <string>

namespace abel {
ABEL_NAMESPACE_BEGIN
MockingBitGen::~MockingBitGen() {

  for (const auto& del : deleters_) {
    del();
  }
}

ABEL_NAMESPACE_END
}  // namespace abel
