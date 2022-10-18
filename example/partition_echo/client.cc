
#include <gflags/gflags.h>
#include <melon/fiber/this_fiber.h>
#include <melon/fiber/internal/fiber.h>
#include "melon/log/logging.h"
#include "melon/times/time.h"
#include <melon/rpc/partition_channel.h>
#include <deque>
#include "echo.pb.h"

DEFINE_int32(thread_num, 1, "Number of threads to send requests");
DEFINE_int32(partition_num, 3, "Number of partitions");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_int32(attachment_size, 0, "Carry so many byte attachment along with requests");
DEFINE_int32(request_size, 16, "Bytes of each request");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in melon/rpc/options.proto");
DEFINE_string(server, "file://server_list", "Mapping to servers");
DEFINE_string(load_balancer, "rr", "Name of load balancer");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");

std::string g_request;
std::string g_attachment;
pthread_mutex_t g_latency_mutex = PTHREAD_MUTEX_INITIALIZER;
struct MELON_CACHELINE_ALIGNMENT SenderInfo {
    size_t nsuccess;
    int64_t latency_sum;
};
std::deque<SenderInfo> g_sender_info;

static void* sender(void* arg) {
    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    example::EchoService_Stub stub(static_cast<google::protobuf::RpcChannel*>(arg));

    SenderInfo* info = NULL;
    {
        MELON_SCOPED_LOCK(g_latency_mutex);
        g_sender_info.push_back(SenderInfo());
        info = &g_sender_info.back();
    }

    int log_id = 0;
    while (!melon::rpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        example::EchoRequest request;
        example::EchoResponse response;
        melon::rpc::Controller cntl;

        request.set_message(g_request);
        cntl.set_log_id(log_id++);  // set by user
        if (!g_attachment.empty()) {
            // Set attachment which is wired to network directly instead of 
            // being serialized into protobuf messages.
            cntl.request_attachment().append(g_attachment);
        }

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        stub.Echo(&cntl, &request, &response, NULL);
        if (!cntl.Failed()) {
            info->latency_sum += cntl.latency_us();
            ++info->nsuccess;
        } else {
            MELON_CHECK(melon::rpc::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            melon::fiber_sleep_for(50000);
        }
    }
    return NULL;
}

class MyPartitionParser : public melon::rpc::PartitionParser {
public:
    bool ParseFromTag(const std::string& tag, melon::rpc::Partition* out) {
        // "N/M" : #N partition of M partitions.
        size_t pos = tag.find_first_of('/');
        if (pos == std::string::npos) {
            MELON_LOG(ERROR) << "Invalid tag=`" << tag << '\'';
            return false;
        }
        char* endptr = NULL;
        out->index = strtol(tag.c_str(), &endptr, 10);
        if (endptr != tag.data() + pos) {
            MELON_LOG(ERROR) << "Invalid index=" << std::string_view(tag.data(), pos);
            return false;
        }
        out->num_partition_kinds = strtol(tag.c_str() + pos + 1, &endptr, 10);
        if (endptr != tag.c_str() + tag.size()) {
            MELON_LOG(ERROR) << "Invalid num=" << tag.data() + pos + 1;
            return false;
        }
        return true;
    }
};


int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::rpc::PartitionChannel channel;

    melon::rpc::PartitionChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    options.succeed_without_server = true;
    options.fail_limit = 1;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;

    if (channel.Init(FLAGS_partition_num, new MyPartitionParser(),
                     FLAGS_server.c_str(),
                     FLAGS_load_balancer.c_str(),
                     &options) != 0) {
        MELON_LOG(ERROR) << "Fail to init channel";
        return -1;
    }
    if (FLAGS_attachment_size > 0) {
        g_attachment.resize(FLAGS_attachment_size, 'a');
    }
    if (FLAGS_request_size <= 0) {
        MELON_LOG(ERROR) << "Bad request_size=" << FLAGS_request_size;
        return -1;
    }
    g_request.resize(FLAGS_request_size, 'r');

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

    int64_t last_counter = 0;
    int64_t last_latency_sum = 0;
    std::vector<size_t> last_nsuccess(FLAGS_thread_num);
    while (!melon::rpc::IsAskedToQuit()) {
        sleep(1);
        int64_t latency_sum = 0;
        int64_t nsuccess = 0;
        pthread_mutex_lock(&g_latency_mutex);
        MELON_CHECK_EQ(g_sender_info.size(), (size_t)FLAGS_thread_num);
        for (size_t i = 0; i < g_sender_info.size(); ++i) {
            const SenderInfo& info = g_sender_info[i];
            latency_sum += info.latency_sum;
            nsuccess += info.nsuccess;
            if (FLAGS_dont_fail) {
                MELON_CHECK(info.nsuccess > last_nsuccess[i]);
            }
            last_nsuccess[i] = info.nsuccess;
        }
        pthread_mutex_unlock(&g_latency_mutex);

        const int64_t avg_latency = (latency_sum - last_latency_sum) /
            std::max(nsuccess - last_counter, (int64_t)1);
        MELON_LOG(INFO) << "Sending EchoRequest at qps=" << nsuccess - last_counter
                  << " latency=" << avg_latency;
        last_counter = nsuccess;
        last_latency_sum = latency_sum;
    }

    MELON_LOG(INFO) << "EchoClient is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(bids[i], NULL);
        }
    }

    return 0;
}
