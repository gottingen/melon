// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.



// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/types.h>
#include <sys/socket.h>
#include <map>
#include <algorithm>
#include <random>
#include "testing/gtest_wrap.h"
#include "melon/fiber/internal/fiber.h"
#include "melon/base/gperftools_profiler.h"
#include "melon/times/time.h"
#include "melon/base/fast_rand.h"
#include "melon/container/doubly_buffered_data.h"
#include "melon/rpc/describable.h"
#include "melon/rpc/socket.h"
#include "melon/strings/numbers.h"
#include "melon/rpc/excluded_servers.h"
#include "melon/rpc/policy/weighted_round_robin_load_balancer.h"
#include "melon/rpc/policy/round_robin_load_balancer.h"
#include "melon/rpc/policy/weighted_randomized_load_balancer.h"
#include "melon/rpc/policy/randomized_load_balancer.h"
#include "melon/rpc/policy/locality_aware_load_balancer.h"
#include "melon/rpc/policy/consistent_hashing_load_balancer.h"
#include "melon/rpc/policy/hasher.h"
#include "melon/rpc/errno.pb.h"
#include "echo.pb.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/server.h"

namespace melon::rpc {
    DECLARE_int32(health_check_interval);
    DECLARE_int64(detect_available_server_interval_ms);
    namespace policy {
        extern uint32_t CRCHash32(const char *key, size_t len);

        extern const char *GetHashName(uint32_t (*hasher)(const void *key, size_t len));
    }
}

namespace {
    void initialize_random() {
        srand(time(0));
    }

    pthread_once_t initialize_random_control = PTHREAD_ONCE_INIT;

    class LoadBalancerTest : public ::testing::Test {
    protected:
        LoadBalancerTest() {
            pthread_once(&initialize_random_control, initialize_random);
        };

        virtual ~LoadBalancerTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    size_t TLS_ctor = 0;
    size_t TLS_dtor = 0;

    struct TLS {
        TLS() {
            ++TLS_ctor;
        }

        ~TLS() {
            ++TLS_dtor;
        }

    };

    struct Foo {
        Foo() : x(0) {}

        int x;
    };

    bool AddN(Foo &f, int n) {
        f.x += n;
        return true;
    }

    TEST_F(LoadBalancerTest, doubly_buffered_data) {
        const size_t old_TLS_ctor = TLS_ctor;
        const size_t old_TLS_dtor = TLS_dtor;
        {
            melon::container::DoublyBufferedData<Foo, TLS> d2;
            melon::container::DoublyBufferedData<Foo, TLS>::ScopedPtr ptr;
            d2.Read(&ptr);
            ASSERT_EQ(old_TLS_ctor + 1, TLS_ctor);
        }
        ASSERT_EQ(old_TLS_ctor + 1, TLS_ctor);
        ASSERT_EQ(old_TLS_dtor + 1, TLS_dtor);

        melon::container::DoublyBufferedData<Foo> d;
        {
            melon::container::DoublyBufferedData<Foo>::ScopedPtr ptr;
            ASSERT_EQ(0, d.Read(&ptr));
            ASSERT_EQ(0, ptr->x);
        }
        {
            melon::container::DoublyBufferedData<Foo>::ScopedPtr ptr;
            ASSERT_EQ(0, d.Read(&ptr));
            ASSERT_EQ(0, ptr->x);
        }

        d.Modify(AddN, 10);
        {
            melon::container::DoublyBufferedData<Foo>::ScopedPtr ptr;
            ASSERT_EQ(0, d.Read(&ptr));
            ASSERT_EQ(10, ptr->x);
        }
    }

    typedef melon::rpc::policy::LocalityAwareLoadBalancer LALB;

    static void ValidateWeightTree(
            std::vector<LALB::ServerInfo> &weight_tree) {
        std::vector<int64_t> weight_sum;
        weight_sum.resize(weight_tree.size());
        for (ssize_t i = weight_tree.size() - 1; i >= 0; --i) {
            const size_t left_child = i * 2 + 1;
            const size_t right_child = i * 2 + 2;
            weight_sum[i] = weight_tree[i].weight->volatile_value();
            if (left_child < weight_sum.size()) {
                weight_sum[i] += weight_sum[left_child];
            }
            if (right_child < weight_sum.size()) {
                weight_sum[i] += weight_sum[right_child];
            }
        }
        for (size_t i = 0; i < weight_tree.size(); ++i) {
            const int64_t left = weight_tree[i].left->load(std::memory_order_relaxed);
            size_t left_child = i * 2 + 1;
            if (left_child < weight_tree.size()) {
                ASSERT_EQ(weight_sum[left_child], left) << "i=" << i;
            } else {
                ASSERT_EQ(0, left);
            }
        }
    }

    static void ValidateLALB(LALB &lalb, size_t N) {
        LALB::Servers *d = lalb._db_servers._data;
        for (size_t R = 0; R < 2; ++R) {
            ASSERT_EQ(d[R].weight_tree.size(), N);
            ASSERT_EQ(d[R].server_map.size(), N);
        }
        ASSERT_EQ(lalb._left_weights.size(), N);
        int64_t total = 0;
        for (size_t i = 0; i < N; ++i) {
            ASSERT_EQ(d[0].weight_tree[i].server_id, d[1].weight_tree[i].server_id);
            ASSERT_EQ(d[0].weight_tree[i].weight, d[1].weight_tree[i].weight);
            for (size_t R = 0; R < 2; ++R) {
                ASSERT_EQ((int64_t *) d[R].weight_tree[i].left, &lalb._left_weights[i]);
                size_t * pindex = d[R].server_map.seek(d[R].weight_tree[i].server_id);
                ASSERT_TRUE(pindex != NULL && *pindex == i);
            }
            total += d[0].weight_tree[i].weight->volatile_value();
        }
        ValidateWeightTree(d[0].weight_tree);
        ASSERT_EQ(total, lalb._total.load());
    }

