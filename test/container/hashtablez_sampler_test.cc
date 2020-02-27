//

#include <abel/container/internal/hashtablez_sampler.h>

#include <atomic>
#include <limits>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/base/profile.h>
#include <abel/container/internal/have_sse.h>
#include <abel/synchronization/blocking_counter.h>
#include <abel/synchronization/internal/thread_pool.h>
#include <abel/synchronization/mutex.h>
#include <abel/synchronization/notification.h>
#include <abel/chrono/clock.h>
#include <abel/chrono/time.h>

#if SWISSTABLE_HAVE_SSE2
constexpr int kProbeLength = 16;
#else
constexpr int kProbeLength = 8;
#endif

namespace abel {

    namespace container_internal {
        class HashtablezInfoHandlePeer {
        public:
            static bool IsSampled(const HashtablezInfoHandle &h) {
                return h.info_ != nullptr;
            }

            static HashtablezInfo *GetInfo(HashtablezInfoHandle *h) { return h->info_; }
        };

        namespace {
            using ::abel::synchronization_internal::ThreadPool;
            using ::testing::IsEmpty;
            using ::testing::UnorderedElementsAre;

            std::vector<size_t> GetSizes(HashtablezSampler *s) {
                std::vector<size_t> res;
                s->Iterate([&](const HashtablezInfo &info) {
                    res.push_back(info.size.load(std::memory_order_acquire));
                });
                return res;
            }

            HashtablezInfo *Register(HashtablezSampler *s, size_t size) {
                auto *info = s->Register();
                assert(info != nullptr);
                info->size.store(size);
                return info;
            }

            TEST(HashtablezInfoTest, PrepareForSampling) {
                abel::abel_time test_start = abel::now();
                HashtablezInfo info;
                abel::mutex_lock l(&info.init_mu);
                info.PrepareForSampling();

                EXPECT_EQ(info.capacity.load(), 0);
                EXPECT_EQ(info.size.load(), 0);
                EXPECT_EQ(info.num_erases.load(), 0);
                EXPECT_EQ(info.max_probe_length.load(), 0);
                EXPECT_EQ(info.total_probe_length.load(), 0);
                EXPECT_EQ(info.hashes_bitwise_or.load(), 0);
                EXPECT_EQ(info.hashes_bitwise_and.load(), ~size_t{});
                EXPECT_GE(info.create_time, test_start);

                info.capacity.store(1, std::memory_order_relaxed);
                info.size.store(1, std::memory_order_relaxed);
                info.num_erases.store(1, std::memory_order_relaxed);
                info.max_probe_length.store(1, std::memory_order_relaxed);
                info.total_probe_length.store(1, std::memory_order_relaxed);
                info.hashes_bitwise_or.store(1, std::memory_order_relaxed);
                info.hashes_bitwise_and.store(1, std::memory_order_relaxed);
                info.create_time = test_start - abel::hours(20);

                info.PrepareForSampling();
                EXPECT_EQ(info.capacity.load(), 0);
                EXPECT_EQ(info.size.load(), 0);
                EXPECT_EQ(info.num_erases.load(), 0);
                EXPECT_EQ(info.max_probe_length.load(), 0);
                EXPECT_EQ(info.total_probe_length.load(), 0);
                EXPECT_EQ(info.hashes_bitwise_or.load(), 0);
                EXPECT_EQ(info.hashes_bitwise_and.load(), ~size_t{});
                EXPECT_GE(info.create_time, test_start);
            }

            TEST(HashtablezInfoTest, RecordStorageChanged) {
                HashtablezInfo info;
                abel::mutex_lock l(&info.init_mu);
                info.PrepareForSampling();
                RecordStorageChangedSlow(&info, 17, 47);
                EXPECT_EQ(info.size.load(), 17);
                EXPECT_EQ(info.capacity.load(), 47);
                RecordStorageChangedSlow(&info, 20, 20);
                EXPECT_EQ(info.size.load(), 20);
                EXPECT_EQ(info.capacity.load(), 20);
            }

