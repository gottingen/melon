
#ifndef MELON_STRINGS_STRING_SPLITTER_H_
#define MELON_STRINGS_STRING_SPLITTER_H_

#include <cstdlib>
#include <stdint.h>
#include <string_view>
#include "melon/strings/safe_substr.h"
// It's common to encode data into strings separated by special characters
// and decode them back, but functions such as `split_string' has to modify
// the input string, which is bad. If we parse the string from scratch, the
// code will be filled with pointer operations and obscure to understand.
//
// What we want is:
// - Scan the string once: just do simple things efficiently.
// - Do not modify input string: Changing input is bad, it may bring hidden
//   bugs, concurrency issues and non-const propagations.
// - Split the string in-place without additional buffer/array.
//
// StringSplitter does meet these requirements.
// Usage:
//     const char* the_string_to_split = ...;
//     for (StringSplitter s(the_string_to_split, '\t'); s; ++s) {
//         printf("%*s\n", s.length(), s.field());    
//     }
// 
// "s" behaves as an iterator and evaluates to true before ending.
// "s.field()" and "s.length()" are address and length of current field
// respectively. Notice that "s.field()" may not end with '\0' because
// we don't modify input. You can copy the field to a dedicated buffer
// or apply a function supporting length.

namespace melon {

    enum EmptyFieldAction {
        SKIP_EMPTY_FIELD,
        ALLOW_EMPTY_FIELD
    };

    // Split a string with one character
    class StringSplitter {
    public:
        // Split `input' with `separator'. If `action' is SKIP_EMPTY_FIELD, zero-
        // length() field() will be skipped.
        inline StringSplitter(const char *input, char separator,
                              EmptyFieldAction action = SKIP_EMPTY_FIELD);

        // Allows containing embedded '\0' characters and separator can be '\0',
        // if str_end is not NULL.
        inline StringSplitter(const char *str_begin, const char *str_end,
                              char separator,
                              EmptyFieldAction action = SKIP_EMPTY_FIELD);

        // Allows containing embedded '\0' characters and separator can be '\0',
        inline StringSplitter(const std::string_view &input, char separator,
                              EmptyFieldAction action = SKIP_EMPTY_FIELD);

        // Move splitter forward.
        inline StringSplitter &operator++();

        inline StringSplitter operator++(int);

        // True iff field() is valid.
        inline operator const void *() const;

        // Beginning address and length of the field. *(field() + length()) may
        // not be '\0' because we don't modify `input'.
        inline const char *field() const;

        inline size_t length() const;

        inline std::string_view field_sp() const;

        // Cast field to specific type, and write the value into `pv'.
        // Returns 0 on success, -1 otherwise.
        // NOTE: If separator is a digit, casting functions always return -1.
        inline int to_int8(int8_t *pv) const;

        inline int to_uint8(uint8_t *pv) const;

        inline int to_int(int *pv) const;

        inline int to_uint(unsigned int *pv) const;

        inline int to_long(long *pv) const;

        inline int to_ulong(unsigned long *pv) const;

        inline int to_longlong(long long *pv) const;

        inline int to_ulonglong(unsigned long long *pv) const;

        inline int to_float(float *pv) const;

        inline int to_double(double *pv) const;

    private:
        inline bool not_end(const char *p) const;

        inline void init();

        const char *_head;
        const char *_tail;
        const char *_str_tail;
        const char _sep;
        const EmptyFieldAction _empty_field_action;
    };

// Split a string with one of the separators
    class StringMultiSplitter {
    public:
        // Split `input' with one character of `separators'. If `action' is
        // SKIP_EMPTY_FIELD, zero-length() field() will be skipped.
        // NOTE: This utility stores pointer of `separators' directly rather than
        //       copying the content because this utility is intended to be used
        //       in ad-hoc manner where lifetime of `separators' is generally
        //       longer than this utility.
        inline StringMultiSplitter(const char *input, const char *separators,
                                   EmptyFieldAction action = SKIP_EMPTY_FIELD);

