// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


//
// Created by liyinbin on 2019/10/28.
//

#include "abel/metrics/histogram.h"
#include "gtest/gtest.h"
#include <limits>

using abel::metrics::histogram;

TEST(HistogramTest, initialize_with_zero) {
    histogram hist{{}};
    auto metric = hist.collect();
    auto h = metric.histogram;
    EXPECT_EQ(h.sample_count, 0U);
    EXPECT_EQ(h.sample_sum, 0);
}

TEST(HistogramTest, sample_count) {
    histogram hist{{1}};
    hist.observe(0);
    hist.observe(200);
    auto metric = hist.collect();
    auto h = metric.histogram;
    EXPECT_EQ(h.sample_count, 2U);
}

TEST(HistogramTest, sample_sum) {
    histogram hist{{1}};
    hist.observe(0);
    hist.observe(1);
    hist.observe(101);
    auto metric = hist.collect();
    auto h = metric.histogram;
    EXPECT_EQ(h.sample_sum, 102);
}

TEST(HistogramTest, bucket_size) {
    histogram hist{{1, 2}};
    auto metric = hist.collect();
    auto h = metric.histogram;
    EXPECT_EQ(h.bucket.size(), 3U);
}

TEST(HistogramTest, bucket_bounds) {
    histogram hist{{1, 2}};
    auto metric = hist.collect();
    auto h = metric.histogram;
    EXPECT_EQ(h.bucket.at(0).upper_bound, 1);
    EXPECT_EQ(h.bucket.at(1).upper_bound, 2);
    EXPECT_EQ(h.bucket.at(2).upper_bound,
              std::numeric_limits<double>::infinity());
}

TEST(HistogramTest, bucket_counts_not_reset_by_collection) {
    histogram hist{{1, 2}};
    hist.observe(1.5);
    hist.collect();
    hist.observe(1.5);
    auto metric = hist.collect();
    auto h = metric.histogram;
    ASSERT_EQ(h.bucket.size(), 3U);
    EXPECT_EQ(h.bucket.at(1).cumulative_count, 2U);
}

TEST(HistogramTest, cumulative_bucket_count) {
    histogram hist{{1, 2}};
    hist.observe(0);
    hist.observe(0.5);
    hist.observe(1);
    hist.observe(1.5);
    hist.observe(1.5);
    hist.observe(2);
    hist.observe(3);
    auto metric = hist.collect();
    auto h = metric.histogram;
    ASSERT_EQ(h.bucket.size(), 3U);
    EXPECT_EQ(h.bucket.at(0).cumulative_count, 3U);
    EXPECT_EQ(h.bucket.at(1).cumulative_count, 6U);
    EXPECT_EQ(h.bucket.at(2).cumulative_count, 7U);
}

TEST(HistogramTest, BucketBuilder) {
    auto bucket = abel::metrics::bucket_builder::liner_values(1, 1, 2);
    histogram hist(bucket);
    hist.observe(0);
    hist.observe(0.5);
    hist.observe(1);
    hist.observe(1.5);
    hist.observe(1.5);
    hist.observe(2);
    hist.observe(3);
    auto metric = hist.collect();
    auto h = metric.histogram;
    ASSERT_EQ(h.bucket.size(), 3U);
    EXPECT_EQ(h.bucket.at(0).cumulative_count, 3U);
    EXPECT_EQ(h.bucket.at(1).cumulative_count, 6U);
    EXPECT_EQ(h.bucket.at(2).cumulative_count, 7U);
}


