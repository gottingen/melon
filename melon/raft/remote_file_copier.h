// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef  MELON_RAFT_REMOTE_FILE_COPIER_H_
#define  MELON_RAFT_REMOTE_FILE_COPIER_H_

#include <melon/rpc/channel.h>
#include <melon/fiber/countdown_event.h>
#include "melon/proto/raft/file_service.pb.h"
#include "melon/raft/util.h"
#include "melon/raft/snapshot_throttle.h"

namespace melon::raft {

    DECLARE_bool(raft_enable_throttle_when_install_snapshot);

    struct CopyOptions {
        CopyOptions();

        int max_retry;
        long retry_interval_ms;
        long timeout_ms;
    };

    inline CopyOptions::CopyOptions()
            : max_retry(3), retry_interval_ms(1000)  // 1s
            , timeout_ms(10L * 1000)   // 10s
    {}

    class FileAdaptor;

    class FileSystemAdaptor;

    class LocalSnapshotWriter;

    class RemoteFileCopier {
    public:
        // Stands for a copying session
        class Session : public mutil::RefCountedThreadSafe<Session> {
        public:
            Session();

            ~Session();

            // Cancel the copy process
            void cancel();

            // Wait until this file was copied from the remote reader
            void join();

            const mutil::Status &status() const { return _st; }

        private:
            friend class RemoteFileCopier;

            friend class Closure;

            struct Closure : google::protobuf::Closure {
                void Run() {
                    owner->on_rpc_returned();
                }

                Session *owner;
            };

            void on_rpc_returned();

            void send_next_rpc();

            void on_finished();

            static void on_timer(void *arg);

            static void *send_next_rpc_on_timedout(void *arg);

            raft_mutex_t _mutex;
            mutil::Status _st;
            melon::Channel *_channel;
            std::string _dest_path;
            FileAdaptor *_file;
            int _retry_times;
            bool _finished;
            melon::CallId _rpc_call;
            mutil::IOBuf *_buf;
            fiber_timer_t _timer;
            CopyOptions _options;
            Closure _done;
            melon::Controller _cntl;
            GetFileRequest _request;
            GetFileResponse _response;
            fiber::CountdownEvent _finish_event;
            scoped_refptr<SnapshotThrottle> _throttle;
            int64_t _throttle_token_acquire_time_us;
        };

        RemoteFileCopier();

        int init(const std::string &uri, FileSystemAdaptor *fs,
                 SnapshotThrottle *throttle);

        // Copy `source' from remote to dest
        int copy_to_file(const std::string &source,
                         const std::string &dest_path,
                         const CopyOptions *options);

        int copy_to_iobuf(const std::string &source,
                          mutil::IOBuf *dest_buf,
                          const CopyOptions *options);

        scoped_refptr<Session> start_to_copy_to_file(
                const std::string &source,
                const std::string &dest_path,
                const CopyOptions *options);

        scoped_refptr<Session> start_to_copy_to_iobuf(
                const std::string &source,
                mutil::IOBuf *dest_buf,
                const CopyOptions *options);

    private:
        int read_piece_of_file(mutil::IOBuf *buf, const std::string &source,
                               off_t offset, size_t max_count,
                               long timeout_ms, bool *is_eof);
        DISALLOW_COPY_AND_ASSIGN(RemoteFileCopier);

        melon::Channel _channel;
        int64_t _reader_id;
        scoped_refptr<FileSystemAdaptor> _fs;
        scoped_refptr<SnapshotThrottle> _throttle;
    };

}  //  namespace melon::raft

#endif  // MELON_RAFT_REMOTE_FILE_COPIER_H_
