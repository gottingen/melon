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


#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <gflags/gflags.h>
#include <fcntl.h>                               // O_RDONLY
#include <signal.h>

#include "melon/utility/build_config.h"                  // OS_LINUX
// Naming services
#include "melon/naming/file_naming_service.h"
#include "melon/naming/list_naming_service.h"
#include "melon/naming/domain_naming_service.h"
#include "melon/naming/remote_file_naming_service.h"
#include "melon/naming/consul_naming_service.h"
#include "melon/naming/discovery_naming_service.h"
#include "melon/naming/nacos_naming_service.h"

// Load Balancers
#include "melon/lb/round_robin_load_balancer.h"
#include "melon/lb/weighted_round_robin_load_balancer.h"
#include "melon/lb/randomized_load_balancer.h"
#include "melon/lb/weighted_randomized_load_balancer.h"
#include "melon/lb/locality_aware_load_balancer.h"
#include "melon/lb/consistent_hashing_load_balancer.h"
#include "melon/rpc/policy/hasher.h"
#include "melon/rpc/policy/dynpart_load_balancer.h"

// Compress handlers
#include "melon/rpc/compress.h"
#include "melon/compress/gzip_compress.h"
#include "melon/compress/snappy_compress.h"

// Protocols
#include "melon/rpc/protocol.h"
#include "melon/rpc/policy/melon_rpc_protocol.h"
#include "melon/rpc/policy/baidu_rpc_protocol.h"
#include "melon/rpc/policy/http_rpc_protocol.h"
#include "melon/rpc/policy/http2_rpc_protocol.h"
#include "melon/rpc/policy/hulu_pbrpc_protocol.h"
#include "melon/rpc/policy/memcache_binary_protocol.h"
#include "melon/rpc/policy/streaming_rpc_protocol.h"
#include "melon/rpc/policy/mongo_protocol.h"
#include "melon/rpc/policy/redis_protocol.h"
#include "melon/rpc/policy/rtmp_protocol.h"

// Concurrency Limiters
#include "melon/rpc/concurrency_limiter.h"
#include "melon/rpc/policy/auto_concurrency_limiter.h"
#include "melon/rpc/policy/constant_concurrency_limiter.h"
#include "melon/rpc/policy/timeout_concurrency_limiter.h"

#include "melon/rpc/input_messenger.h"     // get_or_new_client_side_messenger
#include "melon/rpc/socket_map.h"          // SocketMapList
#include "melon/rpc/server.h"
#include "melon/rpc/trackme.h"             // TrackMe
#include "melon/rpc/details/usercode_backup_pool.h"

#if defined(OS_LINUX)

#include <malloc.h>                   // malloc_trim

#endif

#include "melon/utility/fd_guard.h"
#include "melon/utility/files/file_watcher.h"

extern "C" {
// defined in gperftools/malloc_extension_c.h
void MELON_WEAK MallocExtension_ReleaseFreeMemory(void);
}

namespace melon {

    DECLARE_bool(usercode_in_pthread);

    DEFINE_int32(free_memory_to_system_interval, 0,
                 "Try to return free memory to system every so many seconds, "
                 "values <= 0 disables this feature");
    MELON_VALIDATE_GFLAG(free_memory_to_system_interval, PassValidate);

    namespace policy {
// Defined in http_rpc_protocol.cpp
        void InitCommonStrings();
    }

    using namespace policy;

    const char *const DUMMY_SERVER_PORT_FILE = "dummy_server.port";

    struct GlobalExtensions {
        GlobalExtensions()
                : dns(80), dns_with_ssl(443), ch_mh_lb(melon::lb::CONS_HASH_LB_MURMUR3),
                  ch_md5_lb(melon::lb::CONS_HASH_LB_MD5), ch_ketama_lb(melon::lb::CONS_HASH_LB_KETAMA), constant_cl(0) {
        }

        melon::naming::FileNamingService fns;
        melon::naming::ListNamingService lns;
        melon::naming::DomainListNamingService dlns;
        melon::naming::DomainNamingService dns;
        melon::naming::DomainNamingService dns_with_ssl;
        melon::naming::RemoteFileNamingService rfns;
        melon::naming::ConsulNamingService cns;
        melon::naming::DiscoveryNamingService dcns;
        melon::naming::NacosNamingService nns;

