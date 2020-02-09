//
// Created by liyinbin on 2020/2/8.
//

//
// Created by liyinbin on 2019/10/31.
//
#include <abel/metrics/scope.h>
#include <abel/metrics/prom_serializer.h>
#include <gtest/gtest.h>
#include <limits>
#include <iostream>
#include <thread>

TEST(ScopeTest, scope) {
    auto scopePtr = abel::metrics::scope::new_root_scope("test", "_",
                                               {{"product", "demo1"}});
    auto c = scopePtr->get_counter("counter");
    c->inc(1);

    auto g = scopePtr->get_gauge("gauge");
    g->inc(2);

    abel::metrics::bucket bucket = abel::metrics::bucket_builder::exponential_values(0.1, 1.5, 20);
    auto h = scopePtr->get_histogram("value_histogram", bucket);
    h->observe(0.4);

    abel::metrics::bucket
        durationBucket = abel::metrics::bucket_builder::exponential_duration(abel::nanoseconds(100000000), 2, 20);
    auto h1 = scopePtr->get_histogram("duration_histogram", durationBucket);
    h1->observe(0.4);

    abel::metrics::bucket
        durationBucket1 = abel::metrics::bucket_builder::exponential_duration(abel::nanoseconds(100000000), 3, 20);
    auto t1 = scopePtr->get_timer("duration_timer", durationBucket1);
    t1->observe(400000000);
    auto sw = t1->start();
    std::this_thread::sleep_for(std::chrono::nanoseconds(600000000));
    sw.stop();

    scopePtr->tagged({{"host", "h1"}, {"user", "u1"}})->get_gauge("qps2")->inc(5);

    std::vector<abel::metrics::cache_metrics> cm;
    scopePtr->collect(cm);

    abel::metrics::serializer *serializer = new abel::metrics::prometheus_serializer();
    std::string p = serializer->format(cm);
    std::cout << p << std::endl;
}

