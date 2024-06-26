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


#pragma once

#include <vector>                           // std::vector
#include <gflags/gflags_declare.h>
#include <melon/utility/endpoint.h>
#include <melon/rpc/http/http_header.h>


namespace melon {

    class Controller;

    // These static strings are referenced more than once in melon.
    // Don't turn them to std::strings whose constructing sequences are undefined.
    const char *const UNKNOWN_METHOD_STR = "unknown_method";
    const char *const TRACE_ID_STR = "trace";
    const char *const SPAN_ID_STR = "span";
    const char *const TIME_STR = "time";
    const char *const MAX_SCAN_STR = "max_scan";
    const char *const MIN_LATENCY_STR = "min_latency";
    const char *const MIN_REQUEST_SIZE_STR = "min_request_size";
    const char *const MIN_RESPONSE_SIZE_STR = "min_response_size";
    const char *const LOG_ID_STR = "log_id";
    const char *const ERROR_CODE_STR = "error_code";
    const char *const CONSOLE_STR = "console";
    const char *const USER_AGENT_STR = "user-agent";
    const char *const SETVALUE_STR = "setvalue";

    const size_t MAX_READ = 1024 * 1024;

    enum ProfilingType {
        PROFILING_CPU = 0,
        PROFILING_HEAP = 1,
        PROFILING_GROWTH = 2,
        PROFILING_CONTENTION = 3,
    };

    DECLARE_string(rpc_profiling_dir);

    bool UseHTML(const HttpHeader &header);

    bool MatchAnyWildcard(const std::string &name,
                          const std::vector<std::string> &wildcards);

    void PrintRealDateTime(std::ostream &os, int64_t tm);

    void PrintRealDateTime(std::ostream &os, int64_t tm, bool ignore_microseconds);

    struct PrintedAsDateTime {
        PrintedAsDateTime(int64_t realtime2) : realtime(realtime2) {}

        int64_t realtime;
    };

    std::ostream &operator<<(std::ostream &os, const PrintedAsDateTime &);

    struct Path {
        static const mutil::EndPoint *LOCAL;

        Path(const char *uri2, const mutil::EndPoint *html_addr2)
                : uri(uri2), html_addr(html_addr2), text(NULL) {}

        Path(const char *uri2, const mutil::EndPoint *html_addr2, const char *text2)
                : uri(uri2), html_addr(html_addr2), text(text2) {}

        const char *uri;
        const mutil::EndPoint *html_addr;
        const char *text;
    };

    std::ostream &operator<<(std::ostream &os, const Path &link);

    // Append `filename' to `dir' according to unix directory rules:
    //   "foo/bar" + ".."     -> "foo"
    //   "foo/bar/." + ".."   -> "foo"
    //   "foo" + "."          -> "foo"
    //   "foo/" + ".."        -> ""
    //   "foo/../" + ".."     -> ".."
    //   "/foo/../" + ".."    -> "/"
    //   "foo/./" + ".."      -> ""
    void AppendFileName(std::string *dir, const std::string &filename);

    // style of class=gridtable, wrapped with <style>
    const char *gridtable_style();

    // Put inside <head></head> of html to work with Tabbed.
    const char *TabsHead();

    // The logo ascii art.
    const char *logo();

    // Convert ProfilingType to its description.
    const char *ProfilingType2String(ProfilingType t);

    // Compute 128-bit checksum of the file at `file_path'.
    // Return 0 on success.
    int FileChecksum(const char *file_path, unsigned char *checksum);

    // Get name of current program.
    const char *GetProgramName();

    // Get checksum of current program image.
    const char *GetProgramChecksum();

    // True if the http requester support gzip compression.
    bool SupportGzip(Controller *cntl);

    void Time2GMT(time_t t, char *buf, size_t size);

    template<typename T>
    struct MinWidth {
        MinWidth(const T &obj2, size_t nspace2) : obj(&obj2), nspace(nspace2) {}

        const T *obj;
        size_t nspace;
    };

    template<typename T>
    MinWidth<T> min_width(const T &obj, size_t nspace) {
        return MinWidth<T>(obj, nspace);
    }

    template<typename T>
    inline std::ostream &operator<<(std::ostream &os, const MinWidth<T> &fw) {
        const std::streampos old_pos = os.tellp();
        os << *fw.obj;
        for (size_t i = os.tellp() - old_pos; i < fw.nspace; ++i) {
            os << ' ';
        }
        return os;
    }

} // namespace melon