    TEST_F(LoadBalancerTest, la_sanity) {
        LALB lalb;
        ASSERT_EQ(0, lalb._total.load());
        std::vector<melon::rpc::ServerId> ids;
        const size_t N = 256;
        size_t cur_count = 0;

        std::random_device rd;
        std::mt19937 g(rd());
        for (int REP = 0; REP < 5; ++REP) {
            const size_t before_adding = cur_count;
            for (; cur_count < N; ++cur_count) {
                char addr[32];
                snprintf(addr, sizeof(addr), "192.168.1.%d:8080", (int) cur_count);
                melon::base::end_point dummy;
                ASSERT_EQ(0, str2endpoint(addr, &dummy));
                melon::rpc::ServerId id(8888);
                melon::rpc::SocketOptions options;
                options.remote_side = dummy;
                ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
                ids.push_back(id);
                ASSERT_TRUE(lalb.AddServer(id));
            }
            std::cout << "Added " << cur_count - before_adding << std::endl;
            ValidateLALB(lalb, cur_count);

            const size_t before_removal = cur_count;

            std::shuffle(ids.begin(), ids.end(), g);
            for (size_t i = 0; i < N / 2; ++i) {
                const melon::rpc::ServerId id = ids.back();
                ids.pop_back();
                --cur_count;
                ASSERT_TRUE(lalb.RemoveServer(id)) << "i=" << i;
                ASSERT_EQ(0, melon::rpc::Socket::SetFailed(id.id));
            }
            std::cout << "Removed " << before_removal - cur_count << std::endl;
            ValidateLALB(lalb, cur_count);
        }

        for (size_t i = 0; i < ids.size(); ++i) {
            ASSERT_EQ(0, melon::rpc::Socket::SetFailed(ids[i].id));
        }
    }

    typedef std::map<melon::rpc::SocketId, int> CountMap;
    volatile bool global_stop = false;

    struct SelectArg {
        melon::rpc::LoadBalancer *lb;

        uint32_t (*hash)(const void *, size_t);
    };

    void *select_server(void *arg) {
        SelectArg *sa = (SelectArg *) arg;
        melon::rpc::LoadBalancer *c = sa->lb;
        melon::rpc::SocketUniquePtr ptr;
        CountMap *selected_count = new CountMap;
        melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
        melon::rpc::LoadBalancer::SelectOut out(&ptr);
        uint32_t rand_seed = rand();
        if (sa->hash) {
            uint32_t rd = ++rand_seed;
            in.has_request_code = true;
            in.request_code = sa->hash((const char *) &rd, sizeof(uint32_t));
        }
        int ret = 0;
        while (!global_stop && (ret = c->SelectServer(in, &out)) == 0) {
            if (sa->hash) {
                uint32_t rd = ++rand_seed;
                in.has_request_code = true;
                in.request_code = sa->hash((const char *) &rd, sizeof(uint32_t));
            }
            ++(*selected_count)[ptr->id()];
        }
        MELON_LOG_IF(INFO, ret != 0) << "select_server[" << pthread_self()
                               << "] quits before of " << melon_error(ret);
        return selected_count;
    }

    melon::rpc::SocketId recycled_sockets[1024];
    std::atomic<size_t> nrecycle(0);

    class SaveRecycle : public melon::rpc::SocketUser {
        void BeforeRecycle(melon::rpc::Socket *s) {
            recycled_sockets[nrecycle.fetch_add(1, std::memory_order_relaxed)] = s->id();
            delete this;
        }
    };

