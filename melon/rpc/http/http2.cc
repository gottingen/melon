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


#include <limits>
#include <turbo/log/logging.h>
#include <melon/rpc/http/hpack.h>
#include <melon/proto/rpc/errno.pb.h>
#include <melon/rpc/http/http2.h>

namespace melon {

H2Settings::H2Settings()
    : header_table_size(DEFAULT_HEADER_TABLE_SIZE)
    , enable_push(false)
    , max_concurrent_streams(std::numeric_limits<uint32_t>::max())
    , stream_window_size(256 * 1024)
    , connection_window_size(1024 * 1024)
    , max_frame_size(DEFAULT_MAX_FRAME_SIZE)
    , max_header_list_size(std::numeric_limits<uint32_t>::max()) {
}

bool H2Settings::IsValid(bool log_error) const {
    if (stream_window_size > MAX_WINDOW_SIZE) {
        LOG_IF(ERROR, log_error) << "Invalid stream_window_size=" << stream_window_size;
        return false;
    }
    if (connection_window_size < DEFAULT_INITIAL_WINDOW_SIZE ||
        connection_window_size > MAX_WINDOW_SIZE) {
        LOG_IF(ERROR, log_error) << "Invalid connection_window_size=" << connection_window_size;
        return false;
    }
    if (max_frame_size < DEFAULT_MAX_FRAME_SIZE ||
        max_frame_size > MAX_OF_MAX_FRAME_SIZE) {
        LOG_IF(ERROR, log_error) << "Invalid max_frame_size=" << max_frame_size;
        return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const H2Settings& s) {
    os << "{header_table_size=" << s.header_table_size
       << " enable_push=" << s.enable_push
       << " max_concurrent_streams=" << s.max_concurrent_streams
       << " stream_window_size=" << s.stream_window_size;
    if (s.connection_window_size > 0) {
        os << " conn_window_size=" << s.connection_window_size;
    }
    os << " max_frame_size=" << s.max_frame_size
       << " max_header_list_size=" << s.max_header_list_size
       << '}';
    return os;
}

const char* H2ErrorToString(H2Error e) {
    switch (e) {
    case H2_NO_ERROR: return "NO_ERROR";
    case H2_PROTOCOL_ERROR: return "PROTOCOL_ERROR";
    case H2_INTERNAL_ERROR: return "INTERNAL_ERROR";
    case H2_FLOW_CONTROL_ERROR: return "FLOW_CONTROL_ERROR";
    case H2_SETTINGS_TIMEOUT: return "SETTINGS_TIMEOUT";
    case H2_STREAM_CLOSED_ERROR: return "STREAM_CLOSED";
    case H2_FRAME_SIZE_ERROR: return "FRAME_SIZE_ERROR";
    case H2_REFUSED_STREAM: return "REFUSED_STREAM";
    case H2_CANCEL: return "CANCEL";
    case H2_COMPRESSION_ERROR: return "COMPRESSION_ERROR";
    case H2_CONNECT_ERROR: return "CONNECT_ERROR";
    case H2_ENHANCE_YOUR_CALM: return "ENHANCE_YOUR_CALM";
    case H2_INADEQUATE_SECURITY: return "INADEQUATE_SECURITY";
    case H2_HTTP_1_1_REQUIRED: return "HTTP_1_1_REQUIRED";
    }
    return "Unknown-H2Error";
}

int H2ErrorToStatusCode(H2Error e) {
    switch (e) {
    case H2_NO_ERROR:
        return HTTP_STATUS_OK;
    case H2_SETTINGS_TIMEOUT:
        return HTTP_STATUS_GATEWAY_TIMEOUT;
    case H2_STREAM_CLOSED_ERROR:
        return HTTP_STATUS_BAD_REQUEST;
    case H2_REFUSED_STREAM:
    case H2_CANCEL:
    case H2_ENHANCE_YOUR_CALM:
        return HTTP_STATUS_SERVICE_UNAVAILABLE;
    case H2_INADEQUATE_SECURITY:
        return HTTP_STATUS_UNAUTHORIZED;
    case H2_HTTP_1_1_REQUIRED:
        return HTTP_STATUS_VERSION_NOT_SUPPORTED;
    case H2_PROTOCOL_ERROR:
    case H2_FLOW_CONTROL_ERROR:
    case H2_FRAME_SIZE_ERROR:
    case H2_COMPRESSION_ERROR:
    case H2_CONNECT_ERROR:
    case H2_INTERNAL_ERROR:
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }
    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
}

} // namespace melon