        // Allows containing embedded '\0' characters if str_end is not NULL.
        // NOTE: `separators` cannot contain embedded '\0' character.
        inline StringMultiSplitter(const char *str_begin, const char *str_end,
                                   const char *separators,
                                   EmptyFieldAction action = SKIP_EMPTY_FIELD);

        // Move splitter forward.
        inline StringMultiSplitter &operator++();

        inline StringMultiSplitter operator++(int);

        // True iff field() is valid.
        inline operator const void *() const;

        // Beginning address and length of the field. *(field() + length()) may
        // not be '\0' because we don't modify `input'.
        inline const char *field() const;

        inline size_t length() const;

        inline std::string_view field_sp() const;

        // Cast field to specific type, and write the value into `pv'.
        // Returns 0 on success, -1 otherwise.
        // NOTE: If separators contains digit, casting functions always return -1.
        inline int to_int8(int8_t *pv) const;

        inline int to_uint8(uint8_t *pv) const;

        inline int to_int(int *pv) const;

        inline int to_uint(unsigned int *pv) const;

        inline int to_long(long *pv) const;

        inline int to_ulong(unsigned long *pv) const;

        inline int to_longlong(long long *pv) const;

        inline int to_ulonglong(unsigned long long *pv) const;

        inline int to_float(float *pv) const;

        inline int to_double(double *pv) const;

    private:
        inline bool is_sep(char c) const;

        inline bool not_end(const char *p) const;

        inline void init();

        const char *_head;
        const char *_tail;
        const char *_str_tail;
        const char *const _seps;
        const EmptyFieldAction _empty_field_action;
    };

    // Split query in the format according to the given delimiters.
    // This class can also handle some exceptional cases.
    // 1. consecutive pair_delimiter are omitted, for example,
    //    suppose key_value_delimiter is '=' and pair_delimiter
    //    is '&', then 'k1=v1&&&k2=v2' is normalized to 'k1=k2&k2=v2'.
    // 2. key or value can be empty or both can be empty.
    // 3. consecutive key_value_delimiter are not omitted, for example,
    //    suppose input is 'k1===v2' and key_value_delimiter is '=', then
    //    key() returns 'k1', value() returns '==v2'.
    class KeyValuePairsSplitter {
    public:
        inline KeyValuePairsSplitter(const char *str_begin,
                                     const char *str_end,
                                     char pair_delimiter,
                                     char key_value_delimiter)
                : _sp(str_begin, str_end, pair_delimiter), _delim_pos(std::string_view::npos),
                  _key_value_delim(key_value_delimiter) {
            UpdateDelimiterPosition();
        }

        inline KeyValuePairsSplitter(const char *str_begin,
                                     char pair_delimiter,
                                     char key_value_delimiter)
                : KeyValuePairsSplitter(str_begin, NULL,
                                        pair_delimiter, key_value_delimiter) {}

        inline KeyValuePairsSplitter(const std::string_view &sp,
                                     char pair_delimiter,
                                     char key_value_delimiter)
                : KeyValuePairsSplitter(sp.begin(), sp.end(),
                                        pair_delimiter, key_value_delimiter) {}

        inline std::string_view key() {
            return melon::safe_substr(key_and_value(), 0, _delim_pos);
        }

        inline std::string_view value() {
            return melon::safe_substr(key_and_value(), _delim_pos + 1);
        }

        // Get the current value of key and value
        // in the format of "key=value"
        inline std::string_view key_and_value() {
            return std::string_view(_sp.field(), _sp.length());
        }

        // Move splitter forward.
        inline KeyValuePairsSplitter &operator++() {
            ++_sp;
            UpdateDelimiterPosition();
            return *this;
        }

        inline KeyValuePairsSplitter operator++(int) {
            KeyValuePairsSplitter tmp = *this;
            operator++();
            return tmp;
        }

        inline operator const void *() const { return _sp; }

    private:
        inline void UpdateDelimiterPosition();

    private:
        StringSplitter _sp;
        std::string_view::size_type _delim_pos;
        const char _key_value_delim;
    };

}  // namespace melon

#include "melon/strings/string_splitter_inl.h"

#endif  // MELON_STRINGS_STRING_SPLITTER_H_
