//
// Created by liyinbin on 2021/4/3.
//


#include "abel/future/future.h"

#include <condition_variable>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <type_traits>
#include "gtest/gtest.h"

using namespace std::literals;

namespace abel {

    using MoveOnlyType = std::unique_ptr<int>;

    static_assert(!std::is_copy_constructible_v<future<>>);
    static_assert(!std::is_copy_assignable_v<future<>>);
    static_assert(!std::is_copy_constructible_v<future<int, double>>);
    static_assert(!std::is_copy_assignable_v<future<int, double>>);
    static_assert(std::is_move_constructible_v<future<>>);
    static_assert(std::is_move_assignable_v<future<>>);
    static_assert(std::is_move_constructible_v<future<int, double>>);
    static_assert(std::is_move_assignable_v<future<int, double>>);

    static_assert(!std::is_copy_constructible_v<promise<>>);
    static_assert(!std::is_copy_assignable_v<promise<>>);
    static_assert(!std::is_copy_constructible_v<promise<int, double>>);
    static_assert(!std::is_copy_assignable_v<promise<int, double>>);
    static_assert(std::is_move_constructible_v<promise<>>);
    static_assert(std::is_move_assignable_v<promise<>>);
    static_assert(std::is_move_constructible_v<promise<int, double>>);
    static_assert(std::is_move_assignable_v<promise<int, double>>);

    static_assert(!std::is_copy_constructible_v<abel::future_internal::boxed<MoveOnlyType>>);
    static_assert(!std::is_copy_assignable_v<abel::future_internal::boxed<MoveOnlyType>>);
    static_assert(std::is_move_constructible_v<abel::future_internal::boxed<MoveOnlyType>>);
    static_assert(std::is_move_assignable_v<abel::future_internal::boxed<MoveOnlyType>>);

    template<class T>
    using resource_ptr = std::unique_ptr<T, void (*)(T *)>;

    future<resource_ptr<void>, int> AcquireXxxAsync() {
        promise<resource_ptr<void>, int> p;
        auto rf = p.get_future();

        std::thread([p = std::move(p)]() mutable {
            std::this_thread::sleep_for(10ms);
            p.set_value(resource_ptr<void>(reinterpret_cast<void *>(1), [](auto) {}), 0);
        }).detach();

        return rf;
    }

// Not tests, indeed. (Or it might be treated as a compilation test?)
    TEST(Usage, Initiialization) {
        future<> uf1;                 // Uninitialized future.
        future<int, double> uf2;      // Uninitialized future.
        future<> f(futurize_values);  // Ready future.
        future<int> fi(10);  // future with single type can be constructed directly.
        future<int, double> fid(futurize_values, 1, 2.0);
        future<double, float> f2{std::move(fid)};
        auto df = future(futurize_values, 1, 2);  // Using deduction guides.
        auto vf = make_ready_future();
        auto mf = make_ready_future(1, 2.0);  // future<int, double>

        ASSERT_EQ(10, blocking_get(std::move(fi)));
        // Passing pointer to `future` to `blocking_get` also works.
        ASSERT_EQ(2.0, std::get<1>(blocking_get(&f2)));
        ASSERT_EQ(2.0, std::get<1>(blocking_get(&mf)));
    }

