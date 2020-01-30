
#pragma once

#include <abel/format/format.h>
#include <abel/log/details/log_msg.h>

namespace abel {

class formatter {
public:
    virtual ~formatter () = default;
    virtual void format (const details::log_msg &msg, fmt::memory_buffer &dest) = 0;
    virtual std::unique_ptr<formatter> clone () const = 0;
};
} // namespace abel
