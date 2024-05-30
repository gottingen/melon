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



#ifndef MELON_RPC_HEALTH_REPORTER_H_
#define MELON_RPC_HEALTH_REPORTER_H_

#include <melon/rpc/controller.h>


namespace melon {

// For customizing /health page.
// Inherit this class and assign an instance to ServerOptions.health_reporter.
class HealthReporter {
public:
    virtual ~HealthReporter() {}
    
    // Get the http request from cntl->http_request() / cntl->request_attachment()
    // and put the response in cntl->http_response() / cntl->response_attachment()
    // Don't forget to call done->Run() at the end.
    virtual void GenerateReport(Controller* cntl, google::protobuf::Closure* done) = 0;
};

} // namespace melon


#endif  // MELON_RPC_HEALTH_REPORTER_H_