    TEST(Usage, Continuation) {
        future<int, std::uint64_t> f(futurize_values, 1, 2);
        bool cont_called = false;

        // Values in `future` are passed separately to the continuation.
        std::move(f).then([&](int x, double f) {
            ASSERT_EQ(1, x);
            ASSERT_EQ(2.0, f);
            cont_called = true;
        });

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationVariadic) {
        bool cont_called = false;

        // Generic lambda are also allowed.
        future(futurize_values, 1, 2.0).then([&](auto &&... values) {
            ASSERT_EQ(3, (... + values));
            cont_called = true;
        });

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationAsyncFile) {
        future<FILE *, int> failure_file(futurize_values, nullptr, -1);
        bool cont_called = false;

        std::move(failure_file).then([&](FILE *fp, int ec) {  // `auto` also works.
            ASSERT_EQ(nullptr, fp);
            ASSERT_EQ(-1, ec);

            cont_called = true;
        });

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationWhenAllVariadic) {
        static FILE *const kDummyFile = reinterpret_cast<FILE *>(0x10000123);

        future<FILE *, int> async_file(futurize_values, kDummyFile, 0);
        future<FILE *, int> failure_file(futurize_values, nullptr, -1);
        future<resource_ptr<FILE>, int> move_only_file{
                futurize_values, resource_ptr<FILE>{nullptr, nullptr}, -2};
        future<> void_op(futurize_values);
        bool cont_called = false;

        // Or `when_all(&async_file, &failure_file, &void_op, &move_only_file)`.
        when_all(std::move(async_file), std::move(failure_file), std::move(void_op),
                std::move(move_only_file))
                .then([&](std::tuple<FILE *, int> af, std::tuple<FILE *, int> ff,
                          std::tuple<resource_ptr<FILE>, int> mof) {
                    auto&&[fp1, ec1] = af;
                    auto&&[fp2, ec2] = ff;
                    auto&&[fp3, ec3] = mof;

                    ASSERT_EQ(kDummyFile, fp1);
                    ASSERT_EQ(0, ec1);
                    ASSERT_EQ(nullptr, fp2);
                    ASSERT_EQ(-1, ec2);
                    ASSERT_EQ(nullptr, fp3);
                    ASSERT_EQ(-2, ec3);

                    cont_called = true;
                });

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationWhenAllVariadicOnRvalueRefs) {
        bool cont_called = false;

        blocking_get(when_all(AcquireXxxAsync(), AcquireXxxAsync())
                            .then([&](std::tuple<resource_ptr<void>, int> a,
                                      std::tuple<resource_ptr<void>, int> b) {
                                auto&&[a1, a2] = a;
                                auto&&[b1, b2] = b;

                                ASSERT_NE(nullptr, a1);
                                ASSERT_EQ(0, a2);
                                ASSERT_NE(nullptr, b1);
                                ASSERT_EQ(0, b2);

                                cont_called = true;
                            }));

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationWhenAllCollectionOfEmptyFuture) {
        std::vector<future<>> vfs;
        bool cont_called = false;

        for (int i = 0; i != 1000; ++i) {
            vfs.emplace_back(futurize_values);
        }

// std::vector<future<>> is special, the continuation is called with
// no argument.
        when_all(std::move(vfs)).then([&] { cont_called = true; });
        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationWhenAnyCollectionOfEmptyFuture) {
        std::vector<future<>> vfs;
        bool cont_called = false;

        for (int i = 0; i != 1000; ++i) {
            vfs.emplace_back(futurize_values);
        }

        when_any(&vfs).then([&](std::size_t index) { cont_called = true; });
        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationWhenAnyCollection) {
        std::vector<future<int>> vfs;
        bool cont_called = false;

        for (int i = 0; i != 1000; ++i) {
            vfs.emplace_back(i);
        }

        when_any(&vfs).then([&](std::size_t index, int v) { cont_called = true; });
        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ContinuationWhenAllCollection) {
        std::vector<future<int>> vfs;
        bool cont_called = false;

        for (int i = 0; i != 1000; ++i) {
            vfs.emplace_back(1);
        }

// Or `when_all(&vfs)`.
        when_all(std::move(vfs)).then([&](std::vector<int> v) {
            ASSERT_EQ(1000, std::accumulate(v.begin(), v.end(), 0));
            cont_called = true;
        });
        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, Fork) {
        future<int> rf(1);
        auto forked = abel::future_internal::fork(&rf);  // (Will be) satisfied with the same value as
        // of `rf`.
        bool cont_called = false;

        when_all(&rf, &forked).then([&](int x, int y) {
            ASSERT_EQ(1, x);
            ASSERT_EQ(1, y);

            cont_called = true;
        });

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, ForkVoid) {
        auto rf = make_ready_future();
        auto forked = abel::future_internal::fork(&rf);
        bool cont_called = false;

        when_all(&rf, &forked).then([&] { cont_called = true; });

        ASSERT_TRUE(cont_called);
    }

