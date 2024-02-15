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



#ifndef MELON_RPC_PERIODIC_NAMING_SERVICE_H_
#define MELON_RPC_PERIODIC_NAMING_SERVICE_H_

#include "melon/naming/naming_service.h"


namespace melon {

    class PeriodicNamingService : public NamingService {
    protected:
        virtual int GetServers(const char *service_name,
                               std::vector<ServerNode> *servers) = 0;

        virtual int GetNamingServiceAccessIntervalMs() const;

        int RunNamingService(const char *service_name,
                             NamingServiceActions *actions) override;
    };

} // namespace melon


#endif  // MELON_RPC_PERIODIC_NAMING_SERVICE_H_