    TEST_F(LoadBalancerTest, update_while_selection) {
        for (size_t round = 0; round < 5; ++round) {
            melon::rpc::LoadBalancer *lb = NULL;
            SelectArg sa = {NULL, NULL};
            bool is_lalb = false;
            if (round == 0) {
                lb = new melon::rpc::policy::RoundRobinLoadBalancer;
            } else if (round == 1) {
                lb = new melon::rpc::policy::RandomizedLoadBalancer;
            } else if (round == 2) {
                lb = new LALB;
                is_lalb = true;
            } else if (round == 3) {
                lb = new melon::rpc::policy::WeightedRoundRobinLoadBalancer;
            } else {
                lb = new melon::rpc::policy::ConsistentHashingLoadBalancer(melon::rpc::policy::CONS_HASH_LB_MURMUR3);
                sa.hash = ::melon::rpc::policy::MurmurHash32;
            }
            sa.lb = lb;

            // Accessing empty lb should result in error.
            melon::rpc::SocketUniquePtr ptr;
            melon::rpc::LoadBalancer::SelectIn in = {0, false, true, 0, NULL};
            melon::rpc::LoadBalancer::SelectOut out(&ptr);
            ASSERT_EQ(ENODATA, lb->SelectServer(in, &out));

            nrecycle = 0;
            global_stop = false;
            pthread_t th[8];
            std::vector<melon::rpc::ServerId> ids;
            melon::rpc::SocketId wrr_sid_logoff = -1;
            for (int i = 0; i < 256; ++i) {
                char addr[32];
                snprintf(addr, sizeof(addr), "192.%d.1.%d:8080", i, i);
                melon::base::end_point dummy;
                ASSERT_EQ(0, str2endpoint(addr, &dummy));
                melon::rpc::ServerId id(8888);
                if (3 == round) {
                    if (i < 255) {
                        id.tag = "1";
                    } else {
                        id.tag = "200000000";
                    }
                }
                melon::rpc::SocketOptions options;
                options.remote_side = dummy;
                options.user = new SaveRecycle;
                ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
                ids.push_back(id);
                ASSERT_TRUE(lb->AddServer(id));
                if (round == 3 && i == 255) {
                    wrr_sid_logoff = id.id;
                    // In case of wrr, set 255th socket with huge weight logoff.
                    melon::rpc::SocketUniquePtr ptr;
                    ASSERT_EQ(0, melon::rpc::Socket::Address(id.id, &ptr));
                    ptr->SetLogOff();
                }
            }
            std::cout << "Time " << melon::base::class_name_str(*lb) << " ..." << std::endl;
            melon::stop_watcher tm;
            tm.start();
            for (size_t i = 0; i < MELON_ARRAY_SIZE(th); ++i) {
                ASSERT_EQ(0, pthread_create(&th[i], NULL, select_server, &sa));
            }
            std::vector<melon::rpc::ServerId> removed;
            const size_t REP = 200;
            std::random_device rd;
            std::mt19937 g(rd());
            for (size_t k = 0; k < REP; ++k) {
                if (round != 3) {
                    removed = ids;
                } else {
                    removed.assign(ids.begin(), ids.begin() + 255);
                }

                std::shuffle(removed.begin(), removed.end(), g);
                removed.pop_back();
                ASSERT_EQ(removed.size(), lb->RemoveServersInBatch(removed));
                ASSERT_EQ(removed.size(), lb->AddServersInBatch(removed));
                // // 1: Don't remove first server, otherwise select_server would quit.
                // for (size_t i = 1/*1*/; i < removed.size(); ++i) {
                //    ASSERT_TRUE(lb->RemoveServer(removed[i]));
                // }
                // for (size_t i = 1; i < removed.size(); ++i) {
                //    ASSERT_TRUE(lb->AddServer(removed[i]));
                // }
                if (is_lalb) {
                    LALB *lalb = (LALB *) lb;
                    ValidateLALB(*lalb, ids.size());
                    ASSERT_GT(lalb->_total.load(), 0);
                }
            }
            global_stop = true;
            MELON_LOG(INFO) << "Stop all...";

            void *retval[MELON_ARRAY_SIZE(th)];
            for (size_t i = 0; i < MELON_ARRAY_SIZE(th); ++i) {
                ASSERT_EQ(0, pthread_join(th[i], &retval[i]));
            }
            tm.stop();

            CountMap total_count;
            for (size_t i = 0; i < MELON_ARRAY_SIZE(th); ++i) {
                CountMap *selected_count = (CountMap *) retval[i];
                size_t count = 0;
                for (CountMap::const_iterator it = selected_count->begin();
                     it != selected_count->end(); ++it) {
                    total_count[it->first] += it->second;
                    count += it->second;
                }
                delete selected_count;

                std::cout << "thread " << i << " selected "
                          << count * 1000000L / tm.u_elapsed() << " times/s"
                          << std::endl;
            }
            size_t id_num = ids.size();
            if (round == 3) {
                // Do not include the logoff socket.
                id_num -= 1;
            }
            ASSERT_EQ(id_num, total_count.size());
            for (size_t i = 0; i < id_num; ++i) {
                ASSERT_NE(0, total_count[ids[i].id]) << "i=" << i;
                std::cout << i << "=" << total_count[ids[i].id] << " ";
            }
            std::cout << std::endl;

            for (size_t i = 0; i < id_num; ++i) {
                ASSERT_EQ(0, melon::rpc::Socket::SetFailed(ids[i].id));
            }
            ASSERT_EQ(ids.size(), nrecycle);
            melon::rpc::SocketId id = -1;
            for (size_t i = 0; i < ids.size(); ++i) {
                id = recycled_sockets[i];
                if (id != wrr_sid_logoff) {
                    ASSERT_EQ(1UL, total_count.erase(id));
                } else {
                    ASSERT_EQ(0UL, total_count.erase(id));
                }
            }
            delete lb;
        }
    }

