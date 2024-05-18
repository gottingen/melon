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


// A melon based command-line interface to talk with redis-server

#include <signal.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <gflags/gflags.h>
#include <melon/utility/logging.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/redis/redis.h>

DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "127.0.0.1:6379", "IP Address of server");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");

namespace melon {
    const char *logo();
}

// Send `command' to redis-server via `channel'
static bool access_redis(melon::Channel &channel, const char *command) {
    melon::RedisRequest request;
    if (!request.AddCommand(command)) {
        MLOG(ERROR) << "Fail to add command";
        return false;
    }
    melon::RedisResponse response;
    melon::Controller cntl;
    channel.CallMethod(nullptr, &cntl, &request, &response, nullptr);
    if (cntl.Failed()) {
        MLOG(ERROR) << "Fail to access redis, " << cntl.ErrorText();
        return false;
    } else {
        std::cout << response << std::endl;
        return true;
    }
}

// For freeing the memory returned by readline().
struct Freer {
    void operator()(char *mem) {
        free(mem);
    }
};

static void dummy_handler(int) {}

// The getc for readline. The default getc retries reading when meeting
// EINTR, which is not what we want.
static bool g_canceled = false;

static int cli_getc(FILE *stream) {
    int c = getc(stream);
    if (c == EOF && errno == EINTR) {
        g_canceled = true;
        return '\n';
    }
    return c;
}

int main(int argc, char *argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::Channel channel;

    // Initialize the channel, nullptr means using default options.
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(), &options) != 0) {
        MLOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    if (argc <= 1) {  // interactive mode
        // We need this dummy signal hander to interrupt getc (and returning
        // EINTR), SIG_IGN did not work.
        signal(SIGINT, dummy_handler);

        // Hook getc of readline.
        rl_getc_function = cli_getc;

        // Print welcome information.
        printf("%s\n", melon::logo());
        printf("This command-line tool mimics the look-n-feel of official "
               "redis-cli, as a demostration of melon's capability of"
               " talking to redis-server. The output and behavior is "
               "not exactly same with the official one.\n\n");

        for (;;) {
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "redis %s> ", FLAGS_server.c_str());
            std::unique_ptr<char, Freer> command(readline(prompt));
            if (command == nullptr || *command == '\0') {
                if (g_canceled) {
                    // No input after the prompt and user pressed Ctrl-C,
                    // quit the CLI.
                    return 0;
                }
                // User entered an empty command by just pressing Enter.
                continue;
            }
            if (g_canceled) {
                // User entered sth. and pressed Ctrl-C, start a new prompt.
                g_canceled = false;
                continue;
            }
            // Add user's command to history so that it's browse-able by
            // UP-key and search-able by Ctrl-R.
            add_history(command.get());

            if (!strcmp(command.get(), "help")) {
                printf("This is a redis CLI written in melon.\n");
                continue;
            }
            if (!strcmp(command.get(), "quit")) {
                // Although quit is a valid redis command, it does not make
                // too much sense to run it in this CLI, just quit.
                return 0;
            }
            access_redis(channel, command.get());
        }
    } else {
        std::string command;
        command.reserve(argc * 16);
        for (int i = 1; i < argc; ++i) {
            if (i != 1) {
                command.push_back(' ');
            }
            command.append(argv[i]);
        }
        if (!access_redis(channel, command.c_str())) {
            return -1;
        }
    }
    return 0;
}