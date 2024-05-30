//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//



#ifndef MELON_RPC_PARSE_RESULT_H_
#define MELON_RPC_PARSE_RESULT_H_


namespace melon {

    enum ParseError {
        PARSE_OK = 0,
        PARSE_ERROR_TRY_OTHERS,
        PARSE_ERROR_NOT_ENOUGH_DATA,
        PARSE_ERROR_TOO_BIG_DATA,
        PARSE_ERROR_NO_RESOURCE,
        PARSE_ERROR_ABSOLUTELY_WRONG,
    };

    inline const char *ParseErrorToString(ParseError e) {
        switch (e) {
            case PARSE_OK:
                return "ok";
            case PARSE_ERROR_TRY_OTHERS:
                return "try other protocols";
            case PARSE_ERROR_NOT_ENOUGH_DATA:
                return "not enough data";
            case PARSE_ERROR_TOO_BIG_DATA:
                return "too big data";
            case PARSE_ERROR_NO_RESOURCE:
                return "no resource for the message";
            case PARSE_ERROR_ABSOLUTELY_WRONG:
                return "absolutely wrong message";
        }
        return "unknown ParseError";
    }

    class InputMessageBase;

    // A specialized Maybe<> type to represent a parsing result.
    class ParseResult {
    public:
        // Create a failed parsing result.
        explicit ParseResult(ParseError err)
                : _msg(NULL), _err(err), _user_desc(NULL) {}

        // The `user_desc' must be string constant or always valid.
        explicit ParseResult(ParseError err, const char *user_desc)
                : _msg(NULL), _err(err), _user_desc(user_desc) {}

        // Create a successful parsing result.
        explicit ParseResult(InputMessageBase *msg)
                : _msg(msg), _err(PARSE_OK), _user_desc(NULL) {}

        // Return PARSE_OK when the result is successful.
        ParseError error() const { return _err; }

        const char *error_str() const { return _user_desc ? _user_desc : ParseErrorToString(_err); }

        bool is_ok() const { return error() == PARSE_OK; }

        // definitely NULL when result is failed.
        InputMessageBase *message() const { return _msg; }

    private:
        InputMessageBase *_msg;
        ParseError _err;
        const char *_user_desc;
    };

    // Wrap ParseError/message into ParseResult.
    // You can also call ctor of ParseError directly.
    inline ParseResult MakeParseError(ParseError err) {
        return ParseResult(err);
    }

    // The `user_desc' must be string constant or always valid.
    inline ParseResult MakeParseError(ParseError err, const char *user_desc) {
        return ParseResult(err, user_desc);
    }

    inline ParseResult MakeMessage(InputMessageBase *msg) {
        return ParseResult(msg);
    }

} // namespace melon


#endif  // MELON_RPC_PARSE_RESULT_H_
