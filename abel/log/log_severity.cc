//

#include <abel/log/log_severity.h>

#include <ostream>

namespace abel {


std::ostream &operator << (std::ostream &os, abel::LogSeverity s) {
    if (s == abel::NormalizeLogSeverity(s))
        return os << abel::LogSeverityName(s);
    return os << "abel::LogSeverity(" << static_cast<int>(s) << ")";
}

}  // namespace abel
