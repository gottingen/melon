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

#include <melon/utility/status.h>                          // mutil::Status


namespace melon {

    // Generate the IOBuf to write dynamically, for implementing complex protocols.
    // Used in RTMP and HTTP2 right now.
    class SocketMessage {
    public:
        virtual ~SocketMessage() {}

        // Called once and only once to generate the buffer to write.
        // This object should destroy itself at the end of this method.
        // AppendAndDestroySelf()-s to a Socket are called one by one and the
        // generated data are written into the file descriptor in the same order.
        // AppendAndDestroySelf() are called _after_ completion of connecting
        // including AppConnect.
        // Params:
        //   out  - The buffer to be generated, being empty initially, could
        //          remain empty after being called.
        //   sock - the socket to write. NULL when the message will be abandoned
        // If the status returned is an error, WriteOptions.id_wait (if absent)
        // will be signaled with the error. Other messages are not affected.
        virtual mutil::Status AppendAndDestroySelf(mutil::IOBuf *out, Socket *sock) = 0;

        // Estimated size of the buffer generated by AppendAndDestroySelf()
        virtual size_t EstimatedByteSize() { return 0; }
    };

    namespace details {
        struct SocketMessageDeleter {
            void operator()(SocketMessage *msg) const {
                mutil::IOBuf dummy_buf;
                // We don't care about the return value since the message is abandoned
                (void) msg->AppendAndDestroySelf(&dummy_buf, NULL);
            }
        };
    }

// A RAII pointer to make sure SocketMessage.AppendAndDestroySelf() is always
// called even if the message is rejected by Socket.Write()
// Property: Any SocketMessagePtr<T> can be casted to SocketMessagePtr<> which
// is accepted by Socket.Write()
    template<typename T = void>
    struct SocketMessagePtr;

    template<>
    struct SocketMessagePtr<void>
            : public std::unique_ptr<SocketMessage, details::SocketMessageDeleter> {
        SocketMessagePtr() {}

        SocketMessagePtr(SocketMessage *p)
                : std::unique_ptr<SocketMessage, details::SocketMessageDeleter>(p) {}
    };

    template<typename T>
    struct SocketMessagePtr : public SocketMessagePtr<> {
        SocketMessagePtr() {}

        SocketMessagePtr(T *p) : SocketMessagePtr<>(p) {}

        T &operator*() { return static_cast<T &>(SocketMessagePtr<>::operator*()); }

        T *operator->() { return static_cast<T *>(SocketMessagePtr<>::operator->()); }

        T *release() { return static_cast<T *>(SocketMessagePtr<>::release()); }
    };

} // namespace melon

