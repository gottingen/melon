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



#include <pthread.h>
#include "melon/utility/logging.h"
#include "melon/compress/gzip_compress.h"
#include "melon/builtin/viz_min_js.h"


namespace melon {

static pthread_once_t s_viz_min_buf_once = PTHREAD_ONCE_INIT; 
static mutil::IOBuf* s_viz_min_buf = NULL;
static void InitVizMinBuf() {
    s_viz_min_buf = new mutil::IOBuf;
    s_viz_min_buf->append(viz_min_js());
}
const mutil::IOBuf& viz_min_js_iobuf() {
    pthread_once(&s_viz_min_buf_once, InitVizMinBuf);
    return *s_viz_min_buf;
}

// viz.js is huge. We separate the creation of gzip version from uncompress
// version so that at most time we only keep gzip version in memory.
static pthread_once_t s_viz_min_buf_gzip_once = PTHREAD_ONCE_INIT; 
static mutil::IOBuf* s_viz_min_buf_gzip = NULL;
static void InitVizMinBufGzip() {
    mutil::IOBuf viz_min;
    viz_min.append(viz_min_js());
    s_viz_min_buf_gzip = new mutil::IOBuf;
    MCHECK(compress::GzipCompress(viz_min, s_viz_min_buf_gzip, NULL));
}
const mutil::IOBuf& viz_min_js_iobuf_gzip() {
    pthread_once(&s_viz_min_buf_gzip_once, InitVizMinBufGzip);
    return *s_viz_min_buf_gzip;
}

const char* viz_min_js() {
return "function Ub(nr){throw nr}var cc=void 0,wc=!0,xc=null,ee=!1;function bk(){return(function(){})}"



}

} // namespace melon