    TEST(Usage, Split) {
        {
            auto&&[f1, f2] = split(future < int > (1));
            bool cont_called = false;

            when_all(&f1, &f2).then([&](int x, int y) {
                ASSERT_EQ(1, x);
                ASSERT_EQ(1, y);

                cont_called = true;
            });

            ASSERT_TRUE(cont_called);
        }
        {
            auto&&[f1, f2] = split(make_ready_future());
            bool cont_called = false;

            when_all(&f1, &f2).then([&] { cont_called = true; });
            ASSERT_TRUE(cont_called);
        }
    }

    struct Latch {
        void Countdown() {
            std::unique_lock lk(m);

            if (--left) {
                cv.wait(lk, [&] { return left == 0; });
            } else {
                cv.notify_all();
            }
        }

        explicit Latch(std::size_t size) : left(size) {}

        std::mutex m;
        std::size_t left;
        std::condition_variable cv;
    };

    struct NonDefaultConstructible {
        explicit NonDefaultConstructible(int x) {}

        NonDefaultConstructible() = delete;
    };

    TEST(FutureV2Test, ReadyFuture) {
        int x = 0;
        auto ready = future(10);

        ASSERT_EQ(0, x);
        std::move(ready).then([&](auto xx) { x = xx; });
        ASSERT_EQ(10, x);
    }

    TEST(FutureV2Test, ConversionTest) {
        future<int> f(1);
        future < std::uint64_t > f2 = std::move(f);

        ASSERT_EQ(1, blocking_get(std::move(f2)));
    }

// Primarily a compilation test.
    TEST(FutureV2Test, NonDefaultConstructibleTypes) {
        promise<NonDefaultConstructible> p;

        p.set_value(10);
    }

    TEST(FutureV2Test, MoveOnlyWhenAllVariadic) {
        bool f = false;
        promise<std::unique_ptr<int>, std::unique_ptr<char>> p1;
        promise<> p2;

        when_all(p1.get_future(), p2.get_future()).then([&](auto p) {
            auto&&[pi, pc] = p;
            ASSERT_NE(nullptr, pi);
            ASSERT_EQ(nullptr, pc);

            f = true;
        });

        p1.set_value(std::make_unique<int>(), std::unique_ptr<char>{});
        ASSERT_FALSE(f);
        p2.set_value();
        ASSERT_TRUE(f);
    }

    TEST(FutureV2Test, MoveOnlyWhenAllCollection) {
        constexpr auto kCount = 10000;
        std::vector<promise<std::unique_ptr<int>, NonDefaultConstructible>> vps(
                kCount);
        std::vector<future<>> vfs;
        int x = 0;

        for (auto &&e : vps) {
            vfs.emplace_back(e.get_future().then([&](auto &&...) { ++x; }));
        }

        auto rc = when_all(std::move(vfs));
        ASSERT_EQ(0, x);

        for (auto &&e : vps) {
            e.set_value(std::make_unique<int>(), 10);
        }

        ASSERT_EQ(kCount, x);
        blocking_get(std::move(rc));  // Not needed, though.
        ASSERT_EQ(kCount, x);
    }

    TEST(FutureV2Test, MoveOnlyBlockingGet) {
        for (int i = 0; i != 10000; ++i) {
            std::atomic<bool> f = false;
            promise<std::unique_ptr<int>, std::unique_ptr<char>> p1;

            // Counterexample actually..
            //
            // The future is satisfied once `p1.set_value` is called, potentially
            // before `f` is set to true.
            /*
            std::thread([&] () { p1.set_value(); f = true;}).detach();
            blocking_get(p1.get_future());
            */

            std::thread([&] { p1.set_value(); }).detach();
            blocking_get(p1.get_future().then([&](auto &&...) { f = true; }));
            ASSERT_TRUE(f);
        }
    }

    TEST(FutureV2Test, CompatibleConversion) {
        future<int> f(10);
        future < std::uint64_t > f2 = std::move(f);

        ASSERT_EQ(10, blocking_get(std::move(f2)));
    }

