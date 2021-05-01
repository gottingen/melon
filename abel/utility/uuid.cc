//
// Created by liyinbin on 2021/5/1.
//


#include "abel/utility/uuid.h"

#include <cstdio>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include "abel/strings/format.h"


namespace abel {

    std::string uuid::to_string() const {
        return abel::format(
                "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:"
                "02x}{:02x}{:02x}{:02x}{:02x}",
                bytes_[0], bytes_[1], bytes_[2], bytes_[3], bytes_[4], bytes_[5],
                bytes_[6], bytes_[7], bytes_[8], bytes_[9], bytes_[10], bytes_[11],
                bytes_[12], bytes_[13], bytes_[14], bytes_[15]);
    }


    std::optional<abel::uuid> parse_uuid(const std::string_view &s) {
        static constexpr auto kExpectedLength = 36;
        static constexpr std::array<bool, kExpectedLength> kHexExpected = [&]() {
            std::array<bool, kExpectedLength> ary{};
            for (auto &&e : ary) {
                e = true;
            }
            ary[8] = ary[13] = ary[18] = ary[23] = false;
            return ary;
        }();

        if (s.size() != kExpectedLength) {
            return std::nullopt;
        }
        for (int i = 0; i != kExpectedLength; ++i) {
            if (!kHexExpected[i]) {
                if (s[i] != '-') {
                    return std::nullopt;
                }
            } else {
                bool valid = false;
                valid |= s[i] >= '0' && s[i] <= '9';
                valid |= s[i] >= 'a' && s[i] <= 'f';
                valid |= s[i] >= 'A' && s[i] <= 'F';
                if (!valid) {
                    return std::nullopt;
                }
            }
        }
        // FIXME: See https://en.wikipedia.org/wiki/Universally_unique_identifier,
        // it seems we need to validate certain bytes to see if it's a valid UUID.
        return uuid(s);
    }

}  // namespace abel
