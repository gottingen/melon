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

#include <melon/utility/iobuf.h>
#include <melon/utility/scoped_generic.h>
#include <melon/rpc/socket_id.h>

namespace melon {

    class Controller;

    typedef SocketId StreamId;
    const StreamId INVALID_STREAM_ID = (StreamId) -1L;

    namespace detail {
        struct StreamIdTraits;
    };

// Auto-closed Stream
    typedef mutil::ScopedGeneric<StreamId, detail::StreamIdTraits> ScopedStream;

    class StreamInputHandler {
    public:
        virtual ~StreamInputHandler() = default;

        virtual int on_received_messages(StreamId id,
                                         mutil::IOBuf *const messages[],
                                         size_t size) = 0;

        virtual void on_idle_timeout(StreamId id) = 0;

        virtual void on_closed(StreamId id) = 0;
    };

    struct StreamOptions {
        StreamOptions()
                : min_buf_size(1024 * 1024), max_buf_size(2 * 1024 * 1024), idle_timeout_ms(-1), messages_in_batch(128),
                  handler(NULL) {}

        // stream max buffer size limit in [min_buf_size, max_buf_size]
        // If |min_buf_size| <= 0, there's no min size limit of buf size
        // default: 1048576 (1M)
        int min_buf_size;

        // The max size of unconsumed data allowed at remote side.
        // If |max_buf_size| <= 0, there's no limit of buf size
        // default: 2097152 (2M)
        int max_buf_size;

        // Notify user when there's no data for at least |idle_timeout_ms|
        // milliseconds since the last time that HandleIdleTimeout or HandleInput
        // finished.
        // default: -1
        long idle_timeout_ms;

        // Maximum messages in batch passed to handler->on_received_messages
        // default: 128
        size_t messages_in_batch;

        // Handle input message, if handler is NULL, the remote side is not allowed to
        // write any message, who will get EBADF on writting
        // default: NULL
        StreamInputHandler *handler;
    };

    struct StreamWriteOptions {
        StreamWriteOptions() : write_in_background(false) {}

        // Write message to socket in background thread.
        // Provides batch write effect and better performance in situations when
        // you are continually issuing lots of StreamWrite or async RPC calls in
        // only one thread. Otherwise, each StreamWrite directly writes message into
        // socket and brings poor performance.
        bool write_in_background;
    };

    // [Called at the client side]
    // Create a stream at client-side along with the |cntl|, which will be connected
    // when receiving the response with a stream from server-side. If |options| is
    // NULL, the stream will be created with default options
    // Return 0 on success, -1 otherwise
    int StreamCreate(StreamId *request_stream, Controller &cntl,
                     const StreamOptions *options);

    // [Called at the server side]
    // Accept the stream. If client didn't create a stream with the request
    // (cntl.has_remote_stream() returns false), this method would fail.
    // Return 0 on success, -1 otherwise.
    int StreamAccept(StreamId *response_stream, Controller &cntl,
                     const StreamOptions *options);

    // Write |message| into |stream_id|. The remote-side handler will received the
    // message by the written order
    // Returns 0 on success, errno otherwise
    // Errno:
    //  - EAGAIN: |stream_id| is created with positive |max_buf_size| and buf size
    //            which the remote side hasn't consumed yet excceeds the number.
    //  - EINVAL: |stream_id| is invalied or has been closed
    int StreamWrite(StreamId stream_id, const mutil::IOBuf &message,
                    const StreamWriteOptions *options = NULL);

    // Write util the pending buffer size is less than |max_buf_size| or orrur
    // occurs
    // Returns 0 on success, errno otherwise
    // Errno:
    //  - ETIMEDOUT: when |due_time| is not NULL and time expired this
    //  - EINVAL: the stream was close during waiting
    int StreamWait(StreamId stream_id, const timespec *due_time);

    // Async wait
    void StreamWait(StreamId stream_id, const timespec *due_time,
                    void (*on_writable)(StreamId stream_id, void *arg,
                                        int error_code),
                    void *arg);

    // Close |stream_id|, after this function is called:
    //  - All the following |StreamWrite| would fail
    //  - |StreamWait| wakes up immediately.
    //  - Both sides |on_closed| would be notifed after all the pending buffers have
    //    been received
    // This function could be called multiple times without side-effects
    int StreamClose(StreamId stream_id);

    namespace detail {

        struct StreamIdTraits {
            inline static StreamId InvalidValue() {
                return INVALID_STREAM_ID;
            };

            static void Free(StreamId f) {
                if (f != INVALID_STREAM_ID) {
                    StreamClose(f);
                }
            }
        };

    }  // namespace detail

} // namespace melon