    TEST_F(LoadBalancerTest, fairness) {
        for (size_t round = 0; round < 6; ++round) {
            melon::rpc::LoadBalancer *lb = NULL;
            SelectArg sa = {NULL, NULL};
            if (round == 0) {
                lb = new melon::rpc::policy::RoundRobinLoadBalancer;
            } else if (round == 1) {
                lb = new melon::rpc::policy::RandomizedLoadBalancer;
            } else if (round == 2) {
                lb = new LALB;
            } else if (3 == round || 4 == round) {
                lb = new melon::rpc::policy::WeightedRoundRobinLoadBalancer;
            } else {
                lb = new melon::rpc::policy::ConsistentHashingLoadBalancer(melon::rpc::policy::CONS_HASH_LB_MURMUR3);
                sa.hash = melon::rpc::policy::MurmurHash32;
            }
            sa.lb = lb;

            std::string lb_name = melon::base::class_name_str(*lb);
            // Remove namespace
            size_t ns_pos = lb_name.find_last_of(':');
            if (ns_pos != std::string::npos) {
                lb_name = lb_name.substr(ns_pos + 1);
            }

            nrecycle = 0;
            global_stop = false;
            pthread_t th[8];
            std::vector<melon::rpc::ServerId> ids;
            for (int i = 0; i < 256; ++i) {
                char addr[32];
                snprintf(addr, sizeof(addr), "192.168.1.%d:8080", i);
                melon::base::end_point dummy;
                ASSERT_EQ(0, str2endpoint(addr, &dummy));
                melon::rpc::ServerId id(8888);
                if (3 == round) {
                    id.tag = "100";
                } else if (4 == round) {
                    if (i % 50 == 0) {
                        id.tag = std::to_string(i * 2 + melon::base::fast_rand_less_than(40) + 80);
                    } else {
                        id.tag = std::to_string(melon::base::fast_rand_less_than(40) + 80);
                    }
                }
                melon::rpc::SocketOptions options;
                options.remote_side = dummy;
                options.user = new SaveRecycle;
                ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
                ids.push_back(id);
                lb->AddServer(id);
            }

            for (size_t i = 0; i < MELON_ARRAY_SIZE(th); ++i) {
                ASSERT_EQ(0, pthread_create(&th[i], NULL, select_server, &sa));
            }
            melon::fiber_sleep_for(10000);
            ProfilerStart((lb_name + ".prof").c_str());
            melon::fiber_sleep_for(300000);
            ProfilerStop();

            global_stop = true;

            CountMap total_count;
            for (size_t i = 0; i < MELON_ARRAY_SIZE(th); ++i) {
                void *retval;
                ASSERT_EQ(0, pthread_join(th[i], &retval));
                CountMap *selected_count = (CountMap *) retval;
                ASSERT_TRUE(selected_count);
                int first_count = 0;
                for (CountMap::const_iterator it = selected_count->begin();
                     it != selected_count->end(); ++it) {
                    if (round == 0) {
                        if (first_count == 0) {
                            first_count = it->second;
                        } else {
                            // Load is not ensured to be fair inside each thread
                            // ASSERT_LE(abs(first_count - it->second), 1);
                        }
                    }
                    total_count[it->first] += it->second;
                }
                delete selected_count;
            }
            ASSERT_EQ(ids.size(), total_count.size());
            size_t count_sum = 0;
            size_t count_squared_sum = 0;
            std::cout << lb_name << ':' << '\n';

            if (round != 3 && round != 4) {
                for (size_t i = 0; i < ids.size(); ++i) {
                    size_t count = total_count[ids[i].id];
                    ASSERT_NE(0ul, count) << "i=" << i;
                    std::cout << i << '=' << count << ' ';
                    count_sum += count;
                    count_squared_sum += count * count;
                }

                std::cout << '\n'
                          << ": average=" << count_sum / ids.size()
                          << " deviation=" << sqrt(count_squared_sum * ids.size()
                                                   - count_sum * count_sum) / ids.size() << std::endl;
            } else { // for weighted round robin load balancer
                std::cout << "configured weight: " << std::endl;
                std::ostringstream os;
                melon::rpc::DescribeOptions opt;
                lb->Describe(os, opt);
                std::cout << os.str() << std::endl;
                double scaling_count_sum = 0.0;
                double scaling_count_squared_sum = 0.0;
                for (size_t i = 0; i < ids.size(); ++i) {
                    size_t count = total_count[ids[i].id];
                    ASSERT_NE(0ul, count) << "i=" << i;
                    std::cout << i << '=' << count << ' ';
                    double scaling_count = static_cast<double>(count) / std::stoi(ids[i].tag);
                    scaling_count_sum += scaling_count;
                    scaling_count_squared_sum += scaling_count * scaling_count;
                }
                std::cout << '\n'
                          << ": scaling average=" << scaling_count_sum / ids.size()
                          << " scaling deviation=" << sqrt(scaling_count_squared_sum * ids.size()
                                                           - scaling_count_sum * scaling_count_sum) / ids.size()
                          << std::endl;
            }
            for (size_t i = 0; i < ids.size(); ++i) {
                ASSERT_EQ(0, melon::rpc::Socket::SetFailed(ids[i].id));
            }
            ASSERT_EQ(ids.size(), nrecycle);
            for (size_t i = 0; i < ids.size(); ++i) {
                ASSERT_EQ(1UL, total_count.erase(recycled_sockets[i]));
            }
            delete lb;
        }
    }

