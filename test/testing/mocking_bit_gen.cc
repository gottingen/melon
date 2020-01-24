
#include <testing/mocking_bit_gen.h>

#include <string>

namespace abel {

MockingBitGen::~MockingBitGen () {

    for (const auto &del : deleters_) {
        del();
    }
}

}  // namespace abel