    TEST(FutureV2Test, WhenAllCollectionMultithreaded) {
        for (int i = 0; i != 100; ++i) {
            constexpr auto kCount = 100;
            std::vector<promise<std::unique_ptr<int>, char>> vps(kCount);
            std::vector<future<>> vfs;
            std::vector<std::thread> ts;
            Latch latch(kCount + 1);
            std::atomic<int> x{};

            for (auto &&e : vps) {
                vfs.emplace_back(e.get_future().then([&](auto &&...) { ++x; }));
            }

            auto all = when_all(std::move(vfs));
            ASSERT_EQ(0, x);

            for (auto &&e : vps) {
                ts.push_back(std::thread([&, ep = &e] {
                    latch.Countdown();
                    ep->set_value(std::make_unique<int>(), 'a');
                }));
            }
            ASSERT_EQ(0, x);

            std::thread([&] {
                std::this_thread::sleep_for(10ms);
                latch.Countdown();
            }).detach();
            blocking_get(std::move(all));

            ASSERT_EQ(kCount, x);

// So that `lock` won't be destroyed before the threads exit.
            for (auto &&e : ts) {
                e.join();
            }
        }
    }

// std::vector<bool> is special in that accessing individual members
// might race between each other.
//
// > Notwithstanding (17.6.5.9), implementations are required to avoid
// > data races when the contents of the contained object in different
// > elements in the same sequence, excepting vector<bool>, are modified
// > concurrently.
    TEST(FutureV2Test, WhenAllCollectionMultithreadedBool) {
        for (int i = 0; i != 1000; ++i) {
            constexpr auto kCount = 100;

            std::vector<promise<bool>> vps(kCount);
            std::vector<future<bool>> vfs;
            std::vector<std::thread> ts(kCount);
            Latch latch(kCount + 1);
            std::atomic<bool> cont_called = false;

            for (int i = 0; i != kCount; ++i) {
                vfs.emplace_back(vps[i].get_future());
            }

            when_all(&vfs).then([&](std::vector<bool> v) {
                ASSERT_TRUE(std::all_of(v.begin(), v.end(), [&](auto x) { return x; }));
                cont_called = true;
            });

            for (int i = 0; i != kCount; ++i) {
                ts[i] = std::thread([&latch, &vps, i] {
                    latch.Countdown();
                    vps[i].set_value(true);
                });
            }

            ASSERT_FALSE(cont_called);
            latch.Countdown();
            for (auto &&e : ts) {
                e.join();
            }
            ASSERT_TRUE(cont_called);
        }
    }

    TEST(FutureV2Test, WhenAnyCollectionMultithreaded) {
        for (int i = 0; i != 100; ++i) {
            constexpr auto kCount = 100;
            std::vector<promise<std::unique_ptr<int>, char>> vps(kCount);
            std::vector<future<char>> vfs;
            std::vector<std::thread> ts;
            Latch latch(kCount + 1);
            std::atomic<int> x{};

            for (auto &&e : vps) {
                vfs.emplace_back(e.get_future().then([&](auto &&...) {
                    ++x;
                    return 'a';
                }));
            }

            auto all = when_any(std::move(vfs));
            ASSERT_EQ(0, x);

            for (auto &&e : vps) {
                ts.push_back(std::thread([&, ep = &e] {
                    latch.Countdown();
                    ep->set_value(std::make_unique<int>(), 'a');
                }));
            }
            ASSERT_EQ(0, x);

            std::thread([&] {
                std::this_thread::sleep_for(10ms);
                latch.Countdown();
            }).detach();

            auto&&[index, value] = blocking_get(&all);

            ASSERT_GE(kCount, index);
            ASSERT_LE(0, index);
            ASSERT_EQ('a', value);

            for (auto &&e : ts) {
                e.join();
            }
            ASSERT_EQ(kCount, x);
        }
    }

    TEST(FutureV2Test, WhenAllVariadicMultithreaded) {
        for (int i = 0; i != 10000; ++i) {
            bool f = false;
            promise<std::unique_ptr<int>, std::unique_ptr<char>> p1;
            promise<> p2;
            Latch latch(2 + 1);

            auto all = when_all(p1.get_future(), p2.get_future()).then([&](auto p) {
                auto&&[pi, pc] = p;
                ASSERT_NE(nullptr, pi);
                ASSERT_EQ(nullptr, pc);

                f = true;
            });

            auto t1 = std::thread([&] {
                latch.Countdown();
                p1.set_value(std::make_unique<int>(), std::unique_ptr<char>());
            });
            auto t2 = std::thread([&] {
                latch.Countdown();
                p2.set_value();
            });

            ASSERT_FALSE(f);
            latch.Countdown();
            blocking_get(std::move(all));
            ASSERT_TRUE(f);

            t1.join();
            t2.join();
        }
    }

