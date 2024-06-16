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



#include <melon/base/macros.h>                           // ARRAY_SIZE
#include <melon/base/iobuf.h>                            // mutil::IOBuf
#include <melon/rpc/controller.h>                   // Controller
#include <melon/builtin/get_favicon_service.h>


namespace melon {

    static unsigned char s_favicon_array[] = {
            71, 73, 70, 56, 57, 97, 16, 0, 16, 0, 241, 0, 0, 0, 0, 0, 153, 153, 153, 255,
            255, 255, 0, 0, 0, 33, 249, 4, 9, 50, 0, 3, 0, 33, 255, 11, 78, 69, 84, 83, 67,
            65, 80, 69, 50, 46, 48, 3, 1, 0, 0, 0, 44, 0, 0, 0, 0, 16, 0, 16, 0, 0, 2, 231,
            4, 0, 0, 0, 0, 0, 0, 0, 0, 132, 16, 66, 8, 33, 132, 16, 4, 65, 0, 0, 1, 0, 0, 0,
            0, 0, 0, 0, 0, 64, 0, 0, 1, 32, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 64, 0, 0, 16, 0,
            0, 0, 0, 0, 0, 0, 0, 2, 0, 64, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 2, 0, 64, 0, 0,
            16, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 64, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,
            0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 10, 0, 0, 33, 249, 4, 9, 50, 0, 3, 0, 44, 0, 0, 0, 0, 16, 0, 16,
            0, 0, 2, 231, 4, 0, 0, 0, 0, 0, 0, 0, 0, 132, 16, 66, 8, 33, 132, 16, 4, 65, 0,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 1, 32, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 64,
            0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 64, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 2,
            0, 64, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 64, 0, 32, 0, 0, 0, 2, 129, 0, 0,
            0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
            0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 16, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 59
    };

    static pthread_once_t s_favicon_buf_once = PTHREAD_ONCE_INIT;
    static mutil::IOBuf *s_favicon_buf = NULL;

    static void InitFavIcon() {
        s_favicon_buf = new mutil::IOBuf;
        s_favicon_buf->append((const void *) s_favicon_array,
                              arraysize(s_favicon_array));
    }

    static const mutil::IOBuf &GetFavIcon() {
        pthread_once(&s_favicon_buf_once, InitFavIcon);
        return *s_favicon_buf;
    }

    void GetFaviconService::default_method(
            ::google::protobuf::RpcController *controller,
            const GetFaviconRequest * /*request*/,
            GetFaviconResponse * /*response*/,
            ::google::protobuf::Closure *done) {
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("image/x-icon");
        cntl->response_attachment().clear();
        cntl->response_attachment().append(GetFavIcon());
        if (done) {
            done->Run();
        }
    }

} // namespace melon