        melon::lb::RoundRobinLoadBalancer rr_lb;
        melon::lb::WeightedRoundRobinLoadBalancer wrr_lb;
        melon::lb::RandomizedLoadBalancer randomized_lb;
        melon::lb::WeightedRandomizedLoadBalancer wr_lb;
        melon::lb::LocalityAwareLoadBalancer la_lb;
        melon::lb::ConsistentHashingLoadBalancer ch_mh_lb;
        melon::lb::ConsistentHashingLoadBalancer ch_md5_lb;
        melon::lb::ConsistentHashingLoadBalancer ch_ketama_lb;
        DynPartLoadBalancer dynpart_lb;

        AutoConcurrencyLimiter auto_cl;
        ConstantConcurrencyLimiter constant_cl;
        TimeoutConcurrencyLimiter timeout_cl;
    };

    static pthread_once_t register_extensions_once = PTHREAD_ONCE_INIT;
    static GlobalExtensions *g_ext = nullptr;

    static long ReadPortOfDummyServer(const char *filename) {
        mutil::fd_guard fd(open(filename, O_RDONLY));
        if (fd < 0) {
            LOG(ERROR) << "Fail to open `" << DUMMY_SERVER_PORT_FILE << "'";
            return -1;
        }
        char port_str[32];
        const ssize_t nr = read(fd, port_str, sizeof(port_str));
        if (nr <= 0) {
            LOG(ERROR) << "Fail to read `" << DUMMY_SERVER_PORT_FILE << "': "
                       << (nr == 0 ? "nothing to read" : berror());
            return -1;
        }
        port_str[std::min((size_t) nr, sizeof(port_str) - 1)] = '\0';
        const char *p = port_str;
        for (; isspace(*p); ++p) {}
        char *endptr = nullptr;
        const long port = strtol(p, &endptr, 10);
        for (; isspace(*endptr); ++endptr) {}
        if (*endptr != '\0') {
            LOG(ERROR) << "Invalid port=`" << port_str << "'";
            return -1;
        }
        return port;
    }

// Expose counters of mutil::IOBuf
    static int64_t GetIOBufBlockCount(void *) {
        return mutil::IOBuf::block_count();
    }

    static int64_t GetIOBufBlockCountHitTLSThreshold(void *) {
        return mutil::IOBuf::block_count_hit_tls_threshold();
    }

    static int64_t GetIOBufNewBigViewCount(void *) {
        return mutil::IOBuf::new_bigview_count();
    }

    static int64_t GetIOBufBlockMemory(void *) {
        return mutil::IOBuf::block_memory();
    }

// Defined in server.cpp
    extern mutil::static_atomic<int> g_running_server_count;

