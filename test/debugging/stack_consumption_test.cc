//
//

#include <abel/debugging/internal/stack_consumption.h>

#ifdef ABEL_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#include <string.h>

#include <gtest/gtest.h>
#include <abel/base/internal/raw_logging.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace debugging_internal {
namespace {

static void SimpleSignalHandler(int signo) {
  char buf[100];
  memset(buf, 'a', sizeof(buf));

  // Never true, but prevents compiler from optimizing buf out.
  if (signo == 0) {
    ABEL_RAW_LOG(INFO, "%p", static_cast<void*>(buf));
  }
}

TEST(SignalHandlerStackConsumptionTest, MeasuresStackConsumption) {
  // Our handler should consume reasonable number of bytes.
  EXPECT_GE(GetSignalHandlerStackConsumption(SimpleSignalHandler), 100);
}

}  // namespace
}  // namespace debugging_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION
