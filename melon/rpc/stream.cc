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



#include <melon/rpc/stream.h>

#include <gflags/gflags.h>
#include <melon/utility/time.h>
#include <melon/utility/object_pool.h>
#include <melon/utility/unique_ptr.h>
#include <melon/fiber/unstable.h>
#include <melon/rpc/log.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/input_messenger.h>
#include <melon/rpc/policy/streaming_rpc_protocol.h>
#include <melon/rpc/policy/melon_rpc_protocol.h>
#include <melon/rpc/stream_impl.h>


namespace melon {

DECLARE_bool(usercode_in_pthread);
DECLARE_int64(socket_max_streams_unconsumed_bytes);

const static mutil::IOBuf *TIMEOUT_TASK = (mutil::IOBuf*)-1L;

Stream::Stream() 
    : _host_socket(NULL)
    , _fake_socket_weak_ref(NULL)
    , _connected(false)
    , _closed(false)
    , _produced(0)
    , _remote_consumed(0)
    , _cur_buf_size(0)
    , _local_consumed(0)
    , _parse_rpc_response(false)
    , _pending_buf(NULL)
    , _start_idle_timer_us(0)
    , _idle_timer(0)
{
    _connect_meta.on_connect = NULL;
    CHECK_EQ(0, fiber_mutex_init(&_connect_mutex, NULL));
    CHECK_EQ(0, fiber_mutex_init(&_congestion_control_mutex, NULL));
}

Stream::~Stream() {
    CHECK(_host_socket == NULL);
    fiber_mutex_destroy(&_connect_mutex);
    fiber_mutex_destroy(&_congestion_control_mutex);
    fiber_session_list_destroy(&_writable_wait_list);
}

int Stream::Create(const StreamOptions &options, 
                   const StreamSettings *remote_settings,
                   StreamId *id) {
    Stream* s = new Stream();
    s->_host_socket = NULL;
    s->_fake_socket_weak_ref = NULL;
    s->_connected = false;
    s->_options = options;
    s->_closed = false;
    s->_cur_buf_size = options.max_buf_size > 0 ? options.max_buf_size : 0;
    if (options.max_buf_size > 0 && options.min_buf_size > options.max_buf_size) {
        // set 0 if min_buf_size is invalid.
        s->_options.min_buf_size = 0;
        LOG(WARNING) << "options.min_buf_size is larger than options.max_buf_size, it will be set to 0.";
    }
    if (FLAGS_socket_max_streams_unconsumed_bytes > 0 && s->_options.min_buf_size > 0) {
        s->_cur_buf_size = s->_options.min_buf_size;
    }

    if (remote_settings != NULL) {
        s->_remote_settings.MergeFrom(*remote_settings);
        s->_parse_rpc_response = false;
    } else {
        s->_parse_rpc_response = true;
    }
    if (fiber_session_list_init(&s->_writable_wait_list, 8, 8/*FIXME*/)) {
        delete s;
        return -1;
    }
    fiber::ExecutionQueueOptions q_opt;
    q_opt.fiber_attr
        = FLAGS_usercode_in_pthread ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL;
    if (fiber::execution_queue_start(&s->_consumer_queue, &q_opt, Consume, s) != 0) {
        LOG(FATAL) << "Fail to create ExecutionQueue";
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
    CHECK_EQ(0, Socket::Address(fake_sock_id, &ptr));
    s->_fake_socket_weak_ref = ptr.get();
    s->_id = fake_sock_id;
    *id = s->id();
    return 0;
}

void Stream::BeforeRecycle(Socket *) {
    // No one holds reference now, so we don't need lock here
    fiber_session_list_reset(&_writable_wait_list, ECONNRESET);
    if (_connected) {
        // Send CLOSE frame
        RPC_VLOG << "Send close frame";
        CHECK(_host_socket != NULL);
        policy::SendStreamClose(_host_socket,
                                _remote_settings.stream_id(), id());
    }

    if (_host_socket) {
        _host_socket->RemoveStream(id());
    }
    
    // The instance is to be deleted in the consumer thread
    fiber::execution_queue_stop(_consumer_queue);
}

ssize_t Stream::CutMessageIntoFileDescriptor(int /*fd*/, 
                                             mutil::IOBuf **data_list,
                                             size_t size) {
    if (_host_socket == NULL) {
        CHECK(false) << "Not connected";
        errno = EBADF;
        return -1;
    }
    if (!_remote_settings.writable()) {
        LOG(WARNING) << "The remote side of Stream=" << id()
                     << "->" << _remote_settings.stream_id()
                     << "@" << _host_socket->remote_side()
                     << " doesn't have a handler";
        errno = EBADF;
        return -1;
    }
    mutil::IOBuf out;
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

void Stream::WriteToHostSocket(mutil::IOBuf* b) {
    MELON_HANDLE_EOVERCROWDED(_host_socket->Write(b));
}

ssize_t Stream::CutMessageIntoSSLChannel(SSL*, mutil::IOBuf**, size_t) {
    CHECK(false) << "Stream does support SSL";
    errno = EINVAL;
    return -1;
}

void* Stream::RunOnConnect(void *arg) {
    ConnectMeta* meta = (ConnectMeta*)arg;
    if (meta->ec == 0) {
        meta->on_connect(Socket::STREAM_FAKE_FD, 0, meta->arg);
    } else {
        meta->on_connect(-1, meta->ec, meta->arg);
    }
    delete meta;
    return NULL;
}

int Stream::Connect(Socket* ptr, const timespec*,
                    int (*on_connect)(int, int, void *), void *data) {
    CHECK_EQ(ptr->id(), _id);
    fiber_mutex_lock(&_connect_mutex);
    if (_connect_meta.on_connect != NULL) {
        CHECK(false) << "Connect is supposed to be called once";
        fiber_mutex_unlock(&_connect_mutex);
        return -1;
    }
    _connect_meta.on_connect = on_connect;
    _connect_meta.arg = data;
    if (_connected) {
        ConnectMeta* meta = new ConnectMeta;
        meta->on_connect = _connect_meta.on_connect;
        meta->arg = _connect_meta.arg;
        meta->ec = _connect_meta.ec;
        fiber_mutex_unlock(&_connect_mutex);
        fiber_t tid;
        if (fiber_start_urgent(&tid, &FIBER_ATTR_NORMAL, RunOnConnect, meta) != 0) {
            LOG(FATAL) << "Fail to start fiber, " << berror();
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

void Stream::SetConnected(const StreamSettings* remote_settings) {
    fiber_mutex_lock(&_connect_mutex);
    if (_closed) {
        fiber_mutex_unlock(&_connect_mutex);
        return;
    }
    if (_connected) {
        CHECK(false);
        fiber_mutex_unlock(&_connect_mutex);
        return;
    }
    CHECK(_host_socket != NULL);
    if (remote_settings != NULL) {
        CHECK(!_remote_settings.IsInitialized());
        _remote_settings.MergeFrom(*remote_settings);
    } else {
        CHECK(_remote_settings.IsInitialized());
    }
    CHECK(_host_socket != NULL);
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
        ConnectMeta* meta = new ConnectMeta;
        meta->on_connect = _connect_meta.on_connect;
        meta->arg = _connect_meta.arg;
        meta->ec = _connect_meta.ec;
        fiber_mutex_unlock(&_connect_mutex);
        fiber_t tid;
        if (fiber_start_urgent(&tid, &FIBER_ATTR_NORMAL, RunOnConnect, meta) != 0) {
            LOG(FATAL) << "Fail to start fiber, " << berror();
            RunOnConnect(meta);
        }
        return;
    }
    fiber_mutex_unlock(&_connect_mutex);
}

int Stream::AppendIfNotFull(const mutil::IOBuf &data,
                            const StreamWriteOptions* options) {
    if (_cur_buf_size > 0) {
        std::unique_lock<fiber_mutex_t> lck(_congestion_control_mutex);
        if (_produced >= _remote_consumed + _cur_buf_size) {
            const size_t saved_produced = _produced;
            const size_t saved_remote_consumed = _remote_consumed;
            lck.unlock();
            RPC_VLOG << "Stream=" << _id << " is full"
                     << "_produced=" << saved_produced
                     << " _remote_consumed=" << saved_remote_consumed
                     << " gap=" << saved_produced - saved_remote_consumed
                     << " max_buf_size=" << _cur_buf_size;
            return 1;
        }
        _produced += data.length();
    }

    size_t data_length = data.length();
    mutil::IOBuf copied_data(data);
    Socket::WriteOptions wopt;
    wopt.write_in_background = options != NULL && options->write_in_background;
    const int rc = _fake_socket_weak_ref->Write(&copied_data, &wopt);
    if (rc != 0) {
        // Stream may be closed by peer before
        LOG(WARNING) << "Fail to write to _fake_socket, " << berror();
        MELON_SCOPED_LOCK(_congestion_control_mutex);
        _produced -= data_length;
        return -1;
    }
    if (FLAGS_socket_max_streams_unconsumed_bytes > 0) {
        _host_socket->_total_streams_unconsumed_size += data_length;
    }
    return 0;
}

void Stream::SetRemoteConsumed(size_t new_remote_consumed) {
    CHECK(_cur_buf_size > 0);
    fiber_session_list_t tmplist;
    fiber_session_list_init(&tmplist, 0, 0);
    fiber_mutex_lock(&_congestion_control_mutex);
    if (_remote_consumed >= new_remote_consumed) {
        fiber_mutex_unlock(&_congestion_control_mutex);
        return;
    }
    const bool was_full = _produced >= _remote_consumed + _cur_buf_size;

    if (FLAGS_socket_max_streams_unconsumed_bytes > 0) {
        _host_socket->_total_streams_unconsumed_size -= new_remote_consumed - _remote_consumed;
        if (_host_socket->_total_streams_unconsumed_size > FLAGS_socket_max_streams_unconsumed_bytes) {
            if (_options.min_buf_size > 0) {
                _cur_buf_size = _options.min_buf_size;
            } else {
                _cur_buf_size /= 2;
            }
            LOG(INFO) << "stream consumers on socket " << _host_socket->id() << " is crowded, " <<  "cut stream " << id() << " buffer to " << _cur_buf_size;
        } else if (_produced >= new_remote_consumed + _cur_buf_size && (_options.max_buf_size <= 0 || _cur_buf_size < (size_t)_options.max_buf_size)) {
            if (_options.max_buf_size > 0 && _cur_buf_size * 2 > (size_t)_options.max_buf_size) {
                _cur_buf_size = _options.max_buf_size;
            } else {
                _cur_buf_size *= 2;
            }
        }
    }

    _remote_consumed = new_remote_consumed;
    const bool is_full = _produced >= _remote_consumed + _cur_buf_size;
    if (was_full && !is_full) {
        fiber_session_list_swap(&tmplist, &_writable_wait_list);
    }
    fiber_mutex_unlock(&_congestion_control_mutex);

    // broadcast
    fiber_session_list_reset(&tmplist, 0);
    fiber_session_list_destroy(&tmplist);
}

void* Stream::RunOnWritable(void* arg) {
    WritableMeta *wm = (WritableMeta*)arg;
    wm->on_writable(wm->id, wm->arg, wm->error_code);
    delete wm;
    return NULL;
}

int Stream::TriggerOnWritable(fiber_session_t id, void *data, int error_code) {
    WritableMeta *wm = (WritableMeta*)data;
    
    if (wm->has_timer) {
        fiber_timer_del(wm->timer);
    }
    wm->error_code = error_code;
    if (wm->new_thread) {
        const fiber_attr_t* attr =
            FLAGS_usercode_in_pthread ? &FIBER_ATTR_PTHREAD
            : &FIBER_ATTR_NORMAL;
        fiber_t tid;
        if (fiber_start_background(&tid, attr, RunOnWritable, wm) != 0) {
            LOG(FATAL) << "Fail to start fiber" << berror();
            RunOnWritable(wm);
        }
    } else {
        RunOnWritable(wm);
    }
    return fiber_session_unlock_and_destroy(id);
}

void OnTimedOut(void *arg) {
    fiber_session_t id = { reinterpret_cast<uint64_t>(arg) };
    fiber_session_error(id, ETIMEDOUT);
}

void Stream::Wait(void (*on_writable)(StreamId, void*, int), void* arg, 
                  const timespec* due_time, bool new_thread, fiber_session_t *join_id) {
    WritableMeta *wm = new WritableMeta;
    wm->on_writable = on_writable;
    wm->id = id();
    wm->arg = arg;
    wm->new_thread = new_thread;
    wm->has_timer = false;
    fiber_session_t wait_id;
    const int rc = fiber_session_create(&wait_id, wm, TriggerOnWritable);
    if (rc != 0) {
        CHECK(false) << "Fail to create fiber_session, " << berror(rc);
        wm->error_code = rc;
        RunOnWritable(wm);
        return;
    }
    if (join_id) {
        *join_id = wait_id;
    }
    CHECK_EQ(0, fiber_session_lock(wait_id, NULL));
    if (due_time != NULL) {
        wm->has_timer = true;
        const int rc = fiber_timer_add(&wm->timer, *due_time,
                                         OnTimedOut, 
                                         reinterpret_cast<void*>(wait_id.value));
        if (rc != 0) {
            LOG(ERROR) << "Fail to add timer, " << berror(rc);
            CHECK_EQ(0, TriggerOnWritable(wait_id, wm, rc));
        }
    }
    fiber_mutex_lock(&_congestion_control_mutex);
    if (_cur_buf_size <= 0 
            || _produced < _remote_consumed + _cur_buf_size) {
        fiber_mutex_unlock(&_congestion_control_mutex);
        CHECK_EQ(0, TriggerOnWritable(wait_id, wm, 0));
        return;
    } else {
        fiber_session_list_add(&_writable_wait_list, wait_id);
        fiber_mutex_unlock(&_congestion_control_mutex);
    }
    CHECK_EQ(0, fiber_session_unlock(wait_id));
}

void Stream::Wait(void (*on_writable)(StreamId, void *, int), void *arg,
                  const timespec* due_time) {
    return Wait(on_writable, arg, due_time, true, NULL);
}

void OnWritable(StreamId, void *arg, int error_code) {
    *(int*)arg = error_code;
}

int Stream::Wait(const timespec* due_time) {
    int rc;
    fiber_session_t join_id = INVALID_FIBER_ID;
    Wait(OnWritable, &rc, due_time, false, &join_id);
    if (join_id != INVALID_FIBER_ID) {
        fiber_session_join(join_id);
    }
    return rc;
}

int Stream::OnReceived(const StreamFrameMeta& fm, mutil::IOBuf *buf, Socket* sock) {
    if (_host_socket == NULL) {
        if (SetHostSocket(sock) != 0) {
            return -1;
        }
    }
    switch (fm.frame_type()) {
    case FRAME_TYPE_FEEDBACK:
        SetRemoteConsumed(fm.feedback().consumed_size());
        CHECK(buf->empty());
        break;
    case FRAME_TYPE_DATA:
        if (_pending_buf != NULL) {
            _pending_buf->append(*buf);
            buf->clear();
        } else {
            _pending_buf = new mutil::IOBuf;
            _pending_buf->swap(*buf);
        }
        if (!fm.has_continuation()) {
            mutil::IOBuf *tmp = _pending_buf;
            _pending_buf = NULL;
            if (fiber::execution_queue_execute(_consumer_queue, tmp) != 0) {
                CHECK(false) << "Fail to push into channel";
                delete tmp;
                Close();
            }
        }
        break;
    case FRAME_TYPE_RST:
        RPC_VLOG << "stream=" << id() << " received rst frame";
        Close();
        break;
    case FRAME_TYPE_CLOSE:
        RPC_VLOG << "stream=" << id() << " received close frame";
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
    MessageBatcher(mutil::IOBuf* storage[], size_t cap, Stream* s)
        : _storage(storage)
        , _cap(cap)
        , _size(0)
        , _total_length(0)
        , _s(s)
    {}
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
    void push(mutil::IOBuf* buf) {
        if (_size == _cap) {
            flush();
        }
        _storage[_size++] = buf;
        _total_length += buf->length();

    }
    size_t total_length() { return _total_length; }
private:
    mutil::IOBuf** _storage;
    size_t _cap;
    size_t _size;
    size_t _total_length;
    Stream* _s;
};

int Stream::Consume(void *meta, fiber::TaskIterator<mutil::IOBuf*>& iter) {
    Stream* s = (Stream*)meta;
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
    DEFINE_SMALL_ARRAY(mutil::IOBuf*, buf_list, s->_options.messages_in_batch, 256);
    MessageBatcher mb(buf_list, s->_options.messages_in_batch, s);
    bool has_timeout_task = false;
    for (; iter; ++iter) {
        mutil::IOBuf* t= *iter;
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
    mutil::IOBuf out;
    policy::PackStreamMessage(&out, fm, NULL);
    WriteToHostSocket(&out);
}

int Stream::SetHostSocket(Socket *host_socket) {
    if (_host_socket != NULL) {
        CHECK(false) << "SetHostSocket has already been called";
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
    settings->set_need_feedback(_cur_buf_size > 0);
    settings->set_writable(_options.handler != NULL);
}

void OnIdleTimeout(void *arg) {
    fiber::ExecutionQueueId<mutil::IOBuf*> q = { (uint64_t)arg };
    fiber::execution_queue_execute(q, (mutil::IOBuf*)TIMEOUT_TASK);
}

void Stream::StartIdleTimer() {
    if (_options.idle_timeout_ms < 0) {
        return;
    }
    _start_idle_timer_us = mutil::gettimeofday_us();
    timespec due_time = mutil::microseconds_to_timespec(
            _start_idle_timer_us + _options.idle_timeout_ms * 1000);
    const int rc = fiber_timer_add(&_idle_timer, due_time, OnIdleTimeout,
                                     (void*)(_consumer_queue.value));
    LOG_IF(WARNING, rc != 0) << "Fail to add timer";
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
    Stream* s = (Stream*)ptr->conn();
    s->Close();
    return 0;
}

void Stream::HandleRpcResponse(mutil::IOBuf* response_buffer) {
    CHECK(!_remote_settings.IsInitialized());
    CHECK(_host_socket != NULL);
    std::unique_ptr<mutil::IOBuf> buf_guard(response_buffer);
    ParseResult pr = policy::ParseMStdMessage(response_buffer, NULL, true, NULL);
    if (!pr.is_ok()) {
        CHECK(false);
        Close();
        return;
    }
    InputMessageBase* msg = pr.message();
    if (msg == NULL) {
        CHECK(false);
        Close();
        return;
    }
    _host_socket->PostponeEOF();
    _host_socket->ReAddress(&msg->_socket);
    msg->_received_us = mutil::gettimeofday_us();
    msg->_base_real_us = mutil::gettimeofday_us();
    msg->_arg = NULL; // ProcessMStdResponse() don't need arg
    policy::ProcessMStdResponse(msg);
}

int StreamWrite(StreamId stream_id, const mutil::IOBuf &message,
                const StreamWriteOptions* options) {
    SocketUniquePtr ptr;
    if (Socket::Address(stream_id, &ptr) != 0) {
        return EINVAL;
    }
    Stream* s = (Stream*)ptr->conn();
    const int rc = s->AppendIfNotFull(message, options);
    if (rc == 0) {
        return 0;
    }
    return (rc == 1) ? EAGAIN : errno;
}

void StreamWait(StreamId stream_id, const timespec *due_time,
                void (*on_writable)(StreamId, void*, int), void *arg) {
    SocketUniquePtr ptr;
    if (Socket::Address(stream_id, &ptr) != 0) {
        Stream::WritableMeta* wm = new Stream::WritableMeta;
        wm->id = stream_id;
        wm->arg= arg;
        wm->has_timer = false;
        wm->on_writable = on_writable;
        wm->error_code = EINVAL;
        const fiber_attr_t* attr =
            FLAGS_usercode_in_pthread ? &FIBER_ATTR_PTHREAD
            : &FIBER_ATTR_NORMAL;
        fiber_t tid;
        if (fiber_start_background(&tid, attr, Stream::RunOnWritable, wm) != 0) {
            PLOG(FATAL) << "Fail to start fiber";
            Stream::RunOnWritable(wm);
        }
        return;
    }
    Stream* s = (Stream*)ptr->conn();
    return s->Wait(on_writable, arg, due_time);
}

int StreamWait(StreamId stream_id, const timespec* due_time) {
    SocketUniquePtr ptr;
    if (Socket::Address(stream_id, &ptr) != 0) {
        return EINVAL;
    }
    Stream* s = (Stream*)ptr->conn();
    return s->Wait(due_time);
}

int StreamClose(StreamId stream_id) {
    return Stream::SetFailed(stream_id);
}

int StreamCreate(StreamId *request_stream, Controller &cntl,
                 const StreamOptions* options) {
    if (cntl._request_stream != INVALID_STREAM_ID) {
        LOG(ERROR) << "Can't create request stream more than once";
        return -1;
    }
    if (request_stream == NULL) {
        LOG(ERROR) << "request_stream is NULL";
        return -1;
    }
    StreamId stream_id;
    StreamOptions opt;
    if (options != NULL) {
        opt = *options;
    }
    if (Stream::Create(opt, NULL, &stream_id) != 0) {
        LOG(ERROR) << "Fail to create stream";
        return -1;
    }
    cntl._request_stream = stream_id;
    *request_stream = stream_id;
    return 0;
}

int StreamAccept(StreamId* response_stream, Controller &cntl,
                 const StreamOptions* options) {

    if (cntl._response_stream != INVALID_STREAM_ID) {
        LOG(ERROR) << "Can't create response stream more than once";
        return -1;
    }
    if (response_stream == NULL) {
        LOG(ERROR) << "response_stream is NULL";
        return -1;
    }
    if (!cntl.has_remote_stream()) {
        LOG(ERROR) << "No stream along with this request";
        return -1;
    }
    StreamOptions opt;
    if (options != NULL) {
        opt = *options;
    }
    StreamId stream_id;
    if (Stream::Create(opt, cntl._remote_stream_settings, &stream_id) != 0) {
        LOG(ERROR) << "Fail to create stream";
        return -1;
    }
    cntl._response_stream = stream_id;
    *response_stream = stream_id;
    return 0;
}

} // namespace melon
