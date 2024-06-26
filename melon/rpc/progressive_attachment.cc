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



#include <turbo/log/logging.h>
#include <melon/fiber/fiber.h>   // INVALID_FIBER_ID before fiber r32748
#include <melon/rpc/progressive_attachment.h>
#include <melon/rpc/socket.h>
#include <melon/proto/rpc/errno.pb.h>


namespace melon {

// defined in socket.cpp
DECLARE_int64(socket_max_unwritten_bytes);

const int ProgressiveAttachment::RPC_RUNNING = 0;
const int ProgressiveAttachment::RPC_SUCCEED = 1;
const int ProgressiveAttachment::RPC_FAILED = 2;

ProgressiveAttachment::ProgressiveAttachment(SocketUniquePtr& movable_httpsock,
                                             bool before_http_1_1)
    : _before_http_1_1(before_http_1_1)
    , _pause_from_mark_rpc_as_done(false)
    , _rpc_state(RPC_RUNNING)
    , _notify_id(INVALID_FIBER_ID) {
    _httpsock.swap(movable_httpsock);
}

ProgressiveAttachment::~ProgressiveAttachment() {
    if (_httpsock) {
        CHECK(_rpc_state.load(mutil::memory_order_relaxed) != RPC_RUNNING);
        CHECK(_saved_buf.empty());
        if (!_before_http_1_1) {
            // note: _httpsock may already be failed.
            if (_rpc_state.load(mutil::memory_order_relaxed) == RPC_SUCCEED) {
                mutil::IOBuf tmpbuf;
                tmpbuf.append("0\r\n\r\n", 5);
                Socket::WriteOptions wopt;
                wopt.ignore_eovercrowded = true;
                _httpsock->Write(&tmpbuf, &wopt);
            }
        } else {
            // Close _httpsock to notify the client that all the content has
            // been transferred.
            // Note: invoke ReleaseAdditionalReference instead of SetFailed to
            // make sure that all the data has been written before the fd is
            // closed.
            _httpsock->ReleaseAdditionalReference();
        }
    }
    if (_notify_id != INVALID_FIBER_ID) {
        fiber_session_error(_notify_id, 0);
    }
}

static char s_hex_map[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
                            '9', 'A', 'B', 'C', 'D', 'E', 'F' };
inline char ToHex(uint32_t size/*0-15*/) { return s_hex_map[size]; }

inline void AppendChunkHead(mutil::IOBuf* buf, uint32_t size) {
    char tmp[32];
    int i = (int)sizeof(tmp);
    tmp[--i] = '\n';
    tmp[--i] = '\r';
    if (size == 0) {
        tmp[--i] = '0';
    } else {
        for (--i; i >= 0; --i) {
            const uint32_t new_size = (size >> 4);
            tmp[i] = ToHex(size - (new_size << 4));
            size = new_size;
            if (size == 0) {
                --i;
                break;
            }
        }
    }
    buf->append(tmp + i + 1, sizeof(tmp) - i - 1);
}

inline void AppendAsChunk(mutil::IOBuf* chunk_buf, const mutil::IOBuf& data,
                          bool before_http_1_1) {
    if (!before_http_1_1) {
        AppendChunkHead(chunk_buf, data.size());
        chunk_buf->append(data);
        chunk_buf->append("\r\n", 2);   
    } else {
        chunk_buf->append(data);
    }
}

inline void AppendAsChunk(mutil::IOBuf* chunk_buf, const void* data,
                          size_t length, bool before_http_1_1) {
    if (!before_http_1_1) {
        AppendChunkHead(chunk_buf, length);
        chunk_buf->append(data, length);
        chunk_buf->append("\r\n", 2);   
    } else {
        chunk_buf->append(data, length);
    }
}

int ProgressiveAttachment::Write(const mutil::IOBuf& data) {
    if (data.empty()) {
        LOG_EVERY_N_SEC(WARNING, 1)
            << "Write an empty chunk. To suppress this warning, check emptiness"
            " of the chunk before calling ProgressiveAttachment.Write()";
        return 0;
    }

    int rpc_state = _rpc_state.load(mutil::memory_order_acquire);
    if (rpc_state == RPC_RUNNING) {
        std::unique_lock<mutil::Mutex> mu(_mutex);
        rpc_state = _rpc_state.load(mutil::memory_order_acquire);
        if (rpc_state == RPC_RUNNING) {
            if (_saved_buf.size() >= (size_t)FLAGS_socket_max_unwritten_bytes ||
                _pause_from_mark_rpc_as_done) {
                errno = EOVERCROWDED;
                return -1;
            }
            AppendAsChunk(&_saved_buf, data, _before_http_1_1);
            return 0;
        }
    }
    // The RPC is already done (http headers were written into the socket)
    // write into the socket directly.
    if (rpc_state == RPC_SUCCEED) {
        mutil::IOBuf tmpbuf;
        AppendAsChunk(&tmpbuf, data, _before_http_1_1);
        return _httpsock->Write(&tmpbuf);
    } else {
        errno = ECANCELED;
        return -1;
    }
}

int ProgressiveAttachment::Write(const void* data, size_t n) {
    if (data == NULL || n == 0) {
        LOG_EVERY_N_SEC(WARNING, 1)
            << "Write an empty chunk. To suppress this warning, check emptiness"
            " of the chunk before calling ProgressiveAttachment.Write()";
        return 0;
    }
    int rpc_state = _rpc_state.load(mutil::memory_order_acquire);
    if (rpc_state == RPC_RUNNING) {
        std::unique_lock<mutil::Mutex> mu(_mutex);
        rpc_state = _rpc_state.load(mutil::memory_order_relaxed);
        if (rpc_state == RPC_RUNNING) {
            if (_saved_buf.size() >= (size_t)FLAGS_socket_max_unwritten_bytes ||
                _pause_from_mark_rpc_as_done) {
                errno = EOVERCROWDED;
                return -1;
            }
            AppendAsChunk(&_saved_buf, data, n, _before_http_1_1);
            return 0;
        }
    }
    // The RPC is already done (http headers were written into the socket)
    // write into the socket directly.
    if (rpc_state == RPC_SUCCEED) {
        mutil::IOBuf tmpbuf;
        AppendAsChunk(&tmpbuf, data, n, _before_http_1_1);
        return _httpsock->Write(&tmpbuf);
    } else {
        errno = ECANCELED;
        return -1;
    }
}

void ProgressiveAttachment::MarkRPCAsDone(bool rpc_failed) {
    // Notes:
    // * Writing here is more timely than being flushed in next Write(), in
    //   some extreme situations, the delay before next Write() may be
    //   significant.
    // * Write() should be outside lock because a failed write triggers
    //   SetFailed() which runs the closure to NotifyOnStopped() which may
    //   call methods requesting for the lock again. Another solution is to
    //   use recursive lock.
    // * _saved_buf can't be much longer than FLAGS_socket_max_unwritten_bytes,
    //   ignoring EOVERCROWDED simplifies the error handling.
    // * If the iteration repeats too many times, _pause_from_mark_rpc_as_done
    //   is set to true to make Write() fails with EOVERCROWDED temporarily to
    //   stop _saved_buf from growing.
    const int MAX_TRY = 3;
    int ntry = 0;
    bool permanent_error = false;
    do {
        std::unique_lock<mutil::Mutex> mu(_mutex);
        if (_saved_buf.empty() || permanent_error || rpc_failed) {
            mutil::IOBuf tmp;
            tmp.swap(_saved_buf); // Clear _saved_buf outside lock.
            _pause_from_mark_rpc_as_done = false;
            _rpc_state.store((rpc_failed? RPC_FAILED: RPC_SUCCEED),
                             mutil::memory_order_release);
            mu.unlock();
            return;
        }
        if (++ntry > MAX_TRY) {
            _pause_from_mark_rpc_as_done = true;
        }
        mutil::IOBuf copied;
        copied.swap(_saved_buf);
        mu.unlock();
        Socket::WriteOptions wopt;
        wopt.ignore_eovercrowded = true;
        if (_httpsock->Write(&copied, &wopt) != 0) {
            permanent_error = true;
        }
    } while (true);
}

mutil::EndPoint ProgressiveAttachment::remote_side() const {
    return _httpsock ? _httpsock->remote_side() : mutil::EndPoint();
}

mutil::EndPoint ProgressiveAttachment::local_side() const {
    return _httpsock ? _httpsock->local_side() : mutil::EndPoint();
}

static int RunOnFailed(fiber_session_t id, void* data, int) {
    fiber_session_unlock_and_destroy(id);
    static_cast<google::protobuf::Closure*>(data)->Run();
    return 0;
}

void ProgressiveAttachment::NotifyOnStopped(google::protobuf::Closure* done) {
    if (done == NULL) {
        LOG(ERROR) << "Param[done] is NULL";
        return;
    }
    if (_notify_id != INVALID_FIBER_ID) {
        LOG(ERROR) << "NotifyOnStopped() can only be called once";
        return done->Run();
    }
    if (_httpsock == NULL) {
        return done->Run();
    }
    const int rc = fiber_session_create(&_notify_id, done, RunOnFailed);
    if (rc) {
        LOG(ERROR) << "Fail to create _notify_id: " << berror(rc);
        return done->Run();
    }
    _httpsock->NotifyOnFailed(_notify_id);
}
    
} // namespace melon
