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

#include <melon/fiber/fiber.h>
#include <melon/fiber/execution_queue.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/stream.h>
#include <melon/proto/rpc/streaming_rpc_meta.pb.h>

namespace melon {

    class MELON_CACHELINE_ALIGNMENT Stream : public SocketConnection {
    public:
        // |--------------------------------------------------|
        // |----------- Implement SocketConnection -----------|
        // |--------------------------------------------------|

        int Connect(Socket *ptr, const timespec *due_time,
                    int (*on_connect)(int, int, void *), void *data);

        ssize_t CutMessageIntoFileDescriptor(int, mutil::IOBuf **data_list,
                                             size_t size);

        ssize_t CutMessageIntoSSLChannel(SSL *, mutil::IOBuf **, size_t);

        void BeforeRecycle(Socket *);

        // --------------------- SocketConnection --------------

        int AppendIfNotFull(const mutil::IOBuf &msg,
                            const StreamWriteOptions *options = NULL);

        static int Create(const StreamOptions &options,
                          const StreamSettings *remote_settings,
                          StreamId *id);

        StreamId id() { return _id; }

        int OnReceived(const StreamFrameMeta &fm, mutil::IOBuf *buf, Socket *sock);

        void SetRemoteSettings(const StreamSettings &remote_settings) {
            _remote_settings.MergeFrom(remote_settings);
        }

        int SetHostSocket(Socket *host_socket);

        void SetConnected();

        void SetConnected(const StreamSettings *remote_settings);

        void Wait(void (*on_writable)(StreamId, void *, int), void *arg,
                  const timespec *due_time);

        int Wait(const timespec *due_time);

        void FillSettings(StreamSettings *settings);

        static int SetFailed(StreamId id);

        void Close();

    private:
        friend void StreamWait(StreamId stream_id, const timespec *due_time,
                               void (*on_writable)(StreamId, void *, int), void *arg);

        friend class MessageBatcher;

        Stream();

        ~Stream();

        int Init(const StreamOptions options);

        void SetRemoteConsumed(size_t _remote_consumed);

        void TriggerOnConnectIfNeed();

        void Wait(void (*on_writable)(StreamId, void *, int), void *arg,
                  const timespec *due_time, bool new_thread, fiber_session_t *join_id);

        void SendFeedback();

        void StartIdleTimer();

        void StopIdleTimer();

        void HandleRpcResponse(mutil::IOBuf *response_buffer);

        void WriteToHostSocket(mutil::IOBuf *b);

        static int Consume(void *meta, fiber::TaskIterator<mutil::IOBuf *> &iter);

        static int TriggerOnWritable(fiber_session_t id, void *data, int error_code);

        static void *RunOnWritable(void *arg);

        static void *RunOnConnect(void *arg);

        struct ConnectMeta {
            int (*on_connect)(int, int, void *);

            int ec;
            void *arg;
        };

        struct WritableMeta {
            void (*on_writable)(StreamId, void *, int);

            StreamId id;
            void *arg;
            int error_code;
            bool new_thread;
            bool has_timer;
            fiber_timer_t timer;
        };

        Socket *_host_socket;  // Every stream within a Socket holds a reference
        Socket *_fake_socket_weak_ref;  // Not holding reference
        StreamId _id;
        StreamOptions _options;

        fiber_mutex_t _connect_mutex;
        ConnectMeta _connect_meta;
        bool _connected;
        bool _closed;

        fiber_mutex_t _congestion_control_mutex;
        size_t _produced;
        size_t _remote_consumed;
        size_t _cur_buf_size;
        fiber_session_list_t _writable_wait_list;

        int64_t _local_consumed;
        StreamSettings _remote_settings;

        bool _parse_rpc_response;
        fiber::ExecutionQueueId<mutil::IOBuf *> _consumer_queue;
        mutil::IOBuf *_pending_buf;
        int64_t _start_idle_timer_us;
        fiber_timer_t _idle_timer;
    };

} // namespace melon