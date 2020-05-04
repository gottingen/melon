//
// Created by liyinbin on 2018/11/21.
//

#ifndef ABEL_STERINGS_ASCII_H_
#define ABEL_STERINGS_ASCII_H_

#include <abel/base/profile.h>
#include <cstdint>

namespace abel {

    enum class character_properties : uint32_t {
        eNone = 0x0,
        eControl = 0x0001,
        eSpace = 0x0002,
        ePunct = 0x0004,
        eDigit = 0x0008,
        eHexDigit = 0x0010,
        eAlpha = 0x0020,
        eLower = 0x0040,
        eUpper = 0x0080,
        eGraph = 0x0100,
        ePrint = 0x0200
    };

    ABEL_CONSTEXPR_FUNCTION character_properties operator&(character_properties lhs, character_properties rhs) {
        return static_cast<character_properties>(
                static_cast<uint32_t>(lhs) &
                static_cast<uint32_t>(rhs)
        );
    }

    ABEL_CONSTEXPR_FUNCTION character_properties operator|(character_properties lhs, character_properties rhs) {
        return static_cast<character_properties>(
                static_cast<uint32_t>(lhs) |
                static_cast<uint32_t>(rhs)
        );
    }

    ABEL_CONSTEXPR_FUNCTION character_properties operator~(character_properties lhs) {
        return static_cast<character_properties>(~static_cast<uint32_t>(lhs));
    }

    ABEL_CONSTEXPR_FUNCTION character_properties operator^(character_properties lhs, character_properties rhs) {
        return static_cast<character_properties>(
                static_cast<uint32_t>(lhs) ^
                static_cast<uint32_t>(rhs)
        );
    }

    ABEL_CONSTEXPR_FUNCTION character_properties &operator&=(character_properties &lhs, character_properties rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    ABEL_CONSTEXPR_FUNCTION character_properties &operator|=(character_properties &lhs, character_properties rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    ABEL_CONSTEXPR_FUNCTION character_properties &operator^=(character_properties &lhs, character_properties rhs) {
        lhs = lhs ^ rhs;
        return lhs;
    }

    class ascii {
    public:

        /**
         * @brief get all the properties of character
         * @param ch the input char
         * @return properties
         */
        static character_properties properties(unsigned char ch) noexcept;

        /**
         * @brief has_properties
         * @param ch the input char
         * @param properties properties
         * @return true or false
         */
        static bool has_properties(unsigned char ch, character_properties properties) noexcept;

        /**
         * @brief has_some_properties
         * @param ch the input char
         * @param properties  properties
         * @return true or false
         */
        static bool has_some_properties(unsigned char ch, character_properties properties) noexcept;

        /**
         * @brief is_graph
         * @param ch the input char
         * @return true or false
         */
        static bool is_graph(unsigned char ch) noexcept;

        /**
         * @brief check if the given character is digit
         * @param ch the input char the character
         * @return true if the given character is digit
         */
        static bool is_digit(unsigned char ch) noexcept;

        /**
         * @brief is_white
         * @param ch the input char
         * @return true or false
         */
        static bool is_white(unsigned char ch) noexcept;

        static bool is_blank(unsigned char ch) noexcept;

        /**
         * @brief is_ascii
         * @param ch the input char
         * @return true or false
         */
        static bool is_ascii(unsigned char ch) noexcept;

        /**
         * @brief is_space
         * @param ch the input char
         * @return true or false
         */
        static bool is_space(unsigned char ch) noexcept;

        /**
         * @brief is_hex_digit
         * @param ch the input char
         * @return true or false
         */
        static bool is_hex_digit(unsigned char ch) noexcept;

        /**
         * @brief is_punct
         * @param ch the input char
         * @return true or false
         */
        static bool is_punct(unsigned char ch) noexcept;

        /**
         * @brief is_print
         * @param ch the input char
         * @return true or false
         */
        static bool is_print(unsigned char ch) noexcept;

        /**
         * @brief is_alpha
         * @param ch the input char the input char
         * @return true or false
         */
        static bool is_alpha(unsigned char ch) noexcept;

        /**
         * @brief is_control
         * @param ch the input char
         * @return true or false
         */
        static bool is_control(unsigned char ch) noexcept;

        /**
         * @brief is_alpha_numeric
         * @param ch the input char
         * @return true or false
         */
        static bool is_alpha_numeric(unsigned char ch) noexcept;

        /**
         * @brief is_lower
         * @param ch the input char
         * @return true or false
         */
        static bool is_lower(unsigned char ch) noexcept;

        /**
         * @brief is_upper
         * @param ch the input char
         * @return true or false
         */
        static bool is_upper(unsigned char ch) noexcept;

        /**
         * @brief to_upper
         * @param ch the input char
         * @return true or false
         */
        static char to_upper(unsigned char ch) noexcept;

        /**
         * @brief to_lower
         * @param ch the input char
         * @return true or false
         */
        static char to_lower(unsigned char ch) noexcept;

    private:
        static const character_properties kCharacterProperties[128];
        static const char kToUpper[256];
        static const char kToLower[256];
    };

/// @brief ABEL_CONSTEXPR_FUNCTIONs

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_white(unsigned char ch) noexcept {
        return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_blank(unsigned char ch) noexcept {
        return ch == ' ' || ch == '\t';
    }

    ABEL_CONSTEXPR_FUNCTION character_properties ascii::properties(unsigned char ch) noexcept {
        if (is_ascii(ch)) {
            return kCharacterProperties[ch];
        } else {
            return character_properties::eNone;
        }
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_ascii(unsigned char ch) noexcept {
        return (static_cast<uint32_t>(ch) & 0xFFFFFF80) == 0;
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::has_properties(unsigned char ch, character_properties prop) noexcept {
        return (properties(ch) & prop) == prop;
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::has_some_properties(unsigned char ch, character_properties prop) noexcept {
        return (properties(ch) & prop) != character_properties::eNone;
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_space(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eSpace);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_print(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::ePrint);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_graph(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eGraph);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_digit(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eDigit);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_hex_digit(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eHexDigit);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_punct(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::ePunct);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_alpha(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eAlpha);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_control(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eControl);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_alpha_numeric(unsigned char ch) noexcept {
        return has_some_properties(ch, character_properties::eAlpha | character_properties::eDigit);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_lower(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eLower);
    }

    ABEL_CONSTEXPR_FUNCTION bool ascii::is_upper(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eUpper);
    }

    ABEL_CONSTEXPR_FUNCTION char ascii::to_lower(unsigned char ch) noexcept {
        return kToLower[ch];
    }

    ABEL_CONSTEXPR_FUNCTION char ascii::to_upper(unsigned char ch) noexcept {
        return kToUpper[ch];
    }

} //namespace abel

#endif // ABEL_BASE_STERING_ASCII_H_
