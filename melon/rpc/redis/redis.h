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



#ifndef MELON_RPC_REDIS_REDIS_H_
#define MELON_RPC_REDIS_REDIS_H_

#include <google/protobuf/message.h>
#include <unordered_map>
#include <memory>
#include <list>
#include <melon/utility/iobuf.h>
#include <melon/utility/strings/string_piece.h>
#include <melon/utility/arena.h>
#include <melon/proto/rpc/proto_base.pb.h>
#include <melon/rpc/redis/redis_reply.h>
#include <melon/rpc/parse_result.h>
#include <melon/rpc/callback.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/pb_compat.h>

namespace melon {

    // Request to redis.
    // Notice that you can pipeline multiple commands in one request and sent
    // them to ONE redis-server together.
    // Example:
    //   RedisRequest request;
    //   request.AddCommand("PING");
    //   RedisResponse response;
    //   channel.CallMethod(&controller, &request, &response, NULL/*done*/);
    //   if (!cntl.Failed()) {
    //       MLOG(INFO) << response.reply(0);
    //   }
    class RedisRequest : public ::google::protobuf::Message {
    public:
        RedisRequest();

        virtual ~RedisRequest();

        RedisRequest(const RedisRequest &from);

        inline RedisRequest &operator=(const RedisRequest &from) {
            CopyFrom(from);
            return *this;
        }

        void Swap(RedisRequest *other);

        // Add a command with a va_list to this request. The conversion
        // specifiers are compatible with the ones used by hiredis, namely except
        // that %b stands for binary data, other specifiers are similar with printf.
        bool AddCommandV(const char *fmt, va_list args);

        // Concatenate components into a redis command, similarly with
        // redisCommandArgv() in hiredis.
        // Example:
        //   mutil::StringPiece components[] = { "set", "key", "value" };
        //   request.AddCommandByComponents(components, arraysize(components));
        bool AddCommandByComponents(const mutil::StringPiece *components, size_t n);

        // Add a command with variadic args to this request.
        // The reason that adding so many overloads rather than using ... is that
        // it's the only way to dispatch the AddCommand w/o args differently.
        bool AddCommand(const mutil::StringPiece &command);

        template<typename A1>
        bool AddCommand(const char *format, A1 a1) { return AddCommandWithArgs(format, a1); }

        template<typename A1, typename A2>
        bool AddCommand(const char *format, A1 a1, A2 a2) { return AddCommandWithArgs(format, a1, a2); }

        template<typename A1, typename A2, typename A3>
        bool AddCommand(const char *format, A1 a1, A2 a2, A3 a3) { return AddCommandWithArgs(format, a1, a2, a3); }

        template<typename A1, typename A2, typename A3, typename A4>
        bool AddCommand(const char *format, A1 a1, A2 a2, A3 a3, A4 a4) {
            return AddCommandWithArgs(format, a1, a2, a3, a4);
        }

        template<typename A1, typename A2, typename A3, typename A4, typename A5>
        bool AddCommand(const char *format, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
            return AddCommandWithArgs(format, a1, a2, a3, a4, a5);
        }

        template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
        bool AddCommand(const char *format, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
            return AddCommandWithArgs(format, a1, a2, a3, a4, a5, a6);
        }

        // Number of successfully added commands
        int command_size() const { return _ncommand; }

        // True if previous AddCommand[V] failed.
        bool has_error() const { return _has_error; }

        // Serialize the request into `buf'. Return true on success.
        bool SerializeTo(mutil::IOBuf *buf) const;

        // Protobuf methods.
        RedisRequest *New() const PB_319_OVERRIDE;

#if GOOGLE_PROTOBUF_VERSION >= 3006000

        RedisRequest *New(::google::protobuf::Arena *arena) const override;

#endif

        void CopyFrom(const ::google::protobuf::Message &from) PB_321_OVERRIDE;

        void MergeFrom(const ::google::protobuf::Message &from) override;

        void CopyFrom(const RedisRequest &from);

        void MergeFrom(const RedisRequest &from);

        void Clear() override;

        bool IsInitialized() const override;

        int ByteSize() const;

        bool MergePartialFromCodedStream(
                ::google::protobuf::io::CodedInputStream *input) PB_310_OVERRIDE;

        void SerializeWithCachedSizes(
                ::google::protobuf::io::CodedOutputStream *output) const PB_310_OVERRIDE;

        ::google::protobuf::uint8 *
        SerializeWithCachedSizesToArray(::google::protobuf::uint8 *output) const PB_310_OVERRIDE;

        int GetCachedSize() const override { return _cached_size_; }

        static const ::google::protobuf::Descriptor *descriptor();

        void Print(std::ostream &) const;

    protected:
        ::google::protobuf::Metadata GetMetadata() const override;

    private:
        void SharedCtor();

        void SharedDtor();

        void SetCachedSize(int size) const override;

        bool AddCommandWithArgs(const char *fmt, ...);

        int _ncommand;    // # of valid commands
        bool _has_error;  // previous AddCommand had error
        mutil::IOBuf _buf;  // the serialized request.
        mutable int _cached_size_;  // ByteSize
    };

    // Response from Redis.
    // Notice that a RedisResponse instance may contain multiple replies
    // due to pipelining.
    class RedisResponse : public ::google::protobuf::Message {
    public:
        RedisResponse();

        virtual ~RedisResponse();

        RedisResponse(const RedisResponse &from);

        inline RedisResponse &operator=(const RedisResponse &from) {
            CopyFrom(from);
            return *this;
        }

        void Swap(RedisResponse *other);