    static int GetRunningServerCount(void *) {
        return g_running_server_count.load(mutil::memory_order_relaxed);
    }

// Update global stuff periodically.
    static void *GlobalUpdate(void *) {
        // Expose variables.
        melon::var::PassiveStatus<int64_t> var_iobuf_block_count(
                "iobuf_block_count", GetIOBufBlockCount, nullptr);
        melon::var::PassiveStatus<int64_t> var_iobuf_block_count_hit_tls_threshold(
                "iobuf_block_count_hit_tls_threshold",
                GetIOBufBlockCountHitTLSThreshold, nullptr);
        melon::var::PassiveStatus<int64_t> var_iobuf_new_bigview_count(
                GetIOBufNewBigViewCount, nullptr);
        melon::var::PerSecond<melon::var::PassiveStatus<int64_t> > var_iobuf_new_bigview_second(
                "iobuf_newbigview_second", &var_iobuf_new_bigview_count);
        melon::var::PassiveStatus<int64_t> var_iobuf_block_memory(
                "iobuf_block_memory", GetIOBufBlockMemory, nullptr);
        melon::var::PassiveStatus<int> var_running_server_count(
                "rpc_server_count", GetRunningServerCount, nullptr);

        mutil::FileWatcher fw;
        if (fw.init_from_not_exist(DUMMY_SERVER_PORT_FILE) < 0) {
            LOG(FATAL) << "Fail to init FileWatcher on `" << DUMMY_SERVER_PORT_FILE << "'";
            return nullptr;
        }

        std::vector<SocketId> conns;
        const int64_t start_time_us = mutil::gettimeofday_us();
        const int WARN_NOSLEEP_THRESHOLD = 2;
        int64_t last_time_us = start_time_us;
        int consecutive_nosleep = 0;
        int64_t last_return_free_memory_time = start_time_us;
        while (1) {
            const int64_t sleep_us = 1000000L + last_time_us - mutil::gettimeofday_us();
            if (sleep_us > 0) {
                if (fiber_usleep(sleep_us) < 0) {
                    PLOG_IF(FATAL, errno != ESTOP) << "Fail to sleep";
                    break;
                }
                consecutive_nosleep = 0;
            } else {
                if (++consecutive_nosleep >= WARN_NOSLEEP_THRESHOLD) {
                    consecutive_nosleep = 0;
                    LOG(WARNING) << __FUNCTION__ << " is too busy!";
                }
            }
            last_time_us = mutil::gettimeofday_us();

            TrackMe();

            if (!IsDummyServerRunning()
                && g_running_server_count.load(mutil::memory_order_relaxed) == 0
                && fw.check_and_consume() > 0) {
                long port = ReadPortOfDummyServer(DUMMY_SERVER_PORT_FILE);
                if (port >= 0) {
                    StartDummyServerAt(port);
                }
            }

            SocketMapList(&conns);
            const int64_t now_ms = mutil::cpuwide_time_ms();
            for (size_t i = 0; i < conns.size(); ++i) {
                SocketUniquePtr ptr;
                if (Socket::Address(conns[i], &ptr) == 0) {
                    ptr->UpdateStatsEverySecond(now_ms);
                }
            }

            const int return_mem_interval =
                    FLAGS_free_memory_to_system_interval/*reloadable*/;
            if (return_mem_interval > 0 &&
                last_time_us >= last_return_free_memory_time +
                                return_mem_interval * 1000000L) {
                last_return_free_memory_time = last_time_us;
                // TODO: Calling MallocExtension::instance()->ReleaseFreeMemory may
                // crash the program in later calls to malloc, verified on tcmalloc
                // 1.7 and 2.5, which means making the static member function weak
                // in details/tcmalloc_extension.cpp is probably not correct, however
                // it does work for heap profilers.
                if (MallocExtension_ReleaseFreeMemory != nullptr) {
                    MallocExtension_ReleaseFreeMemory();
                } else {
#if defined(OS_LINUX)
                    // GNU specific.
                    malloc_trim(10 * 1024 * 1024/*leave 10M pad*/);
#endif
                }
            }
        }
        return nullptr;
    }

    static void BaiduStreamingLogHandler(google::protobuf::LogLevel level,
                                         const char *filename, int line,
                                         const std::string &message) {
        switch (level) {
            case google::protobuf::LOGLEVEL_INFO:
                LOG(INFO) << filename << ':' << line << ' ' << message;
                return;
            case google::protobuf::LOGLEVEL_WARNING:
                LOG(WARNING) << filename << ':' << line << ' ' << message;
                return;
            case google::protobuf::LOGLEVEL_ERROR:
                LOG(ERROR) << filename << ':' << line << ' ' << message;
                return;
            case google::protobuf::LOGLEVEL_FATAL:
                LOG(FATAL) << filename << ':' << line << ' ' << message;
                return;
        }
        CHECK(false) << filename << ':' << line << ' ' << message;
    }

