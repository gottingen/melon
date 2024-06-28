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

#include <melon/raft/remote_file_copier.h>
#include <turbo/flags/flag.h>
#include <melon/utility/strings/string_piece.h>
#include <melon/utility/strings/string_number_conversions.h>
#include <melon/utility/files/file_path.h>
#include <melon/utility/file_util.h>
#include <melon/fiber/fiber.h>
#include <melon/rpc/controller.h>
#include <melon/raft/util.h>
#include <melon/raft/snapshot.h>
#include <melon/raft/config.h>

TURBO_FLAG(int32_t, raft_max_byte_count_per_rpc, 1024 * 128 /*128K*/,
           "Maximum of block size per RPC").on_validate(turbo::GtValidator<int32_t, 1>::validate);
TURBO_FLAG(bool, raft_allow_read_partly_when_install_snapshot, true,
           "Whether allowing read snapshot data partly").on_validate(turbo::AllPassValidator<bool>::validate);
TURBO_FLAG(bool, raft_enable_throttle_when_install_snapshot, true,
           "enable throttle when install snapshot, for both leader and follower").on_validate(
        turbo::AllPassValidator<bool>::validate);

namespace melon::raft {

    RemoteFileCopier::RemoteFileCopier()
            : _reader_id(0), _throttle(nullptr) {}

    int RemoteFileCopier::init(const std::string &uri, FileSystemAdaptor *fs,
                               SnapshotThrottle *throttle) {
        // Parse uri format: remote://ip:port/reader_id
        static const size_t prefix_size = strlen("remote://");
        mutil::StringPiece uri_str(uri);
        if (!uri_str.starts_with("remote://")) {
            LOG(ERROR) << "Invalid uri=" << uri;
            return -1;
        }
        uri_str.remove_prefix(prefix_size);
        size_t slash_pos = uri_str.find('/');
        mutil::StringPiece ip_and_port = uri_str.substr(0, slash_pos);
        uri_str.remove_prefix(slash_pos + 1);
        if (!mutil::StringToInt64(uri_str, &_reader_id)) {
            LOG(ERROR) << "Invalid reader_id_format=" << uri_str
                       << " in " << uri;
            return -1;
        }
        melon::ChannelOptions channel_opt;
        channel_opt.connect_timeout_ms = turbo::get_flag(FLAGS_raft_rpc_channel_connect_timeout_ms);
        if (_channel.Init(ip_and_port.as_string().c_str(), &channel_opt) != 0) {
            LOG(ERROR) << "Fail to init Channel to " << ip_and_port;
            return -1;
        }
        _fs = fs;
        _throttle = throttle;
        return 0;
    }

    int RemoteFileCopier::read_piece_of_file(
            mutil::IOBuf *buf,
            const std::string &source,
            off_t offset,
            size_t max_count,
            long timeout_ms,
            bool *is_eof) {
        melon::Controller cntl;
        GetFileRequest request;
        request.set_reader_id(_reader_id);
        request.set_filename(source);
        request.set_count(max_count);
        request.set_offset(offset);
        GetFileResponse response;
        FileService_Stub stub(&_channel);
        cntl.set_timeout_ms(timeout_ms);
        stub.get_file(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) {
            LOG(WARNING) << "Fail to issue RPC, " << cntl.ErrorText();
            return cntl.ErrorCode();
        }
        *is_eof = response.eof();
        buf->swap(cntl.response_attachment());
        return 0;
    }

    int RemoteFileCopier::copy_to_file(const std::string &source,
                                       const std::string &dest_path,
                                       const CopyOptions *options) {
        scoped_refptr<Session> session = start_to_copy_to_file(
                source, dest_path, options);
        if (session == nullptr) {
            return -1;
        }
        session->join();
        return session->status().error_code();
    }

    int RemoteFileCopier::copy_to_iobuf(const std::string &source,
                                        mutil::IOBuf *dest_buf,
                                        const CopyOptions *options) {
        scoped_refptr<Session> session = start_to_copy_to_iobuf(
                source, dest_buf, options);
        if (session == nullptr) {
            return -1;
        }
        session->join();
        return session->status().error_code();
    }