        // Number of replies in this response.
        // (May have more than one reply due to pipeline)
        int reply_size() const { return _nreply; }

        // Get index-th reply. If index is out-of-bound, nil reply is returned.
        const RedisReply &reply(int index) const {
            if (index < reply_size()) {
                return (index == 0 ? _first_reply : _other_replies[index - 1]);
            }
            static RedisReply redis_nil(NULL);
            return redis_nil;
        }

        // Parse and consume intact replies from the buf.
        // Returns PARSE_OK on success.
        // Returns PARSE_ERROR_NOT_ENOUGH_DATA if data in `buf' is not enough to parse.
        // Returns PARSE_ERROR_ABSOLUTELY_WRONG if the parsing failed.
        ParseError ConsumePartialIOBuf(mutil::IOBuf &buf, int reply_count);

        // implements Message ----------------------------------------------

        RedisResponse *New() const PB_319_OVERRIDE;

#if GOOGLE_PROTOBUF_VERSION >= 3006000

        RedisResponse *New(::google::protobuf::Arena *arena) const override;

#endif

        void CopyFrom(const ::google::protobuf::Message &from) PB_321_OVERRIDE;

        void MergeFrom(const ::google::protobuf::Message &from) override;

        void CopyFrom(const RedisResponse &from);

        void MergeFrom(const RedisResponse &from);

        void Clear() override;

        bool IsInitialized() const override;

        int ByteSize() const;

        bool MergePartialFromCodedStream(
                ::google::protobuf::io::CodedInputStream *input) PB_310_OVERRIDE;

        void SerializeWithCachedSizes(
                ::google::protobuf::io::CodedOutputStream *output) const PB_310_OVERRIDE;

        ::google::protobuf::uint8 *
        SerializeWithCachedSizesToArray(::google::protobuf::uint8 *output) const PB_310_OVERRIDE;

        int GetCachedSize() const override { return _cached_size_; }

        static const ::google::protobuf::Descriptor *descriptor();

    protected:
        ::google::protobuf::Metadata GetMetadata() const override;

    private:
        void SharedCtor();

        void SharedDtor();

        void SetCachedSize(int size) const override;

        RedisReply _first_reply;
        RedisReply *_other_replies;
        mutil::Arena _arena;
        int _nreply;
        mutable int _cached_size_;
    };

    std::ostream &operator<<(std::ostream &os, const RedisRequest &);

    std::ostream &operator<<(std::ostream &os, const RedisResponse &);

    class RedisCommandHandler;

    // Container of CommandHandlers.
    // Assign an instance to ServerOption.redis_service to enable redis support.
    class RedisService {
    public:
        virtual ~RedisService() {}

        // Call this function to register `handler` that can handle command `name`.
        bool AddCommandHandler(const std::string &name, RedisCommandHandler *handler);

        // This function should not be touched by user and used by melon deverloper only.
        RedisCommandHandler *FindCommandHandler(const mutil::StringPiece &name) const;

    private:
        typedef std::unordered_map<std::string, RedisCommandHandler *> CommandMap;
        CommandMap _command_map;
    };

    enum RedisCommandHandlerResult {
        REDIS_CMD_HANDLED = 0,
        REDIS_CMD_CONTINUE = 1,
        REDIS_CMD_BATCHED = 2,
    };

    // The Command handler for a redis request. User should impletement Run().
    class RedisCommandHandler {
    public:
        virtual ~RedisCommandHandler() {}

        // Once Server receives commands, it will first find the corresponding handlers and
        // call them sequentially(one by one) according to the order that requests arrive,
        // just like what redis-server does.
        // `args' is the array of request command. For example, "set somekey somevalue"
        // corresponds to args[0]=="set", args[1]=="somekey" and args[2]=="somevalue".
        // `output', which should be filled by user, is the content that sent to client side.
        // Read melon/src/redis_reply.h for more usage.
        // `flush_batched' indicates whether the user should flush all the results of
        // batched commands. If user want to do some batch processing, user should buffer
        // the commands and return REDIS_CMD_BATCHED. Once `flush_batched' is true,
        // run all the commands, set `output' to be an array in which every element is the
        // result of batched commands and return REDIS_CMD_HANDLED.
        //
        // The return value should be REDIS_CMD_HANDLED for normal cases. If you want
        // to implement transaction, return REDIS_CMD_CONTINUE once server receives
        // an start marker and melon will call MultiTransactionHandler() to new a transaction
        // handler that all the following commands are sent to this tranction handler until
        // it returns REDIS_CMD_HANDLED. Read the comment below.
        virtual RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece> &args,
                                              melon::RedisReply *output,
                                              bool flush_batched) = 0;

        // The Run() returns CONTINUE for "multi", which makes melon call this method to
        // create a transaction_handler to process following commands until transaction_handler
        // returns OK. For example, for command "multi; set k1 v1; set k2 v2; set k3 v3;
        // exec":
        // 1) First command is "multi" and Run() should return REDIS_CMD_CONTINUE,
        // then melon calls NewTransactionHandler() to new a transaction_handler.
        // 2) melon calls transaction_handler.Run() with command "set k1 v1",
        // which should return CONTINUE.
        // 3) melon calls transaction_handler.Run() with command "set k2 v2",
        // which should return CONTINUE.
        // 4) melon calls transaction_handler.Run() with command "set k3 v3",
        // which should return CONTINUE.
        // 5) An ending marker(exec) is found in transaction_handler.Run(), user exeuctes all
        // the commands and return OK. This Transation is done.
        virtual RedisCommandHandler *NewTransactionHandler();
    };

} // namespace melon

#endif  // MELON_RPC_REDIS_REDIS_H_
