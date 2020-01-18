//

#include <chrono>
#include <cstdint>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace time_internal {

static int64_t GetCurrentTimeNanosFromSystem() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now() -
             std::chrono::system_clock::from_time_t(0))
      .count();
}

}  // namespace time_internal
ABEL_NAMESPACE_END
}  // namespace abel