    TEST(FutureV2Test, WhenAllCollectionEmpty) {
        {
            std::vector<future<>> vfs;
            int x{};
            when_all(std::move(vfs)).then([&] { x = 10; });
            ASSERT_EQ(10, x);
        }

        {
            std::vector<future<int>> vfs;
            int x{};
            when_all(std::move(vfs)).then([&](auto) { x = 10; });
            ASSERT_EQ(10, x);
        }
    }

    TEST(FutureV2Test, WhenAllOnCollectionOfEmptyFutures) {
        constexpr auto kCount = 100000;
        std::vector<future<>> vfs(kCount);

        for (int i = 0; i != kCount; ++i) {
            vfs[i] = future(futurize_values);
        }

        int x = 0;

        when_all(std::move(vfs)).then([&] { x = 100; });
        ASSERT_EQ(100, x);
    }

    TEST(FutureV2Test, Chaining) {
        constexpr auto kLoopCount = 1000;

        promise<> p;
        auto f = p.get_future();
        int c = 0;

        for (int i = 0; i != kLoopCount; ++i) {
            f = std::move(f).then([&c] { ++c; });
        }

        ASSERT_EQ(0, c);
        p.set_value();
        ASSERT_EQ(kLoopCount, c);
    }


    TEST(FutureV2Test, ConcurrentFork) {
        for (int i = 0; i != 100000; ++i) {
            promise<std::string> ps;
            auto fs = ps.get_future();
            Latch l(2);
            std::atomic<int> x{};
            auto t = std::thread([&] {
                l.Countdown();
                abel::future_internal::fork(&fs).then([&](auto &&...) { ++x; });
            });

            l.Countdown();
            ps.set_value("asdf");  // Will be concurrently executed with `Fork(&fs)`.
            t.join();

            ASSERT_EQ(1, x);
        }
    }

    TEST(FutureV2Test, DurationTimeout) {
        {
            promise<int> p;
            auto rc = blocking_try_get(p.get_future(), 1s);
            ASSERT_FALSE(rc);
            p.set_value(10);
        }
        {
            promise<int> p;
            auto f = p.get_future();
            auto rc = blocking_try_get(&f, 1s);
            ASSERT_FALSE(rc);
        }
        {
            promise<> p;
            auto f = p.get_future();
            auto rc = blocking_try_get(&f, 1s);
            ASSERT_FALSE(rc);
        }
    }

    TEST(FutureV2Test, DurationTimePoint) {
        {
            promise<int> p;
            auto rc =
                    blocking_try_get(p.get_future(), std::chrono::system_clock::now() + 1s);
            ASSERT_FALSE(rc);
        }
        {
            promise<int> p;
            auto f = p.get_future();
            auto rc = blocking_try_get(&f, std::chrono::system_clock::now() + 1s);
            ASSERT_FALSE(rc);
        }
        {
            promise<> p;
            auto f = p.get_future();
            auto rc = blocking_try_get(&f, std::chrono::system_clock::now() + 1s);
            ASSERT_FALSE(rc);
        }
    }

    TEST(FutureV2Test, Repeat) {
        int ct = 0;
        bool f = false;

        future_internal::repeat([&] { return ++ct != 100; }).then([&] { f = true; });

        ASSERT_EQ(100, ct);
        ASSERT_TRUE(f);
    }

    TEST(FutureV2Test, RepeatIfReturnsVoid) {
        std::vector<int> v;
        int ct = 0;
        bool f = false;

        future_internal::repeat_if([&] { v.push_back(++ct); }, [&] { return v.size() < 100; })
                .then([&] { f = true; });

        ASSERT_EQ(100, ct);
        ASSERT_EQ(100, v.size());
    }