            TEST(HashtablezInfoTest, RecordInsert) {
                HashtablezInfo info;
                abel::mutex_lock l(&info.init_mu);
                info.PrepareForSampling();
                EXPECT_EQ(info.max_probe_length.load(), 0);
                RecordInsertSlow(&info, 0x0000FF00, 6 * kProbeLength);
                EXPECT_EQ(info.max_probe_length.load(), 6);
                EXPECT_EQ(info.hashes_bitwise_and.load(), 0x0000FF00);
                EXPECT_EQ(info.hashes_bitwise_or.load(), 0x0000FF00);
                RecordInsertSlow(&info, 0x000FF000, 4 * kProbeLength);
                EXPECT_EQ(info.max_probe_length.load(), 6);
                EXPECT_EQ(info.hashes_bitwise_and.load(), 0x0000F000);
                EXPECT_EQ(info.hashes_bitwise_or.load(), 0x000FFF00);
                RecordInsertSlow(&info, 0x00FF0000, 12 * kProbeLength);
                EXPECT_EQ(info.max_probe_length.load(), 12);
                EXPECT_EQ(info.hashes_bitwise_and.load(), 0x00000000);
                EXPECT_EQ(info.hashes_bitwise_or.load(), 0x00FFFF00);
            }

            TEST(HashtablezInfoTest, RecordErase) {
                HashtablezInfo info;
                abel::mutex_lock l(&info.init_mu);
                info.PrepareForSampling();
                EXPECT_EQ(info.num_erases.load(), 0);
                EXPECT_EQ(info.size.load(), 0);
                RecordInsertSlow(&info, 0x0000FF00, 6 * kProbeLength);
                EXPECT_EQ(info.size.load(), 1);
                RecordEraseSlow(&info);
                EXPECT_EQ(info.size.load(), 0);
                EXPECT_EQ(info.num_erases.load(), 1);
            }

            TEST(HashtablezInfoTest, RecordRehash) {
                HashtablezInfo info;
                abel::mutex_lock l(&info.init_mu);
                info.PrepareForSampling();
                RecordInsertSlow(&info, 0x1, 0);
                RecordInsertSlow(&info, 0x2, kProbeLength);
                RecordInsertSlow(&info, 0x4, kProbeLength);
                RecordInsertSlow(&info, 0x8, 2 * kProbeLength);
                EXPECT_EQ(info.size.load(), 4);
                EXPECT_EQ(info.total_probe_length.load(), 4);

                RecordEraseSlow(&info);
                RecordEraseSlow(&info);
                EXPECT_EQ(info.size.load(), 2);
                EXPECT_EQ(info.total_probe_length.load(), 4);
                EXPECT_EQ(info.num_erases.load(), 2);

                RecordRehashSlow(&info, 3 * kProbeLength);
                EXPECT_EQ(info.size.load(), 2);
                EXPECT_EQ(info.total_probe_length.load(), 3);
                EXPECT_EQ(info.num_erases.load(), 0);
            }

#if ABEL_PER_THREAD_TLS == 1
            TEST(HashtablezSamplerTest, SmallSampleParameter) {
              SetHashtablezEnabled(true);
              SetHashtablezSampleParameter(100);

              for (int i = 0; i < 1000; ++i) {
                int64_t next_sample = 0;
                HashtablezInfo* sample = SampleSlow(&next_sample);
                EXPECT_GT(next_sample, 0);
                EXPECT_NE(sample, nullptr);
                UnsampleSlow(sample);
              }
            }

            TEST(HashtablezSamplerTest, LargeSampleParameter) {
              SetHashtablezEnabled(true);
              SetHashtablezSampleParameter(std::numeric_limits<int32_t>::max());

              for (int i = 0; i < 1000; ++i) {
                int64_t next_sample = 0;
                HashtablezInfo* sample = SampleSlow(&next_sample);
                EXPECT_GT(next_sample, 0);
                EXPECT_NE(sample, nullptr);
                UnsampleSlow(sample);
              }
            }

            TEST(HashtablezSamplerTest, Sample) {
              SetHashtablezEnabled(true);
              SetHashtablezSampleParameter(100);
              int64_t num_sampled = 0;
              int64_t total = 0;
              double sample_rate = 0.0;
              for (int i = 0; i < 1000000; ++i) {
                HashtablezInfoHandle h = Sample();
                ++total;
                if (HashtablezInfoHandlePeer::IsSampled(h)) {
                  ++num_sampled;
                }
                sample_rate = static_cast<double>(num_sampled) / total;
                if (0.005 < sample_rate && sample_rate < 0.015) break;
              }
              EXPECT_NEAR(sample_rate, 0.01, 0.005);
            }
#endif

