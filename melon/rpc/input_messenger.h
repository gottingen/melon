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



#ifndef MELON_RPC_INPUT_MESSENGER_H_
#define MELON_RPC_INPUT_MESSENGER_H_

#include <melon/base/iobuf.h>                    // mutil::IOBuf
#include <melon/rpc/socket.h>              // SocketId, SocketUser
#include <melon/rpc/parse_result.h>        // ParseResult
#include <melon/rpc/input_message_base.h>  // InputMessageBase


namespace melon {
namespace rdma {
class RdmaEndpoint;
}

struct InputMessageHandler {
    // The callback to cut a message from `source'.
    // Returned message will be passed to process_request or process_response
    // later and Destroy()-ed by them.
    // Returns:
    //   MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA):
    //     `source' does not form a complete message yet.
    //   MakeParseError(PARSE_ERROR_TRY_OTHERS).
    //     `source' does not fit the protocol, the data should be tried by
    //     other protocols. If the data is definitely corrupted (e.g. magic 
    //     header matches but other fields are wrong), pop corrupted part
    //     from `source' before returning.
    //  MakeMessage(InputMessageBase*):
    //     The message is parsed successfully and cut from `source'.
    typedef ParseResult (*Parse)(mutil::IOBuf* source, Socket *socket,
                                 bool read_eof, const void *arg);
    Parse parse;
    
    // The callback to handle `msg' created by a successful parse().
    // `msg' must be Destroy()-ed when the processing is done. To make sure
    // Destroy() is always called, consider using DestroyingPtr<> defined in
    // destroyable.h
    // May be called in a different thread from parse().
    typedef void (*Process)(InputMessageBase* msg);
    Process process;

    // The callback to verify authentication of this socket. Only called
    // on the first message that a socket receives. Can be NULL when 
    // authentication is not needed or this is the client side.
    // Returns true on successful authentication.
    typedef bool (*Verify)(const InputMessageBase* msg);
    Verify verify;

    // An argument associated with the handler.
    const void* arg;

    // Name of this handler, must be string constant.
    const char* name;
};

// Process messages from connections.
// `Message' corresponds to a client's request or a server's response.
class InputMessenger : public SocketUser {
friend class rdma::RdmaEndpoint;
public:
    explicit InputMessenger(size_t capacity = 128);
    ~InputMessenger();

    // [thread-safe] Must be called at least once before Start().
    // `handler' contains user-supplied callbacks to cut off and
    // process messages from connections.
    // Returns 0 on success, -1 otherwise.
    int AddHandler(const InputMessageHandler& handler);

    // [thread-safe] Create a socket to process input messages.
    int Create(const mutil::EndPoint& remote_side,
               time_t health_check_interval_s,
               SocketId* id);
    // Overwrite necessary fields in `base_options' and create a socket with
    // the modified options.
    int Create(SocketOptions base_options, SocketId* id);

    // Returns the internal index of `InputMessageHandler' whose name=`name'
    // Returns -1 when not found
    int FindProtocolIndex(const char* name) const;
    int FindProtocolIndex(ProtocolType type) const;
    
    // Get name of the n-th handler
    const char* NameOfProtocol(int n) const;

    // Add a handler which doesn't belong to any registered protocol.
    // Note: Invoking this method indicates that you are using Socket without
    // Channel nor Server. 
    int AddNonProtocolHandler(const InputMessageHandler& handler);

protected:
    // Load data from m->fd() into m->read_buf, cut off new messages and
    // call callbacks.
    static void OnNewMessages(Socket* m);
    
private:
    class InputMessageClosure {
    public:
        InputMessageClosure() : _msg(NULL) { }
        ~InputMessageClosure() noexcept(false);

        InputMessageBase* release() {
            InputMessageBase* m = _msg;
            _msg = NULL;
            return m;
        }

        void reset(InputMessageBase* m);

    private:
        InputMessageBase* _msg;
    };

    // Find a valid scissor from `handlers' to cut off `header' and `payload'
    // from m->read_buf, save index of the scissor into `index'.
    ParseResult CutInputMessage(Socket* m, size_t* index, bool read_eof);

    // Process a new message just received in OnNewMessages
    // Return value >= 0 means success
    int ProcessNewMessage(
            Socket* m, ssize_t bytes, bool read_eof,
            const uint64_t received_us, const uint64_t base_realtime,
            InputMessageClosure& last_msg);

    // User-supplied scissors and handlers.
    // the index of handler is exactly the same as the protocol
    InputMessageHandler* _handlers;
    // Max added protocol type
    mutil::atomic<int> _max_index;
    bool _non_protocol;
    size_t _capacity;

    mutil::Mutex _add_handler_mutex;
};

// Get the global InputMessenger at client-side.
MUTIL_FORCE_INLINE InputMessenger* get_client_side_messenger() {
    extern InputMessenger* g_messenger;
    return g_messenger;
}

InputMessenger* get_or_new_client_side_messenger();

} // namespace melon


#endif  // MELON_RPC_INPUT_MESSENGER_H_
