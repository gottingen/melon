// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#include "melon/rpc/stream.h"

#include <gflags/gflags.h>
#include "melon/times/time.h"
#include "melon/memory/object_pool.h"
#include <memory>
#include "melon/fiber/internal/unstable.h"
#include "melon/rpc/log.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/input_messenger.h"
#include "melon/rpc/policy/streaming_rpc_protocol.h"
#include "melon/rpc/policy/baidu_rpc_protocol.h"
#include "melon/rpc/stream_impl.h"


namespace melon::rpc {

    DECLARE_bool(usercode_in_pthread);

    const static melon::cord_buf *TIMEOUT_TASK = (melon::cord_buf *) -1L;

    Stream::Stream()
            : _host_socket(NULL), _fake_socket_weak_ref(NULL), _connected(false), _closed(false), _produced(0),
              _remote_consumed(0), _local_consumed(0), _parse_rpc_response(false), _pending_buf(NULL),
              _start_idle_timer_us(0), _idle_timer(0) {
        _connect_meta.on_connect = NULL;
        MELON_CHECK_EQ(0, fiber_mutex_init(&_connect_mutex, NULL));
        MELON_CHECK_EQ(0, fiber_mutex_init(&_congestion_control_mutex, NULL));
    }

    Stream::~Stream() {
        MELON_CHECK(_host_socket == NULL);
        fiber_mutex_destroy(&_connect_mutex);
        fiber_mutex_destroy(&_congestion_control_mutex);
        fiber_token_list_destroy(&_writable_wait_list);
    }

    int Stream::Create(const StreamOptions &options,
                       const StreamSettings *remote_settings,
                       StreamId *id) {
        Stream *s = new Stream();
        s->_host_socket = NULL;
        s->_fake_socket_weak_ref = NULL;
        s->_connected = false;
        s->_options = options;
        s->_closed = false;
        if (remote_settings != NULL) {
            s->_remote_settings.MergeFrom(*remote_settings);
            s->_parse_rpc_response = false;
        } else {
            s->_parse_rpc_response = true;
        }
        if (fiber_token_list_init(&s->_writable_wait_list, 8, 8/*FIXME*/)) {
            delete s;
            return -1;
        }
        melon::fiber_internal::ExecutionQueueOptions q_opt;
        q_opt.fiber_attr
                = FLAGS_usercode_in_pthread ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL;
        if (melon::fiber_internal::execution_queue_start(&s->_consumer_queue, &q_opt, Consume, s) != 0) {
            MELON_LOG(FATAL) << "Fail to create ExecutionQueue";
            delete s;
            return -1;
        }
        SocketOptions sock_opt;
        sock_opt.conn = s;
        SocketId fake_sock_id;
        if (Socket::Create(sock_opt, &fake_sock_id) != 0) {
            s->BeforeRecycle(NULL);
            return -1;
        }
        SocketUniquePtr ptr;
        MELON_CHECK_EQ(0, Socket::Address(fake_sock_id, &ptr));
        s->_fake_socket_weak_ref = ptr.get();
        s->_id = fake_sock_id;
        *id = s->id();
        return 0;
    }

    void Stream::BeforeRecycle(Socket *) {
        // No one holds reference now, so we don't need lock here
        fiber_token_list_reset(&_writable_wait_list, ECONNRESET);
        if (_connected) {
            // Send CLOSE frame
            RPC_VLOG << "Send close frame";
            MELON_CHECK(_host_socket != NULL);
            policy::SendStreamClose(_host_socket,
                                    _remote_settings.stream_id(), id());
        }

        if (_host_socket) {
            _host_socket->RemoveStream(id());
        }

        // The instance is to be deleted in the consumer thread
        melon::fiber_internal::execution_queue_stop(_consumer_queue);
    }

    ssize_t Stream::CutMessageIntoFileDescriptor(int /*fd*/,
                                                 melon::cord_buf **data_list,
                                                 size_t size) {
        if (_host_socket == NULL) {
            MELON_CHECK(false) << "Not connected";
            errno = EBADF;
            return -1;
        }
        if (!_remote_settings.writable()) {
            MELON_LOG(WARNING) << "The remote side of Stream=" << id()
                               << "->" << _remote_settings.stream_id()
                               << "@" << _host_socket->remote_side()
                               << " doesn't have a handler";
            errno = EBADF;
            return -1;
        }
        melon::cord_buf out;
        ssize_t len = 0;
        for (size_t i = 0; i < size; ++i) {
            StreamFrameMeta fm;
            fm.set_stream_id(_remote_settings.stream_id());
            fm.set_source_stream_id(id());
            fm.set_frame_type(FRAME_TYPE_DATA);
            // TODO: split large data
            fm.set_has_continuation(false);
            policy::PackStreamMessage(&out, fm, data_list[i]);
            len += data_list[i]->length();
            data_list[i]->clear();
        }
        WriteToHostSocket(&out);
        return len;
    }

    void Stream::WriteToHostSocket(melon::cord_buf *b) {
        MELON_RPC_HANDLE_EOVERCROWDED(_host_socket->Write(b));
    }

    ssize_t Stream::CutMessageIntoSSLChannel(SSL *, melon::cord_buf **, size_t) {
        MELON_CHECK(false) << "Stream does support SSL";
        errno = EINVAL;
        return -1;
    }

    void *Stream::RunOnConnect(void *arg) {
        ConnectMeta *meta = (ConnectMeta *) arg;
        if (meta->ec == 0) {
            meta->on_connect(Socket::STREAM_FAKE_FD, 0, meta->arg);
        } else {
            meta->on_connect(-1, meta->ec, meta->arg);
        }
        delete meta;
        return NULL;
    }

    int Stream::Connect(Socket *ptr, const timespec *,
                        int (*on_connect)(int, int, void *), void *data) {
        MELON_CHECK_EQ(ptr->id(), _id);
        fiber_mutex_lock(&_connect_mutex);
        if (_connect_meta.on_connect != NULL) {
            MELON_CHECK(false) << "Connect is supposed to be called once";
            fiber_mutex_unlock(&_connect_mutex);
            return -1;
        }
        _connect_meta.on_connect = on_connect;
        _connect_meta.arg = data;
        if (_connected) {
            ConnectMeta *meta = new ConnectMeta;
            meta->on_connect = _connect_meta.on_connect;
            meta->arg = _connect_meta.arg;
            meta->ec = _connect_meta.ec;
            fiber_mutex_unlock(&_connect_mutex);
            fiber_id_t tid;
            if (fiber_start_urgent(&tid, &FIBER_ATTR_NORMAL, RunOnConnect, meta) != 0) {
                MELON_LOG(FATAL) << "Fail to start fiber, " << melon_error();
                RunOnConnect(meta);
            }
            return 0;
        }
        fiber_mutex_unlock(&_connect_mutex);
        return 0;
    }

    void Stream::SetConnected() {
        return SetConnected(NULL);
    }

    void Stream::SetConnected(const StreamSettings *remote_settings) {
        fiber_mutex_lock(&_connect_mutex);
        if (_closed) {
            fiber_mutex_unlock(&_connect_mutex);
            return;
        }
        if (_connected) {
            MELON_CHECK(false);
            fiber_mutex_unlock(&_connect_mutex);
            return;
        }
        MELON_CHECK(_host_socket != NULL);
        if (remote_settings != NULL) {
            MELON_CHECK(!_remote_settings.IsInitialized());
            _remote_settings.MergeFrom(*remote_settings);
        } else {
            MELON_CHECK(_remote_settings.IsInitialized());
        }
        MELON_CHECK(_host_socket != NULL);
        RPC_VLOG << "stream=" << id() << " is connected to stream_id="
                 << _remote_settings.stream_id() << " at host_socket=" << *_host_socket;
        _connected = true;
        _connect_meta.ec = 0;
        TriggerOnConnectIfNeed();
        if (remote_settings == NULL) {
            // Start the timer at server-side
            // Client-side timer would triggered in Consume after received the first
            // message which is the very RPC response
            StartIdleTimer();
        }
    }

    void Stream::TriggerOnConnectIfNeed() {
        if (_connect_meta.on_connect != NULL) {
            ConnectMeta *meta = new ConnectMeta;
            meta->on_connect = _connect_meta.on_connect;
            meta->arg = _connect_meta.arg;
            meta->ec = _connect_meta.ec;
            fiber_mutex_unlock(&_connect_mutex);
            fiber_id_t tid;
            if (fiber_start_urgent(&tid, &FIBER_ATTR_NORMAL, RunOnConnect, meta) != 0) {
                MELON_LOG(FATAL) << "Fail to start fiber, " << melon_error();
                RunOnConnect(meta);
            }
            return;
        }
        fiber_mutex_unlock(&_connect_mutex);
    }

    int Stream::AppendIfNotFull(const melon::cord_buf &data) {
        if (_options.max_buf_size > 0) {
            std::unique_lock<fiber_mutex_t> lck(_congestion_control_mutex);
            if (_produced >= _remote_consumed + (size_t) _options.max_buf_size) {
                const size_t saved_produced = _produced;
                const size_t saved_remote_consumed = _remote_consumed;
                lck.unlock();
                RPC_VLOG << "Stream=" << _id << " is full"
                         << "_produced=" << saved_produced
                         << " _remote_consumed=" << saved_remote_consumed
                         << " gap=" << saved_produced - saved_remote_consumed
                         << " max_buf_size=" << _options.max_buf_size;
                return 1;
            }
            _produced += data.length();
        }
        melon::cord_buf copied_data(data);
        const int rc = _fake_socket_weak_ref->Write(&copied_data);
        if (rc != 0) {
            // Stream may be closed by peer before
            MELON_LOG(WARNING) << "Fail to write to _fake_socket, " << melon_error();
            MELON_SCOPED_LOCK(_congestion_control_mutex);
            _produced -= data.length();
            return -1;
        }
        return 0;
    }

    void Stream::SetRemoteConsumed(size_t new_remote_consumed) {
        MELON_CHECK(_options.max_buf_size > 0);
        fiber_token_list_t tmplist;
        fiber_token_list_init(&tmplist, 0, 0);
        fiber_mutex_lock(&_congestion_control_mutex);
        if (_remote_consumed >= new_remote_consumed) {
            fiber_mutex_unlock(&_congestion_control_mutex);
            return;
        }
        const bool was_full = _produced >= _remote_consumed + (size_t) _options.max_buf_size;
        _remote_consumed = new_remote_consumed;
        const bool is_full = _produced >= _remote_consumed + (size_t) _options.max_buf_size;
        if (was_full && !is_full) {
            fiber_token_list_swap(&tmplist, &_writable_wait_list);
        }
        fiber_mutex_unlock(&_congestion_control_mutex);

        // broadcast
        fiber_token_list_reset(&tmplist, 0);
        fiber_token_list_destroy(&tmplist);
    }

    void *Stream::RunOnWritable(void *arg) {
        WritableMeta *wm = (WritableMeta *) arg;
        wm->on_writable(wm->id, wm->arg, wm->error_code);
        delete wm;
        return NULL;
    }

    int Stream::TriggerOnWritable(fiber_token_t id, void *data, int error_code) {
        WritableMeta *wm = (WritableMeta *) data;

        if (wm->has_timer) {
            fiber_timer_del(wm->timer);
        }
        wm->error_code = error_code;
        if (wm->new_thread) {
            const fiber_attribute *attr =
                    FLAGS_usercode_in_pthread ? &FIBER_ATTR_PTHREAD
                                              : &FIBER_ATTR_NORMAL;
            fiber_id_t tid;
            if (fiber_start_background(&tid, attr, RunOnWritable, wm) != 0) {
                MELON_LOG(FATAL) << "Fail to start fiber" << melon_error();
                RunOnWritable(wm);
            }
        } else {
            RunOnWritable(wm);
        }
        return fiber_token_unlock_and_destroy(id);
    }

    void OnTimedOut(void *arg) {
        fiber_token_t id = {reinterpret_cast<uint64_t>(arg)};
        fiber_token_error(id, ETIMEDOUT);
    }

    void Stream::Wait(void (*on_writable)(StreamId, void *, int), void *arg,
                      const timespec *due_time, bool new_thread, fiber_token_t *join_id) {
        WritableMeta *wm = new WritableMeta;
        wm->on_writable = on_writable;
        wm->id = id();
        wm->arg = arg;
        wm->new_thread = new_thread;
        wm->has_timer = false;
        fiber_token_t wait_id;
        const int rc = fiber_token_create(&wait_id, wm, TriggerOnWritable);
        if (rc != 0) {
            MELON_CHECK(false) << "Fail to create fiber_token, " << melon_error(rc);
            wm->error_code = rc;
            RunOnWritable(wm);
            return;
        }
        if (join_id) {
            *join_id = wait_id;
        }
        MELON_CHECK_EQ(0, fiber_token_lock(wait_id, NULL));
        if (due_time != NULL) {
            wm->has_timer = true;
            const int rc = fiber_timer_add(&wm->timer, *due_time,
                                           OnTimedOut,
                                           reinterpret_cast<void *>(wait_id.value));
            if (rc != 0) {
                MELON_LOG(ERROR) << "Fail to add timer, " << melon_error(rc);
                MELON_CHECK_EQ(0, TriggerOnWritable(wait_id, wm, rc));
            }
        }
        fiber_mutex_lock(&_congestion_control_mutex);
        if (_options.max_buf_size <= 0
            || _produced < _remote_consumed + (size_t) _options.max_buf_size) {
            fiber_mutex_unlock(&_congestion_control_mutex);
            MELON_CHECK_EQ(0, TriggerOnWritable(wait_id, wm, 0));
            return;
        } else {
            fiber_token_list_add(&_writable_wait_list, wait_id);
            fiber_mutex_unlock(&_congestion_control_mutex);
        }
        MELON_CHECK_EQ(0, fiber_token_unlock(wait_id));
    }

    void Stream::Wait(void (*on_writable)(StreamId, void *, int), void *arg,
                      const timespec *due_time) {
        return Wait(on_writable, arg, due_time, true, NULL);
    }

    void OnWritable(StreamId, void *arg, int error_code) {
        *(int *) arg = error_code;
    }

    int Stream::Wait(const timespec *due_time) {
        int rc;
        fiber_token_t join_id = INVALID_FIBER_TOKEN;
        Wait(OnWritable, &rc, due_time, false, &join_id);
        if (join_id != INVALID_FIBER_TOKEN) {
            fiber_token_join(join_id);
        }
        return rc;
    }

    int Stream::OnReceived(const StreamFrameMeta &fm, melon::cord_buf *buf, Socket *sock) {
        if (_host_socket == NULL) {
            if (SetHostSocket(sock) != 0) {
                return -1;
            }
        }
        switch (fm.frame_type()) {
            case FRAME_TYPE_FEEDBACK:
                SetRemoteConsumed(fm.feedback().consumed_size());
                MELON_CHECK(buf->empty());
                break;
            case FRAME_TYPE_DATA:
                if (_pending_buf != NULL) {
                    _pending_buf->append(*buf);
                    buf->clear();
                } else {
                    _pending_buf = new melon::cord_buf;
                    _pending_buf->swap(*buf);
                }
                if (!fm.has_continuation()) {
                    melon::cord_buf *tmp = _pending_buf;
                    _pending_buf = NULL;
                    if (melon::fiber_internal::execution_queue_execute(_consumer_queue, tmp) != 0) {
                        MELON_CHECK(false) << "Fail to push into channel";
                        delete tmp;
                        Close();
                    }
                }
                break;
            case FRAME_TYPE_RST:
                RPC_VLOG << "stream=" << id() << " recevied rst frame";
                Close();
                break;
            case FRAME_TYPE_CLOSE:
                RPC_VLOG << "stream=" << id() << " recevied close frame";
                // TODO:: See the comments in Consume
                Close();
                break;
            case FRAME_TYPE_UNKNOWN:
                RPC_VLOG << "Received unknown frame";
                return -1;
        }
        return 0;
    }

    class MessageBatcher {
    public:
        MessageBatcher(melon::cord_buf *storage[], size_t cap, Stream *s)
                : _storage(storage), _cap(cap), _size(0), _total_length(0), _s(s) {}

        ~MessageBatcher() { flush(); }

        void flush() {
            if (_size > 0 && _s->_options.handler != NULL) {
                _s->_options.handler->on_received_messages(
                        _s->id(), _storage, _size);
            }
            for (size_t i = 0; i < _size; ++i) {
                delete _storage[i];
            }
            _size = 0;
        }

        void push(melon::cord_buf *buf) {
            if (_size == _cap) {
                flush();
            }
            _storage[_size++] = buf;
            _total_length += buf->length();

        }

        size_t total_length() { return _total_length; }

    private:
        melon::cord_buf **_storage;
        size_t _cap;
        size_t _size;
        size_t _total_length;
        Stream *_s;
    };

    int Stream::Consume(void *meta, melon::fiber_internal::TaskIterator<melon::cord_buf *> &iter) {
        Stream *s = (Stream *) meta;
        s->StopIdleTimer();
        if (iter.is_queue_stopped()) {
            // indicating the queue was closed
            if (s->_host_socket) {
                DereferenceSocket(s->_host_socket);
                s->_host_socket = NULL;
            }
            if (s->_options.handler != NULL) {
                s->_options.handler->on_closed(s->id());
            }
            delete s;
            return 0;
        }
        DEFINE_SMALL_ARRAY(melon::cord_buf*, buf_list, s->_options.messages_in_batch, 256);
        MessageBatcher mb(buf_list, s->_options.messages_in_batch, s);
        bool has_timeout_task = false;
        for (; iter; ++iter) {
            melon::cord_buf *t = *iter;
            if (t == TIMEOUT_TASK) {
                has_timeout_task = true;
            } else {
                if (s->_parse_rpc_response) {
                    s->_parse_rpc_response = false;
                    s->HandleRpcResponse(t);
                } else {
                    mb.push(t);
                }
            }
        }
        if (s->_options.handler != NULL) {
            if (has_timeout_task && mb.total_length() == 0) {
                s->_options.handler->on_idle_timeout(s->id());
            }
        }
        mb.flush();
        if (s->_remote_settings.need_feedback() && mb.total_length() > 0) {
            s->_local_consumed += mb.total_length();
            s->SendFeedback();
        }
        s->StartIdleTimer();
        return 0;
    }

    void Stream::SendFeedback() {
        StreamFrameMeta fm;
        fm.set_frame_type(FRAME_TYPE_FEEDBACK);
        fm.set_stream_id(_remote_settings.stream_id());
        fm.set_source_stream_id(id());
        fm.mutable_feedback()->set_consumed_size(_local_consumed);
        melon::cord_buf out;
        policy::PackStreamMessage(&out, fm, NULL);
        WriteToHostSocket(&out);
    }

    int Stream::SetHostSocket(Socket *host_socket) {
        if (_host_socket != NULL) {
            MELON_CHECK(false) << "SetHostSocket has already been called";
            return -1;
        }
        SocketUniquePtr ptr;
        host_socket->ReAddress(&ptr);
        // TODO add *this to host socke
        if (ptr->AddStream(id()) != 0) {
            return -1;
        }
        _host_socket = ptr.release();
        return 0;
    }

    void Stream::FillSettings(StreamSettings *settings) {
        settings->set_stream_id(id());
        settings->set_need_feedback(_options.max_buf_size > 0);
        settings->set_writable(_options.handler != NULL);
    }

    void OnIdleTimeout(void *arg) {
        melon::fiber_internal::ExecutionQueueId<melon::cord_buf *> q = {(uint64_t) arg};
        melon::fiber_internal::execution_queue_execute(q, (melon::cord_buf *) TIMEOUT_TASK);
    }

    void Stream::StartIdleTimer() {
        if (_options.idle_timeout_ms < 0) {
            return;
        }
        _start_idle_timer_us = melon::get_current_time_micros();
        timespec due_time = melon::time_point::from_unix_micros(
                _start_idle_timer_us + _options.idle_timeout_ms * 1000).to_timespec();
        const int rc = fiber_timer_add(&_idle_timer, due_time, OnIdleTimeout,
                                       (void *) (_consumer_queue.value));
        MELON_LOG_IF(WARNING, rc != 0) << "Fail to add timer";
    }

    void Stream::StopIdleTimer() {
        if (_options.idle_timeout_ms < 0) {
            return;
        }
        if (_idle_timer != 0) {
            fiber_timer_del(_idle_timer);
        }
    }

    void Stream::Close() {
        _fake_socket_weak_ref->SetFailed();
        fiber_mutex_lock(&_connect_mutex);
        if (_closed) {
            fiber_mutex_unlock(&_connect_mutex);
            return;
        }
        _closed = true;
        if (_connected) {
            fiber_mutex_unlock(&_connect_mutex);
            return;
        }
        _connect_meta.ec = ECONNRESET;
        // Trigger on connect to release the reference of socket
        return TriggerOnConnectIfNeed();
    }

    int Stream::SetFailed(StreamId id) {
        SocketUniquePtr ptr;
        if (Socket::AddressFailedAsWell(id, &ptr) == -1) {
            // Don't care recycled stream
            return 0;
        }
        Stream *s = (Stream *) ptr->conn();
        s->Close();
        return 0;
    }