    scoped_refptr<RemoteFileCopier::Session>
    RemoteFileCopier::start_to_copy_to_file(
            const std::string &source,
            const std::string &dest_path,
            const CopyOptions *options) {
        mutil::File::Error e;
        FileAdaptor *file = _fs->open(dest_path, O_TRUNC | O_WRONLY | O_CREAT | O_CLOEXEC, nullptr, &e);

        if (!file) {
            LOG(ERROR) << "Fail to open " << dest_path
                       << ", " << mutil::File::ErrorToString(e);
            return nullptr;
        }

        scoped_refptr<Session> session(new Session());
        session->_dest_path = dest_path;
        session->_file = file;
        session->_request.set_filename(source);
        session->_request.set_reader_id(_reader_id);
        session->_channel = &_channel;
        if (options) {
            session->_options = *options;
        }
        // pass throttle to Session
        if (_throttle) {
            session->_throttle = _throttle;
        }
        session->send_next_rpc();
        return session;
    }

    scoped_refptr<RemoteFileCopier::Session>
    RemoteFileCopier::start_to_copy_to_iobuf(
            const std::string &source,
            mutil::IOBuf *dest_buf,
            const CopyOptions *options) {
        dest_buf->clear();
        scoped_refptr<Session> session(new Session());
        session->_file = nullptr;
        session->_buf = dest_buf;
        session->_request.set_filename(source);
        session->_request.set_reader_id(_reader_id);
        session->_channel = &_channel;
        if (options) {
            session->_options = *options;
        }
        session->send_next_rpc();
        return session;
    }

    RemoteFileCopier::Session::Session()
            : _channel(nullptr), _file(nullptr), _retry_times(0), _finished(false), _buf(nullptr), _timer(), _throttle(nullptr),
              _throttle_token_acquire_time_us(1) {
        _done.owner = this;
    }

    RemoteFileCopier::Session::~Session() {
        if (_file) {
            _file->close();
            delete _file;
            _file = nullptr;
        }
    }

    void RemoteFileCopier::Session::send_next_rpc() {
        _cntl.Reset();
        _response.Clear();
        // Not clear request as we need some fields of the previous RPC
        off_t offset = _request.offset() + _request.count();
        const size_t max_count =
                (!_buf) ? turbo::get_flag(FLAGS_raft_max_byte_count_per_rpc) : UINT_MAX;
        _cntl.set_timeout_ms(_options.timeout_ms);
        _request.set_offset(offset);
        // Read partly when throttled
        _request.set_read_partly(turbo::get_flag(FLAGS_raft_allow_read_partly_when_install_snapshot));
        std::unique_lock<raft_mutex_t> lck(_mutex);
        if (_finished) {
            return;
        }
        // throttle
        size_t new_max_count = max_count;
        if (_throttle && turbo::get_flag(FLAGS_raft_enable_throttle_when_install_snapshot)) {
            _throttle_token_acquire_time_us = mutil::cpuwide_time_us();
            new_max_count = _throttle->throttled_by_throughput(max_count);
            if (new_max_count == 0) {
                // Reset count to make next rpc retry the previous one
                BRAFT_VLOG << "Copy file throttled, path: " << _dest_path;
                _request.set_count(0);
                AddRef();
                int64_t retry_interval_ms_when_throttled =
                        _throttle->get_retry_interval_ms();
                if (fiber_timer_add(
                        &_timer,
                        mutil::milliseconds_from_now(retry_interval_ms_when_throttled),
                        on_timer, this) != 0) {
                    lck.unlock();
                    LOG(ERROR) << "Fail to add timer";
                    return on_timer(this);
                }
                return;
            }
        }
        _request.set_count(new_max_count);
        _rpc_call = _cntl.call_id();
        FileService_Stub stub(_channel);
        AddRef();  // Release in on_rpc_returned
        return stub.get_file(&_cntl, &_request, &_response, &_done);
    }

