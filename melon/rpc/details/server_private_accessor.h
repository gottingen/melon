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

#include <google/protobuf/descriptor.h>
#include <melon/rpc/server.h>
#include <melon/rpc/acceptor.h>
#include <melon/rpc/details/method_status.h>
#include <melon/builtin/bad_method_service.h>
#include <melon/rpc/restful.h>

namespace melon {

// A wrapper to access some private methods/fields of `Server'
// This is supposed to be used by internal RPC protocols ONLY
class ServerPrivateAccessor {
public:
    explicit ServerPrivateAccessor(const Server* svr) {
        CHECK(svr);
        _server = svr;
    }

    void AddError() {
        _server->_nerror_var << 1;
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
    FindMethodPropertyByFullName(const std::string_view &fullname) {
        return _server->FindMethodPropertyByFullName(fullname);
    }
    const Server::MethodProperty*
    FindMethodPropertyByFullName(const std::string_view& fullname) const {
        return _server->FindMethodPropertyByFullName(fullname);
    }
    const Server::MethodProperty*
    FindMethodPropertyByFullName(const std::string_view& full_service_name,
                                 const std::string_view& method_name) const {
        return _server->FindMethodPropertyByFullName(
            full_service_name, method_name);
    }
    const Server::MethodProperty* FindMethodPropertyByNameAndIndex(
        const std::string_view& service_name, int method_index) const {
        return _server->FindMethodPropertyByNameAndIndex(service_name, method_index);
    }

    const Server::ServiceProperty*
    FindServicePropertyByFullName(const std::string_view& fullname) const {
        return _server->FindServicePropertyByFullName(fullname);
    }

    const Server::ServiceProperty*
    FindServicePropertyByName(const std::string_view& name) const {
        return _server->FindServicePropertyByName(name);
    }

    const Server::ServiceProperty*
    FindServicePropertyAdaptively(const std::string_view& service_name) const {
        if (service_name.find('.') == std::string_view::npos) {
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

