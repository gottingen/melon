// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_STRINGS_STRING_SPLIT_H_
#define MUTIL_STRINGS_STRING_SPLIT_H_

#include <string>
#include <utility>
#include <vector>

#include "melon/utility/base_export.h"
#include "melon/utility/strings/string16.h"
#include <melon/utility/strings/string_piece.h>

namespace mutil {

// Splits |str| into a vector of strings delimited by |c|, placing the results
// in |r|. If several instances of |c| are contiguous, or if |str| begins with
// or ends with |c|, then an empty string is inserted.
//
// Every substring is trimmed of any leading or trailing white space.
// NOTE: |c| must be in BMP (Basic Multilingual Plane)
MUTIL_EXPORT void SplitString(const string16& str,
                             char16 c,
                             std::vector<string16>* r);
MUTIL_EXPORT void SplitString(const mutil::StringPiece16& str,
                             char16 c,
                             std::vector<mutil::StringPiece16>* r);

// |str| should not be in a multi-byte encoding like Shift-JIS or GBK in which
// the trailing byte of a multi-byte character can be in the ASCII range.
// UTF-8, and other single/multi-byte ASCII-compatible encodings are OK.
// Note: |c| must be in the ASCII range.
MUTIL_EXPORT void SplitString(const std::string& str,
                             char c,
                             std::vector<std::string>* r);
MUTIL_EXPORT void SplitString(const mutil::StringPiece& str,
                             char c,
                             std::vector<mutil::StringPiece>* r);

typedef std::vector<std::pair<std::string, std::string> > StringPairs;
typedef std::vector<std::pair<mutil::StringPiece, mutil::StringPiece> > StringPiecePairs;

// Splits |line| into key value pairs according to the given delimiters and
// removes whitespace leading each key and trailing each value. Returns true
// only if each pair has a non-empty key and value. |key_value_pairs| will
// include ("","") pairs for entries without |key_value_delimiter|.
MUTIL_EXPORT bool SplitStringIntoKeyValuePairs(const std::string& line,
                                              char key_value_delimiter,
                                              char key_value_pair_delimiter,
                                              StringPairs* key_value_pairs);
MUTIL_EXPORT bool SplitStringIntoKeyValuePairs(const mutil::StringPiece& line,
                                              char key_value_delimiter,
                                              char key_value_pair_delimiter,
                                              StringPiecePairs* key_value_pairs);

// The same as SplitString, but use a substring delimiter instead of a char.
MUTIL_EXPORT void SplitStringUsingSubstr(const string16& str,
                                        const string16& s,
                                        std::vector<string16>* r);
MUTIL_EXPORT void SplitStringUsingSubstr(const mutil::StringPiece16& str,
                                        const mutil::StringPiece16& s,
                                        std::vector<mutil::StringPiece16>* r);
MUTIL_EXPORT void SplitStringUsingSubstr(const std::string& str,
                                        const std::string& s,
                                        std::vector<std::string>* r);
MUTIL_EXPORT void SplitStringUsingSubstr(const mutil::StringPiece& str,
                                        const mutil::StringPiece& s,
                                        std::vector<mutil::StringPiece>* r);

// The same as SplitString, but don't trim white space.
// NOTE: |c| must be in BMP (Basic Multilingual Plane)
MUTIL_EXPORT void SplitStringDontTrim(const string16& str,
                                     char16 c,
                                     std::vector<string16>* r);
MUTIL_EXPORT void SplitStringDontTrim(const mutil::StringPiece16& str,
                                     char16 c,
                                     std::vector<mutil::StringPiece16>* r);
// |str| should not be in a multi-byte encoding like Shift-JIS or GBK in which
// the trailing byte of a multi-byte character can be in the ASCII range.
// UTF-8, and other single/multi-byte ASCII-compatible encodings are OK.
// Note: |c| must be in the ASCII range.
MUTIL_EXPORT void SplitStringDontTrim(const std::string& str,
                                     char c,
                                     std::vector<std::string>* r);
MUTIL_EXPORT void SplitStringDontTrim(const mutil::StringPiece& str,
                                     char c,
                                     std::vector<mutil::StringPiece>* r);

// WARNING: this uses whitespace as defined by the HTML5 spec. If you need
// a function similar to this but want to trim all types of whitespace, then
// factor this out into a function that takes a string containing the characters
// that are treated as whitespace.
//
// Splits the string along whitespace (where whitespace is the five space
// characters defined by HTML 5). Each contiguous block of non-whitespace
// characters is added to result.
MUTIL_EXPORT void SplitStringAlongWhitespace(const string16& str,
                                            std::vector<string16>* result);
MUTIL_EXPORT void SplitStringAlongWhitespace(const mutil::StringPiece16& str,
                                            std::vector<mutil::StringPiece16>* result);
MUTIL_EXPORT void SplitStringAlongWhitespace(const std::string& str,
                                            std::vector<std::string>* result);
MUTIL_EXPORT void SplitStringAlongWhitespace(const mutil::StringPiece& str,
                                            std::vector<mutil::StringPiece>* result);

}  // namespace mutil

#endif  // MUTIL_STRINGS_STRING_SPLIT_H_