    TEST(FutureV2Test, RepeatIfReturnsValue) {
        std::vector<int> v;
        int ct = 0;
        bool f = false;
        future_internal::repeat_if(
                [&] {
                    v.push_back(++ct);
                    return std::make_unique<int>(v.size());  // Move only.
                },
                [&](auto &&s) { return *s < 100; }  // Can NOT pass-by-value.
        )
                .then([&](auto s) {
                    ASSERT_EQ(100, *s);
                    f = true;
                });

        ASSERT_EQ(100, ct);
        ASSERT_EQ(100, v.size());

        for (int i = 0; i != v.size(); ++i) {
            ASSERT_EQ(i + 1, v[i]);
        }
    }

    TEST(FutureV2Test, RepeatIfReturnsMultipleValue) {
        std::vector<int> v;
        int ct = 0;

        auto&&[vv, s] = blocking_get(future_internal::repeat_if(
                [&] {
                    v.push_back(++ct);
                    return future(futurize_values, 10,
                                  std::make_unique<int>(v.size()));  // Move only.
                },
                [&](int v, auto &&s) { return *s < 100; }));  // Can NOT pass-by-value.

        ASSERT_EQ(10, vv);
        ASSERT_EQ(100, *s);
        ASSERT_EQ(100, ct);
        ASSERT_EQ(100, v.size());

        for (int i = 0; i != v.size(); ++i) {
            ASSERT_EQ(i + 1, v[i]);
        }
    }

    std::atomic<std::uint64_t> posted_jobs = 0;

    class FancyExecutor {
    public:
        void execute(abel::function<void()> job) {
            ++posted_jobs;
            std::thread([job = std::move(job)] { job(); }).detach();
        }
    };

    TEST(FutureV2Test, ExecutorTest) {
        ASSERT_EQ(0, posted_jobs);

        {
            promise<> p;
            p.get_future().then([] {});
            p.set_value();

            ASSERT_EQ(0, posted_jobs);
        }

// Now we enable executor.
        FancyExecutor fe;
        future_internal::set_default_executor(fe);
        posted_jobs = 0;

        auto test = [] {
            std::mutex m;
            std::condition_variable cv;
            std::atomic<bool> last_one = false;

            promise<> p;
            auto f = p.get_future();

            // Now we won't overflow the stack even if we make a very long chain.
            for (int i = 0; i != 10000; ++i) {
                f = std::move(f).then([&] { ASSERT_GT(posted_jobs, 0); });
            }
            p.set_value();

            std::move(f).then([&] {
                // The lock is required so that a spurious wake up of `cv` between change
                // of `last_one` and notifying `cv` won't cause `cv.wait` below to pass
                // and destroy `cv` (as a consequence of leaving the scope).
                //
                // This issue was reported by tsan as a race between `pthread_cond_notify`
                // and `pthread_cond_destory`, although I think this is a separate bug in
                // tsan as it's clearly stated that destorying a condition-variable once
                // all threads waiting on it are awakened is well-defined.
                //
                // [https://linux.die.net/man/3/pthread_cond_destroy]
                //
                // > A condition variable can be destroyed immediately after all the
                // > threads that are blocked on it are awakened.
                //
                // Nonetheless this is still a race in our code, so we lock on `m` here.
                std::lock_guard lk(m);
                last_one = true;
                cv.notify_one();
            });

            std::unique_lock lk(m);
            cv.wait(lk, [&] { return last_one.load(); });

            ASSERT_GT(posted_jobs, 0);
        };

        std::vector<std::thread> vt(10);
        for (auto &&t : vt) {
            t = std::thread(test);
        }
        for (auto &&t : vt) {
            t.join();
        }

        ASSERT_EQ(posted_jobs, 10 * 10000 + 10);

        future_internal::set_default_executor(future_internal::inline_executor());

        {
            posted_jobs = 0;

            promise<> p;
            p.get_future().then([] {});
            p.set_value();

            ASSERT_EQ(0, posted_jobs);
        }
    }

    TEST(FutureV2DeathTest, WhenAnyCollectionEmpty) {
        std::vector<future<>> vfs;
        ASSERT_DEATH(when_any(std::move(vfs)), "on an empty collection is undefined");

        std::vector<future<int>> vfs2;
        ASSERT_DEATH(when_any(&vfs2), "on an empty collection is undefined");
    }

    TEST(FutureDeathTest, DeathOnException) {
        ASSERT_DEATH(
                future(1).then([](int) { throw std::runtime_error("Fancy death"); }),
                "Fancy death");
    }
}
