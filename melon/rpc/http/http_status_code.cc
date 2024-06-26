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



#include <stdio.h>                                  // snprintf

#include <turbo/log/logging.h>                           // MELON_*
#include <melon/utility/macros.h>                            // ARRAY_SIZE
#include <melon/utility/thread_local.h>                      // thread_local
#include <melon/proto/rpc/errno.pb.h>
#include <melon/rpc/http/http_status_code.h>


namespace melon {

static struct status_pair{
    int status_code;
    const char *reason_phrase;
} status_pairs[] = { 
    // Informational 1xx   
    { HTTP_STATUS_CONTINUE,                         "Continue"              },
    { HTTP_STATUS_SWITCHING_PROTOCOLS,              "Switching Protocols"   },

    // Successful 2xx
    { HTTP_STATUS_OK,                               "OK"                    },
    { HTTP_STATUS_CREATED,                          "Created"               },
    { HTTP_STATUS_ACCEPTED,                         "Accepted"              },
    { HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION,    "Non-Authoritative Informational" },
    { HTTP_STATUS_NO_CONTENT,                       "No Content"            },
    { HTTP_STATUS_RESET_CONTENT,                    "Reset Content"         },
    { HTTP_STATUS_PARTIAL_CONTENT,                  "Partial Content"       },

    // Redirection 3xx
    { HTTP_STATUS_MULTIPLE_CHOICES,                 "Multiple Choices"      },
    { HTTP_STATUS_MOVE_PERMANENTLY,                 "Move Permanently"      },
    { HTTP_STATUS_FOUND,                            "Found"                 },
    { HTTP_STATUS_SEE_OTHER,                        "See Other"             },
    { HTTP_STATUS_NOT_MODIFIED,                     "Not Modified"          },
    { HTTP_STATUS_USE_PROXY,                        "Use Proxy"             },
    { HTTP_STATUS_TEMPORARY_REDIRECT,               "Temporary Redirect"    },

    // Client Error 4xx
    { HTTP_STATUS_BAD_REQUEST,                      "Bad Request"           },
    { HTTP_STATUS_UNAUTHORIZED,                     "Unauthorized"          },
    { HTTP_STATUS_PAYMENT_REQUIRED,                 "Payment Required"      },
    { HTTP_STATUS_FORBIDDEN,                        "Forbidden"             },
    { HTTP_STATUS_NOT_FOUND,                        "Not Found"             },
    { HTTP_STATUS_METHOD_NOT_ALLOWED,               "Method Not Allowed"    },
    { HTTP_STATUS_NOT_ACCEPTABLE,                   "Not Acceptable"        },
    { HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,    "Proxy Authentication Required" },
    { HTTP_STATUS_REQUEST_TIMEOUT,                  "Request Timeout"       },
    { HTTP_STATUS_CONFLICT,                         "Conflict"              },
    { HTTP_STATUS_GONE,                             "Gone"                  },
    { HTTP_STATUS_LENGTH_REQUIRED,                  "Length Required"       },
    { HTTP_STATUS_PRECONDITION_FAILED,              "Precondition Failed"   },
    { HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE,         "Request Entity Too Large" },
    { HTTP_STATUS_REQUEST_URI_TOO_LARG,             "Request-URI Too Long"  },
    { HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,           "Unsupported Media Type"},
    { HTTP_STATUS_REQUEST_RANGE_NOT_SATISFIABLE,    "Requested Range Not Satisfiable" },
    { HTTP_STATUS_EXPECTATION_FAILED,               "Expectation Failed"    },

    // Server Error 5xx
    { HTTP_STATUS_INTERNAL_SERVER_ERROR,            "Internal Server Error" },
    { HTTP_STATUS_NOT_IMPLEMENTED,                  "Not Implemented"       },
    { HTTP_STATUS_BAD_GATEWAY,                      "Bad Gateway"           },
    { HTTP_STATUS_SERVICE_UNAVAILABLE,              "Service Unavailable"   },
    { HTTP_STATUS_GATEWAY_TIMEOUT,                  "Gateway Timeout"       },
    { HTTP_STATUS_VERSION_NOT_SUPPORTED,            "HTTP Version Not Supported" },
};

static const char *phrases[1024];
static pthread_once_t init_reason_phrases_once = PTHREAD_ONCE_INIT;

static void InitReasonPhrases() {
    memset(phrases, 0, sizeof(phrases));
    for (size_t i = 0; i < ARRAY_SIZE(status_pairs); ++i) {
        if (status_pairs[i].status_code >= 0 &&
            status_pairs[i].status_code < (int)ARRAY_SIZE(phrases)) {
            phrases[status_pairs[i].status_code] = status_pairs[i].reason_phrase;
        } else {
            LOG(FATAL) << "The status_pairs[" << i << "] is invalid"
                        << " status_code=" << status_pairs[i].status_code
                        << " reason_phrase=`" << status_pairs[i].reason_phrase
                        << '\'';
        }
    }
}

static MELON_THREAD_LOCAL char tls_phrase_cache[64];

const char *HttpReasonPhrase(int status_code) {
    pthread_once(&init_reason_phrases_once, InitReasonPhrases);
    const char* desc = NULL;
    if (status_code >= 0 &&
        status_code < (int)ARRAY_SIZE(phrases) &&
        (desc = phrases[status_code])) {
        return desc;
    }
    snprintf(tls_phrase_cache, sizeof(tls_phrase_cache),
             "Unknown status code (%d)", status_code);
    return tls_phrase_cache;
}

int ErrorCodeToStatusCode(int error_code) {
    if (error_code == 0) {
        return HTTP_STATUS_OK;
    }
    switch (error_code) {
    case ENOSERVICE:
    case ENOMETHOD:
        return HTTP_STATUS_NOT_FOUND;
    case ERPCAUTH:
        return HTTP_STATUS_UNAUTHORIZED;
    case EREQUEST:
    case EINVAL:
        return HTTP_STATUS_BAD_REQUEST;
    case ELIMIT:
    case ELOGOFF:
        return HTTP_STATUS_SERVICE_UNAVAILABLE;
    case EPERM:
        return HTTP_STATUS_FORBIDDEN;
    case ERPCTIMEDOUT:
    case ETIMEDOUT:
        return HTTP_STATUS_GATEWAY_TIMEOUT;
    default:
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }
}

} // namespace melon
