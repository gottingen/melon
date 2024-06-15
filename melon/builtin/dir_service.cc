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



#include <ostream>
#include <dirent.h>                    // opendir
#include <fcntl.h>                     // O_RDONLY
#include <melon/base/fd_guard.h>
#include <melon/base/fd_utility.h>

#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/controller.h>           // Controller
#include <melon/builtin/common.h>
#include <melon/builtin/dir_service.h>


namespace melon {

    void DirService::default_method(::google::protobuf::RpcController *cntl_base,
                                    const ::melon::DirRequest *,
                                    ::melon::DirResponse *,
                                    ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        std::string open_path;

        const std::string &path_str =
                cntl->http_request().unresolved_path();
        if (!path_str.empty()) {
            open_path.reserve(path_str.size() + 2);
            open_path.push_back('/');
            open_path.append(path_str);
        } else {
            open_path = "/";
        }
        DIR *dir = opendir(open_path.c_str());
        if (NULL == dir) {
            mutil::fd_guard fd(open(open_path.c_str(), O_RDONLY));
            if (fd < 0) {
                cntl->SetFailed(errno, "Cannot open `%s'", open_path.c_str());
                return;
            }
            mutil::make_non_blocking(fd);
            mutil::make_close_on_exec(fd);

            mutil::IOPortal read_portal;
            size_t total_read = 0;
            do {
                const ssize_t nr = read_portal.append_from_file_descriptor(
                        fd, MAX_READ);
                if (nr < 0) {
                    cntl->SetFailed(errno, "Cannot read `%s'", open_path.c_str());
                    return;
                }
                if (nr == 0) {
                    break;
                }
                total_read += nr;
            } while (total_read < MAX_READ);
            mutil::IOBuf &resp = cntl->response_attachment();
            resp.swap(read_portal);
            if (total_read >= MAX_READ) {
                std::ostringstream oss;
                oss << " <" << lseek(fd, 0, SEEK_END) - total_read << " more bytes>";
                resp.append(oss.str());
            }
            cntl->http_response().set_content_type("text/plain");
        } else {
            const bool use_html = UseHTML(cntl->http_request());
            const mutil::EndPoint *const html_addr = (use_html ? Path::LOCAL : NULL);
            cntl->http_response().set_content_type(
                    use_html ? "text/html" : "text/plain");

            std::vector<std::string> files;
            files.reserve(32);
            // readdir_r is marked as deprecated since glibc 2.24.
#if defined(__GLIBC__) && \
        (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 24))
            for (struct dirent *p = NULL; (p = readdir(dir)) != NULL;) {
#else
                struct dirent entbuf;
                for (struct dirent* p = NULL; readdir_r(dir, &entbuf, &p) == 0 && p; ) {
#endif
                files.push_back(p->d_name);
            }
            CHECK_EQ(0, closedir(dir));

            std::sort(files.begin(), files.end());
            mutil::IOBufBuilder os;
            if (use_html) {
                os << "<!DOCTYPE html><html><body><pre>";
            }
            std::string str1;
            std::string str2;
            for (size_t i = 0; i < files.size(); ++i) {
                if (path_str.empty() && files[i] == "..") {
                    // back to /index
                    os << Path("", html_addr, files[i].c_str()) << '\n';
                } else {
                    str1 = open_path;
                    AppendFileName(&str1, files[i]);
                    str2 = "/dir";
                    str2.append(str1);
                    os << Path(str2.c_str(), html_addr, files[i].c_str()) << '\n';
                }
            }
            if (use_html) {
                os << "</pre></body></html>";
            }
            os.move_to(cntl->response_attachment());
        }
    }


} // namespace melon
