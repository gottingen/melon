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



#ifndef MELON_BUILTIN_VIZ_MIN_JS_H_
#define MELON_BUILTIN_VIZ_MIN_JS_H_

#include "melon/utility/iobuf.h"


namespace melon {

    // Get the viz.min.js as string or IOBuf.
    // We need to pack all js inside C++ code so that builtin services can be
    // accessed without external resources and network connection.
    const char *viz_min_js();

    const mutil::IOBuf &viz_min_js_iobuf();

    const mutil::IOBuf &viz_min_js_iobuf_gzip();

} // namespace melon


#endif // MELON_BUILTIN_VIZ_MIN_JS_H_