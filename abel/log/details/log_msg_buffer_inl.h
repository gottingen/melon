// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once


namespace abel {
namespace details {

ABEL_FORCE_INLINE log_msg_buffer::log_msg_buffer(const log_msg &orig_msg)
        : log_msg{orig_msg} {
    buffer.append(logger_name.begin(), logger_name.end());
    buffer.append(payload.begin(), payload.end());
    update_string_views();
}

ABEL_FORCE_INLINE log_msg_buffer::log_msg_buffer(const log_msg_buffer &other)
        : log_msg{other} {
    buffer.append(logger_name.begin(), logger_name.end());
    buffer.append(payload.begin(), payload.end());
    update_string_views();
}

ABEL_FORCE_INLINE log_msg_buffer::log_msg_buffer(log_msg_buffer &&other) ABEL_NOEXCEPT: log_msg{other},
                                                                                        buffer{std::move(
                                                                                                other.buffer)} {
    update_string_views();
}

ABEL_FORCE_INLINE log_msg_buffer &log_msg_buffer::operator=(const log_msg_buffer &other) {
    log_msg::operator=(other);
    buffer.clear();
    buffer.append(other.buffer.data(), other.buffer.data() + other.buffer.size());
    update_string_views();
    return *this;
}

ABEL_FORCE_INLINE log_msg_buffer &log_msg_buffer::operator=(log_msg_buffer &&other) ABEL_NOEXCEPT {
    log_msg::operator=(other);
    buffer = std::move(other.buffer);
    update_string_views();
    return *this;
}

ABEL_FORCE_INLINE void log_msg_buffer::update_string_views() {
    logger_name = std::string_view{buffer.data(), logger_name.size()};
    payload = std::string_view{buffer.data() + logger_name.size(), payload.size()};
}

} // namespace details
}  // namespace abel