    static void GlobalInitializeOrDieImpl() {
        //////////////////////////////////////////////////////////////////
        // Be careful about usages of gflags inside this function which //
        // may be called before main() only seeing gflags with default  //
        // values even if the gflags will be set after main().          //
        //////////////////////////////////////////////////////////////////

        // Ignore SIGPIPE.
        struct sigaction oldact;
        if (sigaction(SIGPIPE, nullptr, &oldact) != 0 ||
            (oldact.sa_handler == nullptr && oldact.sa_sigaction == nullptr)) {
            CHECK(SIG_ERR != signal(SIGPIPE, SIG_IGN));
        }

        // Make GOOGLE_LOG print to comlog device
        SetLogHandler(&BaiduStreamingLogHandler);

        // Setting the variable here does not work, the profiler probably check
        // the variable before main() for only once.
        // setenv("TCMALLOC_SAMPLE_PARAMETER", "524288", 0);

        // Initialize openssl library
        SSL_library_init();
        // RPC doesn't require openssl.cnf, users can load it by themselves if needed
        SSL_load_error_strings();
        if (SSLThreadInit() != 0 || SSLDHInit() != 0) {
            exit(1);
        }

        // Defined in http_rpc_protocol.cpp
        InitCommonStrings();

        // Leave memory of these extensions to process's clean up.
        g_ext = new(std::nothrow) GlobalExtensions();
        if (nullptr == g_ext) {
            exit(1);
        }
        // Naming Services
        NamingServiceExtension()->RegisterOrDie("file", &g_ext->fns);
        NamingServiceExtension()->RegisterOrDie("list", &g_ext->lns);
        NamingServiceExtension()->RegisterOrDie("dlist", &g_ext->dlns);
        NamingServiceExtension()->RegisterOrDie("http", &g_ext->dns);
        NamingServiceExtension()->RegisterOrDie("https", &g_ext->dns_with_ssl);
        NamingServiceExtension()->RegisterOrDie("redis", &g_ext->dns);
        NamingServiceExtension()->RegisterOrDie("remotefile", &g_ext->rfns);
        NamingServiceExtension()->RegisterOrDie("consul", &g_ext->cns);
        NamingServiceExtension()->RegisterOrDie("discovery", &g_ext->dcns);
        NamingServiceExtension()->RegisterOrDie("nacos", &g_ext->nns);

        // Load Balancers
        LoadBalancerExtension()->RegisterOrDie("rr", &g_ext->rr_lb);
        LoadBalancerExtension()->RegisterOrDie("wrr", &g_ext->wrr_lb);
        LoadBalancerExtension()->RegisterOrDie("random", &g_ext->randomized_lb);
        LoadBalancerExtension()->RegisterOrDie("wr", &g_ext->wr_lb);
        LoadBalancerExtension()->RegisterOrDie("la", &g_ext->la_lb);
        LoadBalancerExtension()->RegisterOrDie("c_murmurhash", &g_ext->ch_mh_lb);
        LoadBalancerExtension()->RegisterOrDie("c_md5", &g_ext->ch_md5_lb);
        LoadBalancerExtension()->RegisterOrDie("c_ketama", &g_ext->ch_ketama_lb);
        LoadBalancerExtension()->RegisterOrDie("_dynpart", &g_ext->dynpart_lb);

        // Compress Handlers
        const CompressHandler gzip_compress =
                {melon::compress::GzipCompress, melon::compress::GzipDecompress, "gzip"};
        if (RegisterCompressHandler(COMPRESS_TYPE_GZIP, gzip_compress) != 0) {
            exit(1);
        }
        const CompressHandler zlib_compress =
                {melon::compress::ZlibCompress, melon::compress::ZlibDecompress, "zlib"};
        if (RegisterCompressHandler(COMPRESS_TYPE_ZLIB, zlib_compress) != 0) {
            exit(1);
        }
        const CompressHandler snappy_compress =
                {melon::compress::SnappyCompress, melon::compress::SnappyDecompress, "snappy"};
        if (RegisterCompressHandler(COMPRESS_TYPE_SNAPPY, snappy_compress) != 0) {
            exit(1);
        }

        // Protocols
        Protocol melon_protocol = {ParseMStdMessage,
                                   SerializeRequestDefault, PackMStdRequest,
                                   ProcessMStdRequest, ProcessMStdResponse,
                                   VerifyMStdRequest, nullptr, nullptr,
                                   CONNECTION_TYPE_ALL, "melon_std"};
        if (RegisterProtocol(PROTOCOL_MELON_STD, melon_protocol) != 0) {
            exit(1);
        }

        Protocol brpc_protocol = {ParseBRPCMessage,
                                   SerializeRequestDefault, PackBRPCRequest,
                                   ProcessBRPCRequest, ProcessBRPCResponse,
                                   VerifyBRPCRequest, nullptr, nullptr,
                                   CONNECTION_TYPE_ALL, "baidu_std"};
        if (RegisterProtocol(PROTOCOL_BRPC, brpc_protocol) != 0) {
            exit(1);
        }

        Protocol streaming_protocol = {ParseStreamingMessage,
                                       nullptr, nullptr, ProcessStreamingMessage,
                                       ProcessStreamingMessage,
                                       nullptr, nullptr, nullptr,
                                       CONNECTION_TYPE_SINGLE, "streaming_rpc"};

        if (RegisterProtocol(PROTOCOL_STREAMING_RPC, streaming_protocol) != 0) {
            exit(1);
        }

        Protocol http_protocol = {ParseHttpMessage,
                                  SerializeHttpRequest, PackHttpRequest,
                                  ProcessHttpRequest, ProcessHttpResponse,
                                  VerifyHttpRequest, ParseHttpServerAddress,
                                  GetHttpMethodName,
                                  CONNECTION_TYPE_POOLED_AND_SHORT,
                                  "http"};
        if (RegisterProtocol(PROTOCOL_HTTP, http_protocol) != 0) {
            exit(1);
        }

        Protocol http2_protocol = {ParseH2Message,
                                   SerializeHttpRequest, PackH2Request,
                                   ProcessHttpRequest, ProcessHttpResponse,
                                   VerifyHttpRequest, ParseHttpServerAddress,
                                   GetHttpMethodName,
                                   CONNECTION_TYPE_SINGLE,
                                   "h2"};
        if (RegisterProtocol(PROTOCOL_H2, http2_protocol) != 0) {
            exit(1);
        }

        Protocol hulu_protocol = {ParseHuluMessage,
                                  SerializeRequestDefault, PackHuluRequest,
                                  ProcessHuluRequest, ProcessHuluResponse,
                                  VerifyHuluRequest, nullptr, nullptr,
                                  CONNECTION_TYPE_ALL, "hulu_pbrpc"};
        if (RegisterProtocol(PROTOCOL_HULU_PBRPC, hulu_protocol) != 0) {
            exit(1);
        }


        Protocol mc_binary_protocol = {ParseMemcacheMessage,
                                       SerializeMemcacheRequest,
                                       PackMemcacheRequest,
                                       nullptr, ProcessMemcacheResponse,
                                       nullptr, nullptr, GetMemcacheMethodName,
                                       CONNECTION_TYPE_ALL, "memcache"};
        if (RegisterProtocol(PROTOCOL_MEMCACHE, mc_binary_protocol) != 0) {
            exit(1);
        }

        Protocol redis_protocol = {ParseRedisMessage,
                                   SerializeRedisRequest,
                                   PackRedisRequest,
                                   ProcessRedisRequest, ProcessRedisResponse,
                                   nullptr, nullptr, GetRedisMethodName,
                                   CONNECTION_TYPE_ALL, "redis"};
        if (RegisterProtocol(PROTOCOL_REDIS, redis_protocol) != 0) {
            exit(1);
        }

        Protocol mongo_protocol = {ParseMongoMessage,
                                   nullptr, nullptr,
                                   ProcessMongoRequest, nullptr,
                                   nullptr, nullptr, nullptr,
                                   CONNECTION_TYPE_POOLED, "mongo"};
        if (RegisterProtocol(PROTOCOL_MONGO, mongo_protocol) != 0) {
            exit(1);
        }

        Protocol rtmp_protocol = {
                ParseRtmpMessage,
                SerializeRtmpRequest, PackRtmpRequest,
                ProcessRtmpMessage, ProcessRtmpMessage,
                nullptr, nullptr, nullptr,
                (ConnectionType)(CONNECTION_TYPE_SINGLE | CONNECTION_TYPE_SHORT),
                "rtmp"};
        if (RegisterProtocol(PROTOCOL_RTMP, rtmp_protocol) != 0) {
            exit(1);
        }

        std::vector<Protocol> protocols;
        ListProtocols(&protocols);
        for (size_t i = 0; i < protocols.size(); ++i) {
            if (protocols[i].process_response) {
                InputMessageHandler handler;
                // `process_response' is required at client side
                handler.parse = protocols[i].parse;
                handler.process = protocols[i].process_response;
                // No need to verify at client side
                handler.verify = nullptr;
                handler.arg = nullptr;
                handler.name = protocols[i].name;
                if (get_or_new_client_side_messenger()->AddHandler(handler) != 0) {
                    exit(1);
                }
            }
        }

        // Concurrency Limiters
        ConcurrencyLimiterExtension()->RegisterOrDie("auto", &g_ext->auto_cl);
        ConcurrencyLimiterExtension()->RegisterOrDie("constant", &g_ext->constant_cl);
        ConcurrencyLimiterExtension()->RegisterOrDie("timeout", &g_ext->timeout_cl);

        if (FLAGS_usercode_in_pthread) {
            // Optional. If channel/server are initialized before main(), this
            // flag may be false at here even if it will be set to true after
            // main(). In which case, the usercode pool will not be initialized
            // until the pool is used.
            InitUserCodeBackupPoolOnceOrDie();
        }

        // We never join GlobalUpdate, let it quit with the process.
        fiber_t th;
        CHECK(fiber_start_background(&th, nullptr, GlobalUpdate, nullptr) == 0)
        << "Fail to start GlobalUpdate";
    }

    void GlobalInitializeOrDie() {
        if (pthread_once(&register_extensions_once,
                         GlobalInitializeOrDieImpl) != 0) {
            LOG(FATAL) << "Fail to pthread_once";
            exit(1);
        }
    }

} // namespace melon