    void RemoteFileCopier::Session::on_rpc_returned() {
        scoped_refptr<Session> ref_gurad;
        Session *this_ref = this;
        ref_gurad.swap(&this_ref);
        std::unique_lock<raft_mutex_t> lck(_mutex);
        if (_finished) {
            return;
        }
        if (_cntl.Failed()) {
            // Reset count to make next rpc retry the previous one
            int64_t request_count = _request.count();
            _request.set_count(0);
            if (_cntl.ErrorCode() == ECANCELED) {
                if (_st.ok()) {
                    _st.set_error(_cntl.ErrorCode(), _cntl.ErrorText());
                    return on_finished();
                }
            }
            // Throttled reading failure does not increase _retry_times
            if (_cntl.ErrorCode() != EAGAIN && _retry_times++ >= _options.max_retry) {
                if (_st.ok()) {
                    _st.set_error(_cntl.ErrorCode(), _cntl.ErrorText());
                    return on_finished();
                }
            }
            // set retry time interval
            int64_t retry_interval_ms = _options.retry_interval_ms;
            if (_cntl.ErrorCode() == EAGAIN && _throttle) {
                retry_interval_ms = _throttle->get_retry_interval_ms();
                // No token consumed, just return back, other nodes maybe able to use them
                if (turbo::get_flag(FLAGS_raft_enable_throttle_when_install_snapshot)) {
                    _throttle->return_unused_throughput(
                            request_count, 0,
                            mutil::cpuwide_time_us() - _throttle_token_acquire_time_us);
                }
            }
            AddRef();
            if (fiber_timer_add(
                    &_timer,
                    mutil::milliseconds_from_now(retry_interval_ms),
                    on_timer, this) != 0) {
                lck.unlock();
                LOG(ERROR) << "Fail to add timer";
                return on_timer(this);
            }
            return;
        }
        if (_throttle && turbo::get_flag(FLAGS_raft_enable_throttle_when_install_snapshot) &&
            _request.count() > (int64_t) _cntl.response_attachment().size()) {
            _throttle->return_unused_throughput(
                    _request.count(), _cntl.response_attachment().size(),
                    mutil::cpuwide_time_us() - _throttle_token_acquire_time_us);
        }
        _retry_times = 0;
        // Reset count to |real_read_size| to make next rpc get the right offset
        if (_response.has_read_size() && (_response.read_size() != 0)
            && turbo::get_flag(FLAGS_raft_allow_read_partly_when_install_snapshot)) {
            _request.set_count(_response.read_size());
        }
        if (_file) {
            FileSegData data(_cntl.response_attachment());
            uint64_t seg_offset = 0;
            mutil::IOBuf seg_data;
            while (0 != data.next(&seg_offset, &seg_data)) {
                ssize_t nwritten = _file->write(seg_data, seg_offset);
                if (static_cast<size_t>(nwritten) != seg_data.size()) {
                    LOG(WARNING) << "Fail to write into file: " << _dest_path;
                    _st.set_error(EIO, "%s", berror(EIO));
                    return on_finished();
                }
                seg_data.clear();
            }
        } else {
            FileSegData data(_cntl.response_attachment());
            uint64_t seg_offset = 0;
            mutil::IOBuf seg_data;
            while (0 != data.next(&seg_offset, &seg_data)) {
                CHECK_GE((size_t) seg_offset, _buf->length());
                _buf->resize(seg_offset);
                _buf->append(seg_data);
            }
        }
        if (_response.eof()) {
            on_finished();
            return;
        }
        lck.unlock();
        return send_next_rpc();
    }

    void *RemoteFileCopier::Session::send_next_rpc_on_timedout(void *arg) {
        Session *m = (Session *) arg;
        m->send_next_rpc();
        m->Release();
        return nullptr;
    }

    void RemoteFileCopier::Session::on_timer(void *arg) {
        fiber_t tid;
        if (fiber_start_background(
                &tid, nullptr, send_next_rpc_on_timedout, arg) != 0) {
            PLOG(ERROR) << "Fail to start fiber";
            send_next_rpc_on_timedout(arg);
        }
    }

    void RemoteFileCopier::Session::on_finished() {
        if (!_finished) {
            if (_file) {
                if (!_file->sync() || !_file->close()) {
                    _st.set_error(EIO, "%s", berror(EIO));
                }
                delete _file;
                _file = nullptr;
            }
            _finished = true;
            _finish_event.signal();
        }
    }

    void RemoteFileCopier::Session::cancel() {
        MELON_SCOPED_LOCK(_mutex);
        if (_finished) {
            return;
        }
        melon::StartCancel(_rpc_call);
        if (fiber_timer_del(_timer) == 0) {
            // Release reference of the timer task
            Release();
        }
        if (_st.ok()) {
            _st.set_error(ECANCELED, "%s", berror(ECANCELED));
        }
        on_finished();
    }

    void RemoteFileCopier::Session::join() {
        _finish_event.wait();
    }

} //  namespace melon::raft