    TEST_F(LoadBalancerTest, consistent_hashing) {
        ::melon::rpc::policy::HashFunc hashs[::melon::rpc::policy::CONS_HASH_LB_LAST] = {
                ::melon::rpc::policy::MurmurHash32,
                ::melon::rpc::policy::MD5Hash32,
                ::melon::rpc::policy::MD5Hash32
                // ::melon::rpc::policy::CRCHash32 crc is a bad hash function in test
        };

        ::melon::rpc::policy::ConsistentHashingLoadBalancerType hash_type[::melon::rpc::policy::CONS_HASH_LB_LAST] = {
                ::melon::rpc::policy::CONS_HASH_LB_MURMUR3,
                ::melon::rpc::policy::CONS_HASH_LB_MD5,
                ::melon::rpc::policy::CONS_HASH_LB_KETAMA
        };

        const char *servers[] = {
                "10.92.115.19:8833",
                "10.42.108.25:8833",
                "10.36.150.32:8833",
                "10.92.149.48:8833",
                "10.42.122.201:8833",
        };
        for (size_t round = 0; round < MELON_ARRAY_SIZE(hashs); ++round) {
            melon::rpc::policy::ConsistentHashingLoadBalancer chlb(hash_type[round]);
            std::vector<melon::rpc::ServerId> ids;
            std::vector<melon::base::end_point> addrs;
            for (int j = 0; j < 5; ++j)
                for (int i = 0; i < 5; ++i) {
                    const char *addr = servers[i];
                    //snprintf(addr, sizeof(addr), "192.168.1.%d:8080", i);
                    melon::base::end_point dummy;
                    ASSERT_EQ(0, str2endpoint(addr, &dummy));
                    melon::rpc::ServerId id(8888);
                    melon::rpc::SocketOptions options;
                    options.remote_side = dummy;
                    options.user = new SaveRecycle;
                    ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
                    ids.push_back(id);
                    addrs.push_back(dummy);
                    chlb.AddServer(id);
                }
            std::cout << chlb;
            for (int i = 0; i < 5; ++i) {
                std::vector<melon::rpc::ServerId> empty;
                chlb.AddServersInBatch(empty);
                chlb.RemoveServersInBatch(empty);
                std::cout << chlb;
            }
            const size_t SELECT_TIMES = 1000000;
            std::map < melon::base::end_point, size_t > times;
            melon::rpc::SocketUniquePtr ptr;
            melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
            ::melon::rpc::LoadBalancer::SelectOut out(&ptr);
            for (size_t i = 0; i < SELECT_TIMES; ++i) {
                in.has_request_code = true;
                in.request_code = hashs[round]((const char *) &i, sizeof(i));
                chlb.SelectServer(in, &out);
                ++times[ptr->remote_side()];
            }
            std::map<melon::base::end_point, double> load_map;
            chlb.GetLoads(&load_map);
            ASSERT_EQ(times.size(), load_map.size());
            double load_sum = 0;;
            double load_sqr_sum = 0;
            for (size_t i = 0; i < addrs.size(); ++i) {
                double normalized_load =
                        (double) times[addrs[i]] / SELECT_TIMES / load_map[addrs[i]];
                std::cout << i << '=' << normalized_load << ' ';
                load_sum += normalized_load;
                load_sqr_sum += normalized_load * normalized_load;
            }
            std::cout << '\n';
            std::cout << "average_normalized_load=" << load_sum / addrs.size()
                      << " deviation="
                      << sqrt(load_sqr_sum * addrs.size() - load_sum * load_sum) / addrs.size()
                      << '\n';
            for (size_t i = 0; i < ids.size(); ++i) {
                ASSERT_EQ(0, melon::rpc::Socket::SetFailed(ids[i].id));
            }
        }
    }

    TEST_F(LoadBalancerTest, weighted_round_robin) {
        const char *servers[] = {
                "10.92.115.19:8831",
                "10.42.108.25:8832",
                "10.36.150.32:8833",
                "10.36.150.32:8899",
                "10.92.149.48:8834",
                "10.42.122.201:8835",
                "10.42.122.202:8836"
        };
        std::string weight[] = {"3", "2", "7", "200000000", "1ab", "-1", "0"};
        std::map<melon::base::end_point, int> configed_weight;
        melon::rpc::policy::WeightedRoundRobinLoadBalancer wrrlb;

        // Add server to selected list. The server with invalid weight will be skipped.
        for (size_t i = 0; i < MELON_ARRAY_SIZE(servers); ++i) {
            const char *addr = servers[i];
            melon::base::end_point dummy;
            ASSERT_EQ(0, str2endpoint(addr, &dummy));
            melon::rpc::ServerId id(8888);
            melon::rpc::SocketOptions options;
            options.remote_side = dummy;
            options.user = new SaveRecycle;
            ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
            id.tag = weight[i];
            if (i == 3) {
                melon::rpc::SocketUniquePtr ptr;
                ASSERT_EQ(0, melon::rpc::Socket::Address(id.id, &ptr));
                ptr->SetLogOff();
            }
            if (i < 4) {
                int weight_num;
                ASSERT_TRUE(melon::simple_atoi(weight[i], &weight_num));
                configed_weight[dummy] = weight_num;
                EXPECT_TRUE(wrrlb.AddServer(id));
            } else {
                EXPECT_FALSE(wrrlb.AddServer(id));
            }
        }

        // Select the best server according to weight configured.
        // There are 3 valid servers with weight 3, 2 and 7 respectively.
        // We run SelectServer for 12 times. The result number of each server seleted should be
        // consistent with weight configured.
        std::map < melon::base::end_point, size_t > select_result;
        melon::rpc::SocketUniquePtr ptr;
        melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
        melon::rpc::LoadBalancer::SelectOut out(&ptr);
        int total_weight = 12;
        std::vector<melon::base::end_point> select_servers;
        for (int i = 0; i != total_weight; ++i) {
            EXPECT_EQ(0, wrrlb.SelectServer(in, &out));
            select_servers.emplace_back(ptr->remote_side());
            ++select_result[ptr->remote_side()];
        }

        for (const auto &s : select_servers) {
            std::cout << "1=" << s << ", ";
        }
        std::cout << std::endl;
        // Check whether slected result is consistent with expected.
        EXPECT_EQ((size_t) 3, select_result.size());
        for (const auto &result : select_result) {
            std::cout << result.first << " result=" << result.second
                      << " configured=" << configed_weight[result.first] << std::endl;
            EXPECT_EQ(result.second, (size_t) configed_weight[result.first]);
        }
    }

