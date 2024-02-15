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



#ifndef MELON_NAMING_NAMING_SERVICE_FILTER_H_
#define MELON_NAMING_NAMING_SERVICE_FILTER_H_

#include "melon/naming/naming_service.h"      // ServerNode


namespace melon {

    class NamingServiceFilter {
    public:
        virtual ~NamingServiceFilter() {}

        // Return true to take this `server' as a candidate to issue RPC
        // Return false to filter it out
        virtual bool Accept(const ServerNode &server) const = 0;
    };

} // namespace melon



#endif // MELON_NAMING_NAMING_SERVICE_FILTER_H_
