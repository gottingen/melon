\


#include "abel/fiber/internal/run_queue.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "abel/thread/latch.h"
#include "abel/base/random.h"

using namespace std::literals;

namespace abel {
    namespace fiber_internal {

        fiber_entity *CreateEntity(int x) { return reinterpret_cast<fiber_entity *>(x); }

        TEST(run_queue, Basics) {
            run_queue queue(32);
            ASSERT_TRUE(queue.push(CreateEntity(3), false));
            ASSERT_FALSE(queue.unsafe_empty());
            ASSERT_EQ(CreateEntity(3), queue.pop());
        }

        TEST(run_queue, Steal) {
            run_queue queue(32);
            ASSERT_TRUE(queue.push(CreateEntity(3), false));
            ASSERT_FALSE(queue.unsafe_empty());
            ASSERT_EQ(CreateEntity(3), queue.steal());
        }

        TEST(run_queue, Nonstealable) {
            run_queue queue(32);
            ASSERT_TRUE(queue.push(CreateEntity(3), true));
            ASSERT_FALSE(queue.unsafe_empty());
            ASSERT_FALSE(queue.steal());
            ASSERT_EQ(CreateEntity(3), queue.pop());
        }

        TEST(run_queue, Torture) {
            constexpr auto N = 1'000'000;

            run_queue queue(1048576);

            // Loop for several rounds so that we can test the case when queue's internal
            // ring buffer wraps around.
            for (int k = 0; k != 10; ++k) {
                constexpr auto T = 200;
                std::thread ts[T];
                abel::latch lh(T);
                std::mutex lock;
                std::vector<fiber_entity *> rcs;
                std::atomic<std::size_t> read = 0;
                static_assert(N % T == 0 && T % 2 == 0);
                for (int i = 0; i != T / 2; ++i) {
                    ts[i] = std::thread([s = N / (T / 2) * i, &queue, &lh] {
                        auto as_batch = Random() % 2 == 0;
                        if (as_batch) {
                            std::vector<fiber_entity *> fbs;
                            for (int i = 0; i != N / (T / 2); ++i) {
                                fbs.push_back(CreateEntity(s + i + 1));
                            }
                            lh.count_down();
                            lh.wait();
                            for (auto iter = fbs.begin(); iter < fbs.end();) {
                                auto size = std::min<std::size_t>(200, fbs.end() - iter);
                                ASSERT_TRUE(queue.batch_push(&*iter, &*(iter + size), false));
                                iter += size;
                            }
                        } else {
                            lh.count_down();
                            lh.wait();
                            for (int i = 0; i != N / (T / 2); ++i) {
                                ASSERT_TRUE(queue.push(CreateEntity(s + i + 1), false));
                            }
                        }
                    });
                }
                for (int i = 0; i != T / 2; ++i) {
                    ts[i + T / 2] = std::thread([&] {
                        std::vector<fiber_entity *> vfes;
                        lh.count_down();
                        lh.wait();
                        while (read != N) {
                            if (auto rc = queue.pop()) {
                                vfes.push_back(rc);
                                ++read;
                            }
                        }
                        std::scoped_lock lk(lock);
                        for (auto &&e : vfes) {
                            rcs.push_back(e);
                        }
                    });
                }
                for (auto &&e : ts) {
                    e.join();
                }
                std::sort(rcs.begin(), rcs.end());
                ASSERT_EQ(rcs.end(), std::unique(rcs.begin(), rcs.end()));
                ASSERT_EQ(N, rcs.size());
                ASSERT_EQ(rcs.front(), CreateEntity(1));
                ASSERT_EQ(rcs.back(), CreateEntity(N));
            }
        }