    TEST_F(LoadBalancerTest, weighted_round_robin_no_valid_server) {
        const char *servers[] = {
                "10.92.115.19:8831",
                "10.42.108.25:8832",
                "10.36.150.32:8833"
        };
        std::string weight[] = {"200000000", "2", "600000"};
        std::map<melon::base::end_point, int> configed_weight;
        melon::rpc::policy::WeightedRoundRobinLoadBalancer wrrlb;
        melon::rpc::ExcludedServers *exclude = melon::rpc::ExcludedServers::Create(3);
        for (size_t i = 0; i < MELON_ARRAY_SIZE(servers); ++i) {
            const char *addr = servers[i];
            melon::base::end_point dummy;
            ASSERT_EQ(0, str2endpoint(addr, &dummy));
            melon::rpc::ServerId id(8888);
            melon::rpc::SocketOptions options;
            options.remote_side = dummy;
            options.user = new SaveRecycle;
            id.tag = weight[i];
            if (i < 2) {
                ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
            }
            EXPECT_TRUE(wrrlb.AddServer(id));
            if (i == 0) {
                exclude->Add(id.id);
            }
            if (i == 1) {
                melon::rpc::SocketUniquePtr ptr;
                ASSERT_EQ(0, melon::rpc::Socket::Address(id.id, &ptr));
                ptr->SetLogOff();
            }
        }
        // The first socket is excluded. The second socket is logfoff.
        // The third socket is invalid.
        melon::rpc::SocketUniquePtr ptr;
        melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, exclude};
        melon::rpc::LoadBalancer::SelectOut out(&ptr);
        EXPECT_EQ(EHOSTDOWN, wrrlb.SelectServer(in, &out));
        melon::rpc::ExcludedServers::Destroy(exclude);
    }

    TEST_F(LoadBalancerTest, weighted_randomized) {
        const char *servers[] = {
                "10.92.115.19:8831",
                "10.42.108.25:8832",
                "10.36.150.31:8833",
                "10.36.150.32:8899",
                "10.92.149.48:8834",
                "10.42.122.201:8835",
                "10.42.122.202:8836"
        };
        std::string weight[] = {"3", "2", "5", "10", "1ab", "-1", "0"};
        std::map<melon::base::end_point, int> configed_weight;
        uint64_t configed_weight_sum = 0;
        melon::rpc::policy::WeightedRandomizedLoadBalancer wrlb;
        size_t valid_weight_num = 4;

        // Add server to selected list. The server with invalid weight will be skipped.
        for (size_t i = 0; i < MELON_ARRAY_SIZE(servers); ++i) {
            const char *addr = servers[i];
            melon::base::end_point dummy;
            ASSERT_EQ(0, str2endpoint(addr, &dummy));
            melon::rpc::ServerId id(8888);
            melon::rpc::SocketOptions options;
            options.remote_side = dummy;
            options.user = new SaveRecycle;
            ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
            id.tag = weight[i];
            if (i < valid_weight_num) {
                int weight_num;
                ASSERT_TRUE(melon::simple_atoi(weight[i], &weight_num));
                configed_weight[dummy] = weight_num;
                configed_weight_sum += weight_num;
                EXPECT_TRUE(wrlb.AddServer(id));
            } else {
                EXPECT_FALSE(wrlb.AddServer(id));
            }
        }

        // Select the best server according to weight configured.
        // There are 4 valid servers with weight 3, 2, 5 and 10 respectively.
        // We run SelectServer for multiple times. The result number of each server seleted should be
        // weight randomized with weight configured.
        std::map < melon::base::end_point, size_t > select_result;
        melon::rpc::SocketUniquePtr ptr;
        melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
        melon::rpc::LoadBalancer::SelectOut out(&ptr);
        int run_times = configed_weight_sum * 10;
        std::vector<melon::base::end_point> select_servers;
        for (int i = 0; i < run_times; ++i) {
            EXPECT_EQ(0, wrlb.SelectServer(in, &out));
            select_servers.emplace_back(ptr->remote_side());
            ++select_result[ptr->remote_side()];
        }

        for (const auto &server : select_servers) {
            std::cout << "weight randomized=" << server << ", ";
        }
        std::cout << std::endl;

        // Check whether selected result is weight with expected.
        EXPECT_EQ(valid_weight_num, select_result.size());
        std::cout << "configed_weight_sum=" << configed_weight_sum << " run_times=" << run_times << std::endl;
        for (const auto &result : select_result) {
            double actual_rate = result.second * 1.0 / run_times;
            double expect_rate = configed_weight[result.first] * 1.0 / configed_weight_sum;
            std::cout << result.first << " weight=" << configed_weight[result.first]
                      << " select_times=" << result.second
                      << " actual_rate=" << actual_rate << " expect_rate=" << expect_rate
                      << " expect_rate/2=" << expect_rate / 2 << " expect_rate*2=" << expect_rate * 2
                      << std::endl;
            // actual_rate >= expect_rate / 2
            ASSERT_GE(actual_rate, expect_rate / 2);
            // actual_rate <= expect_rate * 2
            ASSERT_LE(actual_rate, expect_rate * 2);
        }
    }

