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

//
// A melon based redis-server. Currently just implement set and
// get, but it's sufficient that you can get the idea how to
// implement melon::RedisCommandHandler.

#include <melon/rpc/server.h>
#include <melon/rpc/redis/redis.h>
#include <melon/utility/crc32c.h>
#include <melon/utility/strings/string_split.h>
#include <gflags/gflags.h>
#include <memory>
#include <unordered_map>

#include <melon/utility/time.h>

DEFINE_int32(port, 6379, "TCP Port of this server");

class RedisServiceImpl : public melon::RedisService {
public:
    bool Set(const std::string& key, const std::string& value) {
        int slot = mutil::crc32c::Value(key.c_str(), key.size()) % kHashSlotNum;
        _mutex[slot].lock();
        _map[slot][key] = value;
        _mutex[slot].unlock();
        return true;
    }

    bool Get(const std::string& key, std::string* value) {
        int slot = mutil::crc32c::Value(key.c_str(), key.size()) % kHashSlotNum;
        _mutex[slot].lock();
        auto it = _map[slot].find(key);
        if (it == _map[slot].end()) {
            _mutex[slot].unlock();
            return false;
        }
        *value = it->second;
        _mutex[slot].unlock();
        return true;
    }

private:
    const static int kHashSlotNum = 32;
    std::unordered_map<std::string, std::string> _map[kHashSlotNum];
    mutil::Mutex _mutex[kHashSlotNum];
};

class GetCommandHandler : public melon::RedisCommandHandler {
public:
    explicit GetCommandHandler(RedisServiceImpl* rsimpl)
        : _rsimpl(rsimpl) {}

    melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                        melon::RedisReply* output,
                                        bool /*flush_batched*/) override {
        if (args.size() != 2ul) {
            output->FormatError("Expect 1 arg for 'get', actually %lu", args.size()-1);
            return melon::REDIS_CMD_HANDLED;
        }
        const std::string key(args[1].data(), args[1].size());
        std::string value;
        if (_rsimpl->Get(key, &value)) {
            output->SetString(value);
        } else {
            output->SetNullString();
        }
        return melon::REDIS_CMD_HANDLED;
	}

private:
   	RedisServiceImpl* _rsimpl;
};

class SetCommandHandler : public melon::RedisCommandHandler {
public:
    explicit SetCommandHandler(RedisServiceImpl* rsimpl)
        : _rsimpl(rsimpl) {}

    melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                        melon::RedisReply* output,
                                        bool /*flush_batched*/) override {
        if (args.size() != 3ul) {
            output->FormatError("Expect 2 args for 'set', actually %lu", args.size()-1);
            return melon::REDIS_CMD_HANDLED;
        }
        const std::string key(args[1].data(), args[1].size());
        const std::string value(args[2].data(), args[2].size());
        _rsimpl->Set(key, value);
        output->SetStatus("OK");
        return melon::REDIS_CMD_HANDLED;
	}

private:
   	RedisServiceImpl* _rsimpl;
};

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    RedisServiceImpl *rsimpl = new RedisServiceImpl;
    auto get_handler =std::unique_ptr<GetCommandHandler>(new GetCommandHandler(rsimpl));
    auto set_handler =std::unique_ptr<SetCommandHandler>( new SetCommandHandler(rsimpl));
    rsimpl->AddCommandHandler("get", get_handler.get());
    rsimpl->AddCommandHandler("set", set_handler.get());

    melon::Server server;
    melon::ServerOptions server_options;
    server_options.redis_service = rsimpl;
    if (server.Start(FLAGS_port, &server_options) != 0) {
        MLOG(ERROR) << "Fail to start server";
        return -1;
    }
    server.RunUntilAskedToQuit();
    return 0;
}
