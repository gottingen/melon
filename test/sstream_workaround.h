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


#ifndef MUTIL_TEST_SSTREAM_WORKAROUND
#define MUTIL_TEST_SSTREAM_WORKAROUND

// defining private as public makes it fail to compile sstream with gcc5.x like this:
// "error: ‘struct std::__cxx11::basic_stringbuf<_CharT, _Traits, _Alloc>::
// __xfer_bufptrs’ redeclared with different access"

#ifdef private
# undef private
# include <sstream>
# define private public
#else
# include <sstream>
#endif

#endif  //  MUTIL_TEST_SSTREAM_WORKAROUND
