//

#include <testing/test_instance_tracker.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace test_internal {
int BaseCountedInstance::num_instances_ = 0;
int BaseCountedInstance::num_live_instances_ = 0;
int BaseCountedInstance::num_moves_ = 0;
int BaseCountedInstance::num_copies_ = 0;
int BaseCountedInstance::num_swaps_ = 0;
int BaseCountedInstance::num_comparisons_ = 0;

}  // namespace test_internal
ABEL_NAMESPACE_END
}  // namespace abel