    void Stream::HandleRpcResponse(melon::cord_buf *response_buffer) {
        MELON_CHECK(!_remote_settings.IsInitialized());
        MELON_CHECK(_host_socket != NULL);
        std::unique_ptr<melon::cord_buf> buf_guard(response_buffer);
        ParseResult pr = policy::ParseRpcMessage(response_buffer, NULL, true, NULL);
        if (!pr.is_ok()) {
            MELON_CHECK(false);
            Close();
            return;
        }
        InputMessageBase *msg = pr.message();
        if (msg == NULL) {
            MELON_CHECK(false);
            Close();
            return;
        }
        _host_socket->PostponeEOF();
        _host_socket->ReAddress(&msg->_socket);
        msg->_received_us = melon::get_current_time_micros();
        msg->_base_real_us = melon::get_current_time_micros();
        msg->_arg = NULL; // ProcessRpcResponse() don't need arg
        policy::ProcessRpcResponse(msg);
    }

    int StreamWrite(StreamId stream_id, const melon::cord_buf &message) {
        SocketUniquePtr ptr;
        if (Socket::Address(stream_id, &ptr) != 0) {
            return EINVAL;
        }
        Stream *s = (Stream *) ptr->conn();
        const int rc = s->AppendIfNotFull(message);
        if (rc == 0) {
            return 0;
        }
        return (rc == 1) ? EAGAIN : errno;
    }

    void StreamWait(StreamId stream_id, const timespec *due_time,
                    void (*on_writable)(StreamId, void *, int), void *arg) {
        SocketUniquePtr ptr;
        if (Socket::Address(stream_id, &ptr) != 0) {
            Stream::WritableMeta *wm = new Stream::WritableMeta;
            wm->id = stream_id;
            wm->arg = arg;
            wm->has_timer = false;
            wm->on_writable = on_writable;
            wm->error_code = EINVAL;
            const fiber_attribute *attr =
                    FLAGS_usercode_in_pthread ? &FIBER_ATTR_PTHREAD
                                              : &FIBER_ATTR_NORMAL;
            fiber_id_t tid;
            if (fiber_start_background(&tid, attr, Stream::RunOnWritable, wm) != 0) {
                MELON_PLOG(FATAL) << "Fail to start fiber";
                Stream::RunOnWritable(wm);
            }
            return;
        }
        Stream *s = (Stream *) ptr->conn();
        return s->Wait(on_writable, arg, due_time);
    }

    int StreamWait(StreamId stream_id, const timespec *due_time) {
        SocketUniquePtr ptr;
        if (Socket::Address(stream_id, &ptr) != 0) {
            return EINVAL;
        }
        Stream *s = (Stream *) ptr->conn();
        return s->Wait(due_time);
    }

    int StreamClose(StreamId stream_id) {
        return Stream::SetFailed(stream_id);
    }

    int StreamCreate(StreamId *request_stream, Controller &cntl,
                     const StreamOptions *options) {
        if (cntl._request_stream != INVALID_STREAM_ID) {
            MELON_LOG(ERROR) << "Can't create request stream more than once";
            return -1;
        }
        if (request_stream == NULL) {
            MELON_LOG(ERROR) << "request_stream is NULL";
            return -1;
        }
        StreamId stream_id;
        StreamOptions opt;
        if (options != NULL) {
            opt = *options;
        }
        if (Stream::Create(opt, NULL, &stream_id) != 0) {
            MELON_LOG(ERROR) << "Fail to create stream";
            return -1;
        }
        cntl._request_stream = stream_id;
        *request_stream = stream_id;
        return 0;
    }

    int StreamAccept(StreamId *response_stream, Controller &cntl,
                     const StreamOptions *options) {

        if (cntl._response_stream != INVALID_STREAM_ID) {
            MELON_LOG(ERROR) << "Can't create reponse stream more than once";
            return -1;
        }
        if (response_stream == NULL) {
            MELON_LOG(ERROR) << "response_stream is NULL";
            return -1;
        }
        if (!cntl.has_remote_stream()) {
            MELON_LOG(ERROR) << "No stream along with this request";
            return -1;
        }
        StreamOptions opt;
        if (options != NULL) {
            opt = *options;
        }
        StreamId stream_id;
        if (Stream::Create(opt, cntl._remote_stream_settings, &stream_id) != 0) {
            MELON_LOG(ERROR) << "Fail to create stream";
            return -1;
        }
        cntl._response_stream = stream_id;
        *response_stream = stream_id;
        return 0;
    }

} // namespace melon::rpc
