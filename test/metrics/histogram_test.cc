
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/metrics/histogram.h"
#include "melon/metrics/prometheus_dumper.h"
#include "melon/base/fast_rand.h"
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include <iostream>
#include "testing/gtest_wrap.h"

TEST(metrics, histogram) {

    auto b = melon::bucket_builder::liner_values(10, 4, 5);
    melon::histogram h1("h1","", b, {{"a","search"}, {"q","qruu"}});
    for(int i =0 ; i < 200; i++) {
        auto v = melon::base::fast_rand_in(0, 25);
        h1.observe(v);
    }

    for(int i =0 ; i < 200; i++) {
        auto v = melon::base::fast_rand_in(0, 25);
        h1<<v;
    }
    melon::cache_metrics cm;
    h1.collect_metrics(cm);
    auto t = melon::time_now();
    auto str = melon::prometheus_dumper::dump_to_string(cm, &t);
    std::cout <<str<<std::endl;

    melon::histogram h2;
    h2.expose("h2","", b, {{"a","search"}, {"q","qruu"}});

    for(int i =0 ; i < 1800; i++) {
        auto v = melon::base::fast_rand_in(0, 1900);
        h1<<v;
    }

    melon::cache_metrics cm2;
    h2.collect_metrics(cm2);
    auto t2 = melon::time_now();
    auto str2 = melon::prometheus_dumper::dump_to_string(cm2, &t);
    std::cout <<str2<<std::endl;

    std::vector<melon::cache_metrics> cml;
    melon::variable_base::list_metrics(&cml);
    EXPECT_EQ(cml.size(),2ul);
}