    TEST_F(LoadBalancerTest, health_check_no_valid_server) {
        const char *servers[] = {
                "10.92.115.19:8832",
                "10.42.122.201:8833",
        };
        std::vector<melon::rpc::LoadBalancer *> lbs;
        lbs.push_back(new melon::rpc::policy::RoundRobinLoadBalancer);
        lbs.push_back(new melon::rpc::policy::RandomizedLoadBalancer);
        lbs.push_back(new melon::rpc::policy::WeightedRoundRobinLoadBalancer);

        for (int i = 0; i < (int) lbs.size(); ++i) {
            melon::rpc::LoadBalancer *lb = lbs[i];
            std::vector<melon::rpc::ServerId> ids;
            for (size_t i = 0; i < MELON_ARRAY_SIZE(servers); ++i) {
                melon::base::end_point dummy;
                ASSERT_EQ(0, str2endpoint(servers[i], &dummy));
                melon::rpc::ServerId id(8888);
                melon::rpc::SocketOptions options;
                options.remote_side = dummy;
                ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
                id.tag = "50";
                ids.push_back(id);
                lb->AddServer(id);
            }

            // Without setting anything, the lb should work fine
            for (int i = 0; i < 4; ++i) {
                melon::rpc::SocketUniquePtr ptr;
                melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
                melon::rpc::LoadBalancer::SelectOut out(&ptr);
                ASSERT_EQ(0, lb->SelectServer(in, &out));
            }

            melon::rpc::SocketUniquePtr ptr;
            ASSERT_EQ(0, melon::rpc::Socket::Address(ids[0].id, &ptr));
            ptr->_ninflight_app_health_check.store(1, std::memory_order_relaxed);
            for (int i = 0; i < 4; ++i) {
                melon::rpc::SocketUniquePtr ptr;
                melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
                melon::rpc::LoadBalancer::SelectOut out(&ptr);
                ASSERT_EQ(0, lb->SelectServer(in, &out));
                // After putting server[0] into health check state, the only choice is servers[1]
                ASSERT_EQ(ptr->remote_side().port, 8833);
            }

            ASSERT_EQ(0, melon::rpc::Socket::Address(ids[1].id, &ptr));
            ptr->_ninflight_app_health_check.store(1, std::memory_order_relaxed);
            for (int i = 0; i < 4; ++i) {
                melon::rpc::SocketUniquePtr ptr;
                melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
                melon::rpc::LoadBalancer::SelectOut out(&ptr);
                // There is no server available
                ASSERT_EQ(EHOSTDOWN, lb->SelectServer(in, &out));
            }

            ASSERT_EQ(0, melon::rpc::Socket::Address(ids[0].id, &ptr));
            ptr->_ninflight_app_health_check.store(0, std::memory_order_relaxed);
            ASSERT_EQ(0, melon::rpc::Socket::Address(ids[1].id, &ptr));
            ptr->_ninflight_app_health_check.store(0, std::memory_order_relaxed);
            // After reset health check state, the lb should work fine
            bool get_server1 = false;
            bool get_server2 = false;
            for (int i = 0; i < 20; ++i) {
                melon::rpc::SocketUniquePtr ptr;
                melon::rpc::LoadBalancer::SelectIn in = {0, false, false, 0u, NULL};
                melon::rpc::LoadBalancer::SelectOut out(&ptr);
                ASSERT_EQ(0, lb->SelectServer(in, &out));
                if (ptr->remote_side().port == 8832) {
                    get_server1 = true;
                } else {
                    get_server2 = true;
                }
            }
            ASSERT_TRUE(get_server1 && get_server2);
            delete lb;
        }
    }

    TEST_F(LoadBalancerTest, revived_from_all_failed_sanity) {
        const char *servers[] = {
                "10.92.115.19:8832",
                "10.42.122.201:8833",
        };
        melon::rpc::LoadBalancer *lb = NULL;
        int rand = melon::base::fast_rand_less_than(2);
        if (rand == 0) {
            melon::rpc::policy::RandomizedLoadBalancer rlb;
            lb = rlb.New("min_working_instances=2 hold_seconds=2");
        } else if (rand == 1) {
            melon::rpc::policy::RoundRobinLoadBalancer rrlb;
            lb = rrlb.New("min_working_instances=2 hold_seconds=2");
        }
        melon::rpc::SocketUniquePtr ptr[2];
        for (size_t i = 0; i < MELON_ARRAY_SIZE(servers); ++i) {
            melon::base::end_point dummy;
            ASSERT_EQ(0, str2endpoint(servers[i], &dummy));
            melon::rpc::SocketOptions options;
            options.remote_side = dummy;
            melon::rpc::ServerId id(8888);
            id.tag = "50";
            ASSERT_EQ(0, melon::rpc::Socket::Create(options, &id.id));
            ASSERT_EQ(0, melon::rpc::Socket::Address(id.id, &ptr[i]));
            lb->AddServer(id);
        }
        melon::rpc::SocketUniquePtr sptr;
        melon::rpc::LoadBalancer::SelectIn in = {0, false, true, 0u, NULL};
        melon::rpc::LoadBalancer::SelectOut out(&sptr);
        ASSERT_EQ(0, lb->SelectServer(in, &out));

        ptr[0]->SetFailed();
        ptr[1]->SetFailed();
        ASSERT_EQ(EHOSTDOWN, lb->SelectServer(in, &out));
        // should reject all request since there is no available server
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQ(melon::rpc::EREJECT, lb->SelectServer(in, &out));
        }
        {
            melon::rpc::SocketUniquePtr dummy_ptr;
            ASSERT_EQ(1, melon::rpc::Socket::AddressFailedAsWell(ptr[0]->id(), &dummy_ptr));
            dummy_ptr->Revive();
        }
        melon::fiber_sleep_for(melon::rpc::FLAGS_detect_available_server_interval_ms * 1000);
        // After one server is revived, the reject rate should be 50%
        int num_ereject = 0;
        int num_ok = 0;
        for (int i = 0; i < 100; ++i) {
            int rc = lb->SelectServer(in, &out);
            if (rc == melon::rpc::EREJECT) {
                num_ereject++;
            } else if (rc == 0) {
                num_ok++;
            } else {
                ASSERT_TRUE(false);
            }
        }
        ASSERT_TRUE(abs(num_ereject - num_ok) < 30);
        melon::fiber_sleep_for((2000 /* hold_seconds */ + 10) * 1000);

