
#ifndef ABEL_LOG_FORMATTERR_H_
#define ABEL_LOG_FORMATTERR_H_

#include <abel/format/format.h>
#include <abel/log/details/log_msg.h>

namespace abel {
    namespace log {
        class formatter {
        public:
            virtual ~formatter() = default;

            virtual void format(const details::log_msg &msg, fmt::memory_buffer &dest) = 0;

            virtual std::unique_ptr<formatter> clone() const = 0;
        };
    } //namespace log
} // namespace abel
#endif //ABEL_LOG_FORMATTERR_H_
