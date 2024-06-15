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


#include <limits>                           //std::numeric_limits
#include <melon/var/detail/sampler.h>
#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <gtest/gtest.h>
#include <melon/base/config.h>
namespace {

TEST(SamplerTest, linked_list) {
    mutil::LinkNode<melon::var::detail::Sampler> n1, n2;
    n1.InsertBeforeAsList(&n2);
    ASSERT_EQ(n1.next(), &n2);
    ASSERT_EQ(n1.previous(), &n2);
    ASSERT_EQ(n2.next(), &n1);
    ASSERT_EQ(n2.previous(), &n1);

    mutil::LinkNode<melon::var::detail::Sampler> n3, n4;
    n3.InsertBeforeAsList(&n4);
    ASSERT_EQ(n3.next(), &n4);
    ASSERT_EQ(n3.previous(), &n4);
    ASSERT_EQ(n4.next(), &n3);
    ASSERT_EQ(n4.previous(), &n3);

    n1.InsertBeforeAsList(&n3);
    ASSERT_EQ(n1.next(), &n2);
    ASSERT_EQ(n2.next(), &n3);
    ASSERT_EQ(n3.next(), &n4);
    ASSERT_EQ(n4.next(), &n1);
    ASSERT_EQ(&n1, n2.previous());
    ASSERT_EQ(&n2, n3.previous());
    ASSERT_EQ(&n3, n4.previous());
    ASSERT_EQ(&n4, n1.previous());
}

class DebugSampler : public melon::var::detail::Sampler {
public:
    DebugSampler() : _ncalled(0) {}
    ~DebugSampler() {
        ++_s_ndestroy;
    }
    void take_sample() {
        ++_ncalled;
    }
    int called_count() const { return _ncalled; }
private:
    int _ncalled;
    static int _s_ndestroy;
};
int DebugSampler::_s_ndestroy = 0;

TEST(SamplerTest, single_threaded) {
#if !MELON_WITH_GLOG
    logging::StringSink log_str;
    logging::LogSink* old_sink = logging::SetLogSink(&log_str);
#endif
    const int N = 100;
    DebugSampler* s[N];
    for (int i = 0; i < N; ++i) {
        s[i] = new DebugSampler;
        s[i]->schedule();
    }
    usleep(1010000);
    for (int i = 0; i < N; ++i) {
        // LE: called once every second, may be called more than once
        ASSERT_LE(1, s[i]->called_count()) << "i=" << i;
    }
    EXPECT_EQ(0, DebugSampler::_s_ndestroy);
    for (int i = 0; i < N; ++i) {
        s[i]->destroy();
    }
    usleep(1010000);
    EXPECT_EQ(N, DebugSampler::_s_ndestroy);
#if !MELON_WITH_GLOG
    ASSERT_EQ(&log_str, logging::SetLogSink(old_sink));
    if (log_str.find("Removed ") != std::string::npos) {
        ASSERT_NE(std::string::npos, log_str.find("Removed 0, sampled 100"));
        ASSERT_NE(std::string::npos, log_str.find("Removed 100, sampled 0"));
    }
#endif
}

static void* check(void*) {
    const int N = 100;
    DebugSampler* s[N];
    for (int i = 0; i < N; ++i) {
        s[i] = new DebugSampler;
        s[i]->schedule();
    }
    usleep(1010000);
    for (int i = 0; i < N; ++i) {
        EXPECT_LE(1, s[i]->called_count()) << "i=" << i;
    }
    for (int i = 0; i < N; ++i) {
        s[i]->destroy();
    }
    return NULL;
}

TEST(SamplerTest, multi_threaded) {
#if !MELON_WITH_GLOG
    logging::StringSink log_str;
    logging::LogSink* old_sink = logging::SetLogSink(&log_str);
#endif
    pthread_t th[10];
    DebugSampler::_s_ndestroy = 0;
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(0, pthread_create(&th[i], NULL, check, NULL));
    }
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(0, pthread_join(th[i], NULL));
    }
    sleep(1);
    EXPECT_EQ(100 * arraysize(th), (size_t)DebugSampler::_s_ndestroy);
#if !MELON_WITH_GLOG
    ASSERT_EQ(&log_str, logging::SetLogSink(old_sink));
    if (log_str.find("Removed ") != std::string::npos) {
        ASSERT_NE(std::string::npos, log_str.find("Removed 0, sampled 1000"));
        ASSERT_NE(std::string::npos, log_str.find("Removed 1000, sampled 0"));
    }
#endif
}
} // namespace
