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

#pragma once

#include <pthread.h>
#include <melon/var/var.h>

namespace melon {

struct InfoThreadOptions {
    melon::var::LatencyRecorder* latency_recorder;
    melon::var::Adder<int64_t>* sent_count;
    melon::var::Adder<int64_t>* error_count;

    InfoThreadOptions()
        : latency_recorder(NULL)
        , sent_count(NULL)
        , error_count(NULL) {}
};

class InfoThread {
public:
    InfoThread();
    ~InfoThread();
    void run();
    bool start(const InfoThreadOptions&);
    void stop();
    
private:
    bool _stop;
    InfoThreadOptions _options;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    pthread_t _tid;
};

} // namespace melon
