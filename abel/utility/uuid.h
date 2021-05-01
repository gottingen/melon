//
// Created by liyinbin on 2021/5/1.
//

#ifndef ABEL_UTILITY_UUID_H_
#define ABEL_UTILITY_UUID_H_


#include <cstddef>
#include <cstring>

#include <optional>
#include <string>
#include <string_view>

#include "abel/log/logging.h"

namespace abel {


    // Represents a UUID.
    class uuid {
    public:
        constexpr uuid() = default;

        // If `from` is malformed, the program crashes.
        //
        // To parse UUID from untrusted source, use `TryParse<uuid>(...)` instead.
        constexpr explicit uuid(const std::string_view &from);

        std::string to_string() const;

        constexpr bool operator==(const uuid &other) const noexcept;

        constexpr bool operator!=(const uuid &other) const noexcept;

        constexpr bool operator<(const uuid &other) const noexcept;

    private:
        constexpr int compare_to(const uuid &other) const noexcept;

        constexpr static int to_decimal(char x);

        constexpr static std::uint8_t to_uint8(const char *starts_at);

    private:
        std::uint8_t bytes_[16] = {0};
    };


    constexpr uuid::uuid(const std::string_view &from) {
        DCHECK_EQ(from.size(), 36);  // 8-4-4-4-12
        auto p = from.data();

        bytes_[0] = to_uint8(p);
        bytes_[1] = to_uint8(p + 2);
        bytes_[2] = to_uint8(p + 4);
        bytes_[3] = to_uint8(p + 6);
        p += 8;
        DCHECK_EQ(*p++, '-');

        bytes_[4] = to_uint8(p);
        bytes_[5] = to_uint8(p + 2);
        p += 4;
        DCHECK_EQ(*p++, '-');

        bytes_[6] = to_uint8(p);
        bytes_[7] = to_uint8(p + 2);
        p += 4;
        DCHECK_EQ(*p++, '-');

        bytes_[8] = to_uint8(p);
        bytes_[9] = to_uint8(p + 2);
        p += 4;
        DCHECK_EQ(*p++, '-');

        bytes_[10] = to_uint8(p);
        bytes_[11] = to_uint8(p + 2);
        bytes_[12] = to_uint8(p + 4);
        bytes_[13] = to_uint8(p + 6);
        bytes_[14] = to_uint8(p + 8);
        bytes_[15] = to_uint8(p + 10);
    }

    constexpr bool uuid::operator==(const uuid &other) const noexcept {
        return compare_to(other) == 0;
    }

    constexpr bool uuid::operator!=(const uuid &other) const noexcept {
        return compare_to(other) != 0;
    }

    constexpr bool uuid::operator<(const uuid &other) const noexcept {
        return compare_to(other) < 0;
    }

    constexpr int uuid::compare_to(const uuid &other) const noexcept {
        // `memcmp` is not `constexpr`..
        return __builtin_memcmp(bytes_, other.bytes_, sizeof(bytes_));
    }

    constexpr int uuid::to_decimal(char x) {
        if (x >= '0' && x <= '9') {
            return x - '0';
        } else if (x >= 'a' && x <= 'f') {
            return x - 'a' + 10;
        } else if (x >= 'A' && x <= 'F') {
            return x - 'A' + 10;
        } else {
            DCHECK_MSG(0, "Invalid hex digit [{}].", x);
        }
    }

    constexpr std::uint8_t uuid::to_uint8(const char *starts_at) {
        return to_decimal(starts_at[0]) * 16 + to_decimal(starts_at[1]);
    }

    std::optional<abel::uuid> parse_uuid(const std::string_view &s);


}  // namespace abel

#endif  // ABEL_UTILITY_UUID_H_
