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



#ifndef MELON_RPC_DUMP_RPC_DUMP_H_
#define MELON_RPC_DUMP_RPC_DUMP_H_

#include <gflags/gflags_declare.h>
#include <melon/base/iobuf.h>                            // IOBuf
#include <melon/utility/files/file_path.h>                  // FilePath
#include <melon/var/collector.h>
#include <melon/proto/rpc/rpc_dump.pb.h>                       // RpcDumpMeta

namespace mutil {
    class FileEnumerator;
}

namespace melon {

    DECLARE_bool(rpc_dump);

    // Randomly take samples of all requests and write into a file in batch in
    // a background thread.

    // Example:
    //   SampledRequest* sample = AskToBeSampled();
    //   If (sample) {
    //     sample->xxx = yyy;
    //     sample->request = ...;
    //     sample->Submit();
    //   }
    //
    // In practice, sampled requests are just small fraction of all requests.
    // The overhead of sampling should be negligible for overall performance.

    class SampledRequest : public melon::var::Collected {
    public:
        mutil::IOBuf request;
        RpcDumpMeta meta;

        // Implement methods of Sampled.
        void dump_and_destroy(size_t round) override;

        void destroy() override;

        melon::var::CollectorSpeedLimit *speed_limit() override {
            extern melon::var::CollectorSpeedLimit g_rpc_dump_sl;
            return &g_rpc_dump_sl;
        }
    };

    // If this function returns non-NULL, the caller must fill the returned
    // object and submit it for later dumping by calling SubmitSample(). If
    // the caller ignores non-NULL return value, the object is leaked.
    inline SampledRequest *AskToBeSampled() {
        extern melon::var::CollectorSpeedLimit g_rpc_dump_sl;
        if (!FLAGS_rpc_dump || !melon::var::is_collectable(&g_rpc_dump_sl)) {
            return NULL;
        }
        return new(std::nothrow) SampledRequest;
    }

    // Read samples from dumped files in a directory.
    // Example:
    //   SampleIterator it("./rpc_dump_echo_server");
    //   for (SampledRequest* req = it->Next(); req != NULL; req = it->Next()) {
    //     ...
    //   }
    class SampleIterator {
    public:
        explicit SampleIterator(const mutil::StringPiece &dir);

        ~SampleIterator();

        // Read a sample. Order of samples are not guaranteed to be same with
        // the order that they're stored in dumped files.
        // Returns the sample which should be deleted by caller. NULL means
        // all dumped files are read.
        SampledRequest *Next();

    private:
        // Parse on request from the buf. Set `format_error' to true when
        // the buf does not match the format.
        static SampledRequest *Pop(mutil::IOBuf &buf, bool *format_error);

        mutil::IOPortal _cur_buf;
        int _cur_fd;
        mutil::FileEnumerator *_enum;
        mutil::FilePath _dir;
    };

} // namespace melon


#endif  // MELON_RPC_DUMP_RPC_DUMP_H_
