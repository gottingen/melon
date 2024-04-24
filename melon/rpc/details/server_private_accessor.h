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


#pragma once

#include <google/protobuf/descriptor.h>
#include "melon/rpc/server.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/details/method_status.h"
#include "melon/builtin/bad_method_service.h"
#include "melon/rpc/restful.h"

namespace melon {

// A wrapper to access some private methods/fields of `Server'
// This is supposed to be used by internal RPC protocols ONLY
class ServerPrivateAccessor {
public:
    explicit ServerPrivateAccessor(const Server* svr) {
        MCHECK(svr);
        _server = svr;
    }

    void AddError() {
        _server->_nerror_bvar << 1;
    }

    // Returns true if the `max_concurrency' limit is not reached.
    bool AddConcurrency(Controller* c) {
        if (_server->options().max_concurrency <= 0) {
            return true;
        }
        c->add_flag(Controller::FLAGS_ADDED_CONCURRENCY);
        return (mutil::subtle::NoBarrier_AtomicIncrement(&_server->_concurrency, 1)
                <= _server->options().max_concurrency);
    }

    void RemoveConcurrency(const Controller* c) {
        if (c->has_flag(Controller::FLAGS_ADDED_CONCURRENCY)) {
            mutil::subtle::NoBarrier_AtomicIncrement(&_server->_concurrency, -1);
        }
    }

    // Find by MethodDescriptor::full_name
    const Server::MethodProperty*
    FindMethodPropertyByFullName(const mutil::StringPiece &fullname) {
        return _server->FindMethodPropertyByFullName(fullname);
    }
    const Server::MethodProperty*
    FindMethodPropertyByFullName(const mutil::StringPiece& fullname) const {
        return _server->FindMethodPropertyByFullName(fullname);
    }
    const Server::MethodProperty*
    FindMethodPropertyByFullName(const mutil::StringPiece& full_service_name,
                                 const mutil::StringPiece& method_name) const {
        return _server->FindMethodPropertyByFullName(
            full_service_name, method_name);
    }
    const Server::MethodProperty* FindMethodPropertyByNameAndIndex(
        const mutil::StringPiece& service_name, int method_index) const {
        return _server->FindMethodPropertyByNameAndIndex(service_name, method_index);
    }

    const Server::ServiceProperty*
    FindServicePropertyByFullName(const mutil::StringPiece& fullname) const {
        return _server->FindServicePropertyByFullName(fullname);
    }

    const Server::ServiceProperty*
    FindServicePropertyByName(const mutil::StringPiece& name) const {
        return _server->FindServicePropertyByName(name);
    }

    const Server::ServiceProperty*
    FindServicePropertyAdaptively(const mutil::StringPiece& service_name) const {
        if (service_name.find('.') == mutil::StringPiece::npos) {
            return _server->FindServicePropertyByName(service_name);
        } else {
            return _server->FindServicePropertyByFullName(service_name);
        }
    }

    Acceptor* acceptor() const { return _server->_am; }

    RestfulMap* global_restful_map() const
    { return _server->_global_restful_map; }

private:
    const Server* _server;
};

// Count one error if release() is not called before destruction of this object.
class ScopedNonServiceError {
public:
    ScopedNonServiceError(const Server* server) : _server(server) {}
    ~ScopedNonServiceError() {
        if (_server) {
            ServerPrivateAccessor(_server).AddError();
            _server = NULL;
        }
    }
    const Server* release() {
        const Server* tmp = _server;
        _server = NULL;
        return tmp;
    }
private:
    DISALLOW_COPY_AND_ASSIGN(ScopedNonServiceError);
    const Server* _server;
};

} // namespace melon

