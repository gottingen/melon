//

#include <abel/base/log_severity.h>

#include <ostream>

namespace abel {
ABEL_NAMESPACE_BEGIN

std::ostream& operator<<(std::ostream& os, abel::LogSeverity s) {
  if (s == abel::NormalizeLogSeverity(s)) return os << abel::LogSeverityName(s);
  return os << "abel::LogSeverity(" << static_cast<int>(s) << ")";
}
ABEL_NAMESPACE_END
}  // namespace abel
