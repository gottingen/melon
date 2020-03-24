//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_STRINGS_TRIM_H_
#define ABEL_STRINGS_TRIM_H_

#include <abel/base/profile.h>
#include <abel/asl/ascii.h>
#include <abel/strings/string_view.h>
#include <string>

namespace abel {
/*!
 * Trims the given string in-place only on the right. Removes all characters in
 * the given drop array, which defaults to " \r\n\t".
 *
 * \param str   string to process
 * \return      reference to the modified string
 */
    std::string &trim_right(std::string *str);

/*!
 * Trims the given string in-place only on the right. Removes all characters in
 * the given drop array.
 *
 * \param str   string to process
 * \param drop  remove these characters
 * \return      reference to the modified string
 */
    std::string &trim_right(std::string *str, abel::string_view drop);

/*!
 * Trims the given string only on the right. Removes all characters in the
 * given drop array. Returns a copy of the string.
 *
 * \param str   string to process
 * \param drop  remove these characters
 * \return      new trimmed string
 */
    abel::string_view trim_right(abel::string_view str, abel::string_view drop);

/**
 * @brief
 * @param str
 * @return
 */


    ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE abel::string_view trim_right(abel::string_view str) {
        auto it = std::find_if_not(str.rbegin(), str.rend(), abel::ascii::is_space);
        return str.substr(0, str.rend() - it);
    }

/******************************************************************************/

/*!
 * Trims the given string in-place only on the left. Removes all characters in
 * the given drop array.
 *
 * \param str   string to process
 * \return      reference to the modified string
 */
    std::string &trim_left(std::string *str);

/*!
 * Trims the given string in-place only on the left. Removes all characters in
 * the given drop array.
 *
 * \param str   string to process
 * \param drop  remove these characters
 * \return      reference to the modified string
 */
    std::string &trim_left(std::string *str, abel::string_view drop);

/*!
 * Trims the given string only on the left. Removes all characters in the given
 * drop array. Returns a copy of the string.
 *
 * \param str   string to process
 * \param drop  remove these characters
 * \return      new trimmed string
 */
    abel::string_view trim_left(abel::string_view str, abel::string_view drop);

/**
 * @brief
 * @param str
 * @return
 */
    ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE abel::string_view trim_left(
            abel::string_view str) {
        auto it = std::find_if_not(str.begin(), str.end(), abel::ascii::is_space);
        return str.substr(it - str.begin());
    }

/*!
 * Trims the given string in-place on the left and right. Removes all
 * characters in the given drop array, which defaults to " \r\n\t".
 *
 * \param str   string to process
 * \return      reference to the modified string
 */
    std::string &trim_all(std::string *str);

/*!
 * Trims the given string in-place on the left and right. Removes all
 * characters in the given drop array, which defaults to " \r\n\t".
 *
 * \param str   string to process
 * \param drop  remove these characters
 * \return      reference to the modified string
 */
    std::string &trim_all(std::string *str, abel::string_view drop);


/*!
 * Trims the given string in-place on the left and right. Removes all
 * characters in the given drop array, which defaults to " \r\n\t".
 *
 * \param str   string to process
 * \param drop  remove these characters
 * \return      reference to the modified string
 */
    abel::string_view trim_all(abel::string_view str, abel::string_view drop);

    ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE abel::string_view trim_all(
            abel::string_view str) {
        return trim_right(trim_left(str));
    }

    void trim_complete(std::string *);
}
#endif //ABEL_STRINGS_TRIM_H_