        TEST(run_queue, Overrun) {
            constexpr auto T = 40;
            constexpr auto N = 100'000;

            // Loop for several rounds so that we can test the case when queue's internal
            // ring buffer wraps around.
            for (int k = 0; k != 10; ++k) {
                run_queue queue(8192);
                std::atomic<std::size_t> overruns{};
                std::atomic<std::size_t> popped{};

                std::thread ts[T], ts2[T];

                for (int i = 0; i != T; ++i) {
                    ts[i] = std::thread([&overruns, &queue] {
                        auto as_batch = Random() % 2 == 0;
                        if (as_batch) {
                            static_assert(N % 100 == 0);
                            constexpr auto B = N / 100;
                            std::vector<fiber_entity *> batch(B, CreateEntity(1));
                            for (int j = 0; j != N; j += B) {
                                while (!queue.batch_push(&batch[0], &batch[B], false)) {
                                    ++overruns;
                                }
                            }
                        } else {
                            for (int j = 0; j != N; ++j) {
                                while (!queue.push(CreateEntity(1), false)) {
                                    ++overruns;
                                }
                            }
                        }
                    });
                }

                for (int i = 0; i != T; ++i) {
                    ts2[i] = std::thread([&] {
                        std::this_thread::sleep_for(1s);  // Let the queue overrun.
                        while (popped.load(std::memory_order_relaxed) != N * T) {
                            if (queue.pop()) {
                                ++popped;
                            } else {
                                std::this_thread::sleep_for(1us);
                            }
                        }
                    });
                }
                for (auto &&e : ts) {
                    e.join();
                }
                for (auto &&e : ts2) {
                    e.join();
                }
                std::cout << "Overruns: " << overruns.load() << std::endl;
                ASSERT_GT(overruns.load(), 0);
                ASSERT_EQ(N * T, popped.load());
            }
        }

        TEST(run_queue, Throughput) {
            constexpr auto N = 1'000'000;

            run_queue queue(1048576);

            // Loop for several rounds so that we can test the case when queue's internal
            // ring buffer wraps around.
            for (int k = 0; k != 10; ++k) {
                constexpr auto T = 200;
                std::thread ts[T];
                abel::latch latch1(T), latch2(T);
                std::mutex lock;
                std::vector<fiber_entity *> rcs;
                static_assert(N % T == 0);

                // Batch produce.
                for (int i = 0; i != T; ++i) {
                    ts[i] = std::thread([s = N / T * i, &queue, &latch1] {
                        auto as_batch = Random() % 2 == 0;
                        if (as_batch) {
                            std::vector<fiber_entity *> fbs;
                            for (int i = 0; i != N / T; ++i) {
                                fbs.push_back(CreateEntity(s + i + 1));
                            }
                            latch1.count_down();
                            latch1.wait();
                            for (auto iter = fbs.begin(); iter < fbs.end();) {
                                auto size = std::min<std::size_t>(200, fbs.end() - iter);
                                ASSERT_TRUE(queue.batch_push(&*iter, &*(iter + size), false));
                                iter += size;
                            }
                        } else {
                            latch1.count_down();
                            latch1.wait();
                            for (int i = 0; i != N / T; ++i) {
                                ASSERT_TRUE(queue.push(CreateEntity(s + i + 1), false));
                            }
                        }
                    });
                }
                for (auto &&e : ts) {
                    e.join();
                }

                // Batch consume.
                for (int i = 0; i != T; ++i) {
                    ts[i] = std::thread([&] {
                        std::vector<fiber_entity *> vfes;
                        latch2.count_down();
                        latch2.wait();
                        for (int j = 0; j != N / T; ++j) {
                            auto rc = queue.pop();
                            vfes.push_back(rc);
                        }
                        std::scoped_lock lk(lock);
                        for (auto &&e : vfes) {
                            rcs.push_back(e);
                        }
                    });
                }
                for (auto &&e : ts) {
                    e.join();
                }
                std::sort(rcs.begin(), rcs.end());
                ASSERT_EQ(rcs.end(), std::unique(rcs.begin(), rcs.end()));
                ASSERT_EQ(N, rcs.size());
                ASSERT_EQ(rcs.front(), CreateEntity(1));
                ASSERT_EQ(rcs.back(), CreateEntity(N));
            }
        }
    } // namespace fiber_internal
}  // namespace abel