        // After enough waiting time, traffic should be sent to all available servers.
        for (int i = 0; i < 10; ++i) {
            ASSERT_EQ(0, lb->SelectServer(in, &out));
        }
    }

    class EchoServiceImpl : public test::EchoService {
    public:
        EchoServiceImpl()
                : _num_request(0) {}

        virtual ~EchoServiceImpl() {}

        virtual void Echo(google::protobuf::RpcController *cntl_base,
                          const test::EchoRequest *req,
                          test::EchoResponse *res,
                          google::protobuf::Closure *done) {
            //melon::rpc::Controller* cntl =
            //        static_cast<melon::rpc::Controller*>(cntl_base);
            melon::rpc::ClosureGuard done_guard(done);
            int p = _num_request.fetch_add(1, std::memory_order_relaxed);
            // concurrency in normal case is 50
            if (p < 70) {
                melon::fiber_sleep_for(100 * 1000);
                _num_request.fetch_sub(1, std::memory_order_relaxed);
                res->set_message("OK");
            } else {
                _num_request.fetch_sub(1, std::memory_order_relaxed);
                melon::fiber_sleep_for(1000 * 1000);
            }
            return;
        }

        std::atomic<int> _num_request;
    };

    std::atomic<int32_t> num_failed(0);
    std::atomic<int32_t> num_reject(0);

    class Done : public google::protobuf::Closure {
    public:
        void Run() {
            if (cntl.Failed()) {
                num_failed.fetch_add(1, std::memory_order_relaxed);
                if (cntl.ErrorCode() == melon::rpc::EREJECT) {
                    num_reject.fetch_add(1, std::memory_order_relaxed);
                }
            }
            delete this;
        }

        melon::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
    };

    TEST_F(LoadBalancerTest, invalid_lb_params) {
        const char *lb_algo[] = {"random:mi_working_instances=2 hold_seconds=2",
                                 "rr:min_working_instances=2 hold_secon=2"};
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "http";
        ASSERT_EQ(channel.Init("list://127.0.0.1:7777 50, 127.0.0.1:7778 50",
                               lb_algo[melon::base::fast_rand_less_than(MELON_ARRAY_SIZE(lb_algo))],
                               &options), -1);
    }

    TEST_F(LoadBalancerTest, revived_from_all_failed_intergrated) {
        google::SetCommandLineOption("circuit_breaker_short_window_size", "20");
        google::SetCommandLineOption("circuit_breaker_short_window_error_percent", "30");
        // Those two lines force the interval of first hc to 3s
        google::SetCommandLineOption("circuit_breaker_max_isolation_duration_ms", "3000");
        google::SetCommandLineOption("circuit_breaker_min_isolation_duration_ms", "3000");

        const char *lb_algo[] = {"random:min_working_instances=2 hold_seconds=2",
                                 "rr:min_working_instances=2 hold_seconds=2"};
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "http";
        options.timeout_ms = 300;
        options.enable_circuit_breaker = true;
        // Disable retry to make health check happen one by one
        options.max_retry = 0;
        ASSERT_EQ(channel.Init("list://127.0.0.1:7777 50, 127.0.0.1:7778 50",
                               lb_algo[melon::base::fast_rand_less_than(MELON_ARRAY_SIZE(lb_algo))],
                               &options), 0);
        test::EchoRequest req;
        req.set_message("123");
        test::EchoResponse res;
        test::EchoService_Stub stub(&channel);
        {
            // trigger one server to health check
            melon::rpc::Controller cntl;
            stub.Echo(&cntl, &req, &res, NULL);
        }
        // This sleep make one server revived 700ms earlier than the other server, which
        // can make the server down again if no request limit policy are applied here.
        melon::fiber_sleep_for(700000);
        {
            // trigger the other server to health check
            melon::rpc::Controller cntl;
            stub.Echo(&cntl, &req, &res, NULL);
        }

        melon::base::end_point point(melon::base::IP_ANY, 7777);
        melon::rpc::Server server;
        EchoServiceImpl service;
        ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        ASSERT_EQ(0, server.Start(point, NULL));

        melon::base::end_point point2(melon::base::IP_ANY, 7778);
        melon::rpc::Server server2;
        EchoServiceImpl service2;
        ASSERT_EQ(0, server2.AddService(&service2, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        ASSERT_EQ(0, server2.Start(point2, NULL));

        int64_t start_ms = melon::time_now().to_unix_millis();
        while ((melon::time_now().to_unix_millis() - start_ms) < 3500) {
            Done *done = new Done;
            done->req.set_message("123");
            stub.Echo(&done->cntl, &done->req, &done->res, done);
            melon::fiber_sleep_for(1000);
        }
        // All error code should be equal to EREJECT, except when the situation
        // all servers are down, the very first call that trigger recovering would
        // fail with EHOSTDOWN instead of EREJECT. This is where the number 1 comes
        // in following ASSERT.
        ASSERT_TRUE(num_failed.load(std::memory_order_relaxed) -
                    num_reject.load(std::memory_order_relaxed) == 1);
        num_failed.store(0, std::memory_order_relaxed);

        // should recover now
        for (int i = 0; i < 1000; ++i) {
            Done *done = new Done;
            done->req.set_message("123");
            stub.Echo(&done->cntl, &done->req, &done->res, done);
            melon::fiber_sleep_for(1000);
        }
        melon::fiber_sleep_for(500000 /* sleep longer than timeout of channel */);
        ASSERT_EQ(0, num_failed.load(std::memory_order_relaxed));
    }

} //namespace
