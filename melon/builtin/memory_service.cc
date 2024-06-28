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


#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/builtin/memory_service.h>
#include <melon/rpc/details/tcmalloc_extension.h>
#include <turbo/flags/flag.h>

TURBO_FLAG(int, max_tc_stats_buf_len, 32 * 1024, "max length of TCMalloc stats").on_validate([](std::string_view value, std::string *err) noexcept ->bool {
    if(value.empty()) {
        *err = "max_tc_stats_buf_len must be a positive integer";
        return false;
    }
    int len;
    auto r = turbo::parse_flag(value, &len, err);
    if(!r) {
        return false;
    }
    if(len <= 0) {
        if(err)
            *err = "max_tc_stats_buf_len must be a positive integer";
        return false;
    }
    return true;
});

namespace melon {

    static inline void get_tcmalloc_num_prop(MallocExtension *malloc_ext,
                                             const char *prop_name,
                                             mutil::IOBufBuilder &os) {
        size_t value;
        if (malloc_ext->GetNumericProperty(prop_name, &value)) {
            os << prop_name << ": " << value << "\n";
        }
    }

    static void get_tcmalloc_memory_info(mutil::IOBuf &out) {
        MallocExtension *malloc_ext = MallocExtension::instance();
        mutil::IOBufBuilder os;
        os << "------------------------------------------------\n";
        get_tcmalloc_num_prop(malloc_ext, "generic.total_physical_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "generic.current_allocated_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "generic.heap_size", os);
        get_tcmalloc_num_prop(malloc_ext, "tcmalloc.current_total_thread_cache_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "tcmalloc.central_cache_free_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "tcmalloc.transfer_cache_free_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "tcmalloc.thread_cache_free_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "tcmalloc.pageheap_free_bytes", os);
        get_tcmalloc_num_prop(malloc_ext, "tcmalloc.pageheap_unmapped_bytes", os);

        int32_t len = turbo::get_flag(FLAGS_max_tc_stats_buf_len);
        std::unique_ptr<char[]> buf(new char[len]);
        malloc_ext->GetStats(buf.get(), len);
        os << buf.get();

        os.move_to(out);
    }

    void MemoryService::default_method(::google::protobuf::RpcController *cntl_base,
                                       const ::melon::MemoryRequest *,
                                       ::melon::MemoryResponse *,
                                       ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        auto cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        mutil::IOBuf &resp = cntl->response_attachment();

        if (IsTCMallocEnabled()) {
            mutil::IOBufBuilder os;
            get_tcmalloc_memory_info(resp);
        } else {
            resp.append("tcmalloc is not enabled");
            cntl->http_response().set_status_code(HTTP_STATUS_FORBIDDEN);
            return;
        }
    }

} // namespace melon
