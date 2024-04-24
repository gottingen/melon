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



#ifndef MELON_RPC_DUMP_RPC_DUMP_H_
#define MELON_RPC_DUMP_RPC_DUMP_H_

#include <gflags/gflags_declare.h>
#include "melon/utility/iobuf.h"                            // IOBuf
#include "melon/utility/files/file_path.h"                  // FilePath
#include "melon/var/collector.h"
#include "melon/proto/rpc/rpc_dump.pb.h"                       // RpcDumpMeta

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
