
#include <gflags/gflags.h>
#include <melon/fiber/this_fiber.h>
#include <melon/fiber/internal/fiber.h>
#include "melon/log/logging.h"
#include "melon/metrics/counter.h"
//#include <melon/strings/strings.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/memcache.h>
#include <melon/rpc/policy/couchbase_authenticator.h>

DEFINE_int32(thread_num, 10, "Number of threads to send requests");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_bool(use_couchbase, false, "Use couchbase.");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:11211", "IP Address of server");
DEFINE_string(bucket_name, "", "Couchbase bucktet name");
DEFINE_string(bucket_password, "", "Couchbase bucket password");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(exptime, 0, "The to-be-got data will be expired after so many seconds");
DEFINE_string(key, "hello", "The key to be get");
DEFINE_string(value, "world", "The value associated with the key");
DEFINE_int32(batch, 1, "Pipelined Operations");

melon::LatencyRecorder g_latency_recorder("client");
melon::counter<int> g_error_count("client_error_count");
melon::static_atomic<int> g_sender_count = MELON_STATIC_ATOMIC_INIT(0);

static void* sender(void* arg) {
    google::protobuf::RpcChannel* channel = 
        static_cast<google::protobuf::RpcChannel*>(arg);
    const int base_index = g_sender_count.fetch_add(1, std::memory_order_relaxed);

    std::string value;
    std::vector<std::pair<std::string, std::string> > kvs;
    kvs.resize(FLAGS_batch);
    for (int i = 0; i < FLAGS_batch; ++i) {
        kvs[i].first = melon::string_printf("%s%d", FLAGS_key.c_str(), base_index + i);
        kvs[i].second = melon::string_printf("%s%d", FLAGS_value.c_str(), base_index + i);
    }
    melon::rpc::MemcacheRequest request;
    for (int i = 0; i < FLAGS_batch; ++i) {
        MELON_CHECK(request.Get(kvs[i].first));
    }
    while (!melon::rpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        melon::rpc::MemcacheResponse response;
        melon::rpc::Controller cntl;

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        channel->CallMethod(NULL, &cntl, &request, &response, NULL);
        const int64_t elp = cntl.latency_us();
        if (!cntl.Failed()) {
            g_latency_recorder << cntl.latency_us();
            for (int i = 0; i < FLAGS_batch; ++i) {
                uint32_t flags;
                if (!response.PopGet(&value, &flags, NULL)) {
                    MELON_LOG(INFO) << "Fail to GET the key, " << response.LastError();
                    melon::rpc::AskToQuit();
                    return NULL;
                }
                MELON_CHECK(flags == 0xdeadbeef + base_index + i)
                    << "flags=" << flags;
                MELON_CHECK(kvs[i].second == value)
                    << "base=" << base_index << " i=" << i << " value=" << value;
            }
        } else {
            g_error_count << 1; 
            MELON_CHECK(melon::rpc::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << cntl.ErrorText() << " latency=" << elp;
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            melon::fiber_sleep_for(50000);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_exptime < 0) {
        FLAGS_exptime = 0;
    }

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::rpc::Channel channel;
    
    // Initialize the channel, NULL means using default options. 
    melon::rpc::ChannelOptions options;
    options.protocol = melon::rpc::PROTOCOL_MEMCACHE;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (FLAGS_use_couchbase && !FLAGS_bucket_name.empty()) {
        melon::rpc::policy::CouchbaseAuthenticator* auth =
            new melon::rpc::policy::CouchbaseAuthenticator(FLAGS_bucket_name,
                                                     FLAGS_bucket_password);
        options.auth = auth;
    }

    if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
        MELON_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Pipeline #batch * #thread_num SET requests into memcache so that we
    // have keys to get.
    melon::rpc::MemcacheRequest request;
    melon::rpc::MemcacheResponse response;
    melon::rpc::Controller cntl;
    for (int i = 0; i < FLAGS_batch * FLAGS_thread_num; ++i) {
        if (!request.Set(melon::string_printf("%s%d", FLAGS_key.c_str(), i),
                         melon::string_printf("%s%d", FLAGS_value.c_str(), i),
                         0xdeadbeef + i, FLAGS_exptime, 0)) {
            MELON_LOG(ERROR) << "Fail to SET " << i << "th request";
            return -1;
        }
    }
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        MELON_LOG(ERROR) << "Fail to access memcache, " << cntl.ErrorText();
        return -1;
    }
    for (int i = 0; i < FLAGS_batch * FLAGS_thread_num; ++i) {
        if (!response.PopSet(NULL)) {
            MELON_LOG(ERROR) << "Fail to SET memcache, i=" << i
                       << ", " << response.LastError();
            return -1;
        }
    }
    if (FLAGS_exptime > 0) {
        MELON_LOG(INFO) << "Set " << FLAGS_batch * FLAGS_thread_num
                  << " values, expired after " << FLAGS_exptime << " seconds";
    } else {
        MELON_LOG(INFO) << "Set " << FLAGS_batch * FLAGS_thread_num
                  << " values, never expired";
    }
    
    std::vector<fiber_id_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_fiber) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], NULL, sender, &channel) != 0) {
                MELON_LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (fiber_start_background(
                    &bids[i], NULL, sender, &channel) != 0) {
                MELON_LOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }

    while (!melon::rpc::IsAskedToQuit()) {
        sleep(1);
        MELON_LOG(INFO) << "Accessing memcache server at qps=" << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    MELON_LOG(INFO) << "memcache_client is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(bids[i], NULL);
        }
    }
    if (options.auth) {
        delete options.auth;
    }

    return 0;
}