            TEST(HashtablezSamplerTest, Handle) {
                auto &sampler = HashtablezSampler::Global();
                HashtablezInfoHandle h(sampler.Register());
                auto *info = HashtablezInfoHandlePeer::GetInfo(&h);
                info->hashes_bitwise_and.store(0x12345678, std::memory_order_relaxed);

                bool found = false;
                sampler.Iterate([&](const HashtablezInfo &h) {
                    if (&h == info) {
                        EXPECT_EQ(h.hashes_bitwise_and.load(), 0x12345678);
                        found = true;
                    }
                });
                EXPECT_TRUE(found);

                h = HashtablezInfoHandle();
                found = false;
                sampler.Iterate([&](const HashtablezInfo &h) {
                    if (&h == info) {
                        // this will only happen if some other thread has resurrected the info
                        // the old handle was using.
                        if (h.hashes_bitwise_and.load() == 0x12345678) {
                            found = true;
                        }
                    }
                });
                EXPECT_FALSE(found);
            }

            TEST(HashtablezSamplerTest, Registration) {
                HashtablezSampler sampler;
                auto *info1 = Register(&sampler, 1);
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(1));

                auto *info2 = Register(&sampler, 2);
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(1, 2));
                info1->size.store(3);
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(3, 2));

                sampler.Unregister(info1);
                sampler.Unregister(info2);
            }

            TEST(HashtablezSamplerTest, Unregistration) {
                HashtablezSampler sampler;
                std::vector<HashtablezInfo *> infos;
                for (size_t i = 0; i < 3; ++i) {
                    infos.push_back(Register(&sampler, i));
                }
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(0, 1, 2));

                sampler.Unregister(infos[1]);
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(0, 2));

                infos.push_back(Register(&sampler, 3));
                infos.push_back(Register(&sampler, 4));
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(0, 2, 3, 4));
                sampler.Unregister(infos[3]);
                EXPECT_THAT(GetSizes(&sampler), UnorderedElementsAre(0, 2, 4));

                sampler.Unregister(infos[0]);
                sampler.Unregister(infos[2]);
                sampler.Unregister(infos[4]);
                EXPECT_THAT(GetSizes(&sampler), IsEmpty());
            }

            TEST(HashtablezSamplerTest, MultiThreaded) {
                HashtablezSampler sampler;
                notification stop;
                ThreadPool pool(10);

                for (int i = 0; i < 10; ++i) {
                    pool.Schedule([&sampler, &stop]() {
                        std::random_device rd;
                        std::mt19937 gen(rd());

                        std::vector<HashtablezInfo *> infoz;
                        while (!stop.has_been_notified()) {
                            if (infoz.empty()) {
                                infoz.push_back(sampler.Register());
                            }
                            switch (std::uniform_int_distribution<>(0, 2)(gen)) {
                                case 0: {
                                    infoz.push_back(sampler.Register());
                                    break;
                                }
                                case 1: {
                                    size_t p =
                                            std::uniform_int_distribution<>(0, infoz.size() - 1)(gen);
                                    HashtablezInfo *info = infoz[p];
                                    infoz[p] = infoz.back();
                                    infoz.pop_back();
                                    sampler.Unregister(info);
                                    break;
                                }
                                case 2: {
                                    abel::duration oldest = abel::zero_duration();
                                    sampler.Iterate([&](const HashtablezInfo &info) {
                                        oldest = std::max(oldest, abel::now() - info.create_time);
                                    });
                                    ASSERT_GE(oldest, abel::zero_duration());
                                    break;
                                }
                            }
                        }
                    });
                }
                // The threads will hammer away.  Give it a little bit of time for tsan to
                // spot errors.
                abel::sleep_for(abel::seconds(3));
                stop.Notify();
            }

            TEST(HashtablezSamplerTest, Callback) {
                HashtablezSampler sampler;

                auto *info1 = Register(&sampler, 1);
                auto *info2 = Register(&sampler, 2);

                static const HashtablezInfo *expected;

                auto callback = [](const HashtablezInfo &info) {
                    // We can't use `info` outside of this callback because the object will be
                    // disposed as soon as we return from here.
                    EXPECT_EQ(&info, expected);
                };

                // Set the callback.
                EXPECT_EQ(sampler.SetDisposeCallback(callback), nullptr);
                expected = info1;
                sampler.Unregister(info1);

                // Unset the callback.
                EXPECT_EQ(callback, sampler.SetDisposeCallback(nullptr));
                expected = nullptr;  // no more calls.
                sampler.Unregister(info2);
            }

        }  // namespace
    }  // namespace container_internal

}  // namespace abel
