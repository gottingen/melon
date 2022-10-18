
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/dag/dag.h"
#include "testing/gtest_wrap.h"

using namespace testing;

namespace {

    struct Data {
        std::mutex mutex;
        std::vector<std::string> order;

        void push(std::string &&s) {
            std::unique_lock<std::mutex> lock(mutex);
            order.emplace_back(std::move(s));
        }
    };

    template<typename T>
    std::vector<T> slice(const std::vector<T> &in, size_t from, size_t to) {
        return {in.begin() + from, in.begin() + to};
    }

}  // namespace

//  [A] --> [B] --> [C]                                                        |
TEST(dag, DAGChainNoArg) {
    melon::dag_graph<>::builder_type builder;

    Data data;
    builder.root()
            .then([&] { data.push("A"); })
            .then([&] { data.push("B"); })
            .then([&] { data.push("C"); });

    auto dag = builder.build();
    dag->run();

    ASSERT_THAT(data.order, ElementsAre("A", "B", "C"));
}

//  [A] --> [B] --> [C]                                                        |
TEST(dag, dag_async_void) {
    melon::dag_graph<>::builder_type builder;

    Data data;
    builder.root()
            .then([&] { data.push("A"); })
            .then([&] { data.push("B"); })
            .then([&] { data.push("C"); });

    auto dag = builder.build();
    auto w = dag->async();
    w.wait();
    ASSERT_THAT(data.order, ElementsAre("A", "B", "C"));
}

//  [A] --> [B] --> [C]                                                        |
TEST(dag, dag_async_void_time_out) {
    melon::dag_graph<>::builder_type builder;

    Data data;
    builder.root()
            .then([&] { data.push("A"); })
            .then([&] { data.push("B"); })
            .then([&] { std::this_thread::sleep_for(
                    std::chrono::duration<double, std::milli>(100.0)); })
            .then([&] { data.push("C"); });

    auto dag = builder.build();
    auto w = dag->async();
    auto r = w.wait_for(melon::duration::milliseconds(30));
    ASSERT_THAT(data.order, ElementsAre("A", "B"));
    EXPECT_EQ(r, false);
    w.wait();
    ASSERT_THAT(data.order, ElementsAre("A", "B", "C"));
}


//  [A] --> [B] --> [C]                                                        |
TEST(dag, DAGChain) {
    melon::dag_graph<Data &>::builder_type builder;

    builder.root()
            .then([](Data &data) { data.push("A"); })
            .then([](Data &data) { data.push("B"); })
            .then([](Data &data) { data.push("C"); });

    auto dag = builder.build();

    Data data;
    dag->run(data);

    ASSERT_THAT(data.order, ElementsAre("A", "B", "C"));
}

//  [A] --> [B] --> [C]                                                        |
TEST(dag, DAGRunRepeat) {
    melon::dag_graph<Data &>::builder_type builder;

    builder.root()
            .then([](Data &data) { data.push("A"); })
            .then([](Data &data) { data.push("B"); })
            .then([](Data &data) { data.push("C"); });

    auto dag = builder.build();

    Data dataA, dataB;
    dag->run(dataA);
    dag->run(dataB);
    dag->run(dataA);

    ASSERT_THAT(dataA.order, ElementsAre("A", "B", "C", "A", "B", "C"));
    ASSERT_THAT(dataB.order, ElementsAre("A", "B", "C"));
}

//           /--> [A]                                                          |
//  [root] --|--> [B]                                                          |
//           \--> [C]                                                          |
TEST(dag, DAGFanOutFromRoot) {
    melon::dag_graph<Data &>::builder_type builder;

    auto root = builder.root();
    root.then([](Data &data) { data.push("A"); });
    root.then([](Data &data) { data.push("B"); });
    root.then([](Data &data) { data.push("C"); });

    auto dag = builder.build();

    Data data;
    dag->run(data);

    ASSERT_THAT(data.order, UnorderedElementsAre("A", "B", "C"));
}

//                /--> [A]                                                     |
// [root] -->[N]--|--> [B]                                                     |
//                \--> [C]                                                     |
TEST(dag, DAGFanOutFromNonRoot) {
    melon::dag_graph<Data &>::builder_type builder;

    auto root = builder.root();
    auto node = root.then([](Data &data) { data.push("N"); });
    node.then([](Data &data) { data.push("A"); });
    node.then([](Data &data) { data.push("B"); });
    node.then([](Data &data) { data.push("C"); });

    auto dag = builder.build();

    Data data;
    dag->run(data);

    ASSERT_THAT(data.order, UnorderedElementsAre("N", "A", "B", "C"));
    ASSERT_EQ(data.order[0], "N");
    ASSERT_THAT(slice(data.order, 1, 4), UnorderedElementsAre("A", "B", "C"));
}

//          /--> [A0] --\        /--> [C0] --\        /--> [E0] --\            |
// [root] --|--> [A1] --|-->[B]--|--> [C1] --|-->[D]--|--> [E1] --|-->[F]      |
//                               \--> [C2] --/        |--> [E2] --|            |
//                                                    \--> [E3] --/            |
TEST(dag, DAGFanOutFanIn) {
    melon::dag_graph<Data &>::builder_type builder;

    auto root = builder.root();
    auto a0 = root.then([](Data &data) { data.push("A0"); });
    auto a1 = root.then([](Data &data) { data.push("A1"); });

    auto b = builder.node([](Data &data) { data.push("B"); }, {a0, a1});

    auto c0 = b.then([](Data &data) { data.push("C0"); });
    auto c1 = b.then([](Data &data) { data.push("C1"); });
    auto c2 = b.then([](Data &data) { data.push("C2"); });

    auto d = builder.node([](Data &data) { data.push("D"); }, {c0, c1, c2});

    auto e0 = d.then([](Data &data) { data.push("E0"); });
    auto e1 = d.then([](Data &data) { data.push("E1"); });
    auto e2 = d.then([](Data &data) { data.push("E2"); });
    auto e3 = d.then([](Data &data) { data.push("E3"); });

    builder.node([](Data &data) { data.push("F"); }, {e0, e1, e2, e3});

    auto dag = builder.build();

    Data data;
    dag->run(data);

    ASSERT_THAT(data.order,
                UnorderedElementsAre("A0", "A1", "B", "C0", "C1", "C2", "D", "E0",
                                     "E1", "E2", "E3", "F"));
    ASSERT_THAT(slice(data.order, 0, 2), UnorderedElementsAre("A0", "A1"));
    ASSERT_THAT(data.order[2], "B");
    ASSERT_THAT(slice(data.order, 3, 6), UnorderedElementsAre("C0", "C1", "C2"));
    ASSERT_THAT(data.order[6], "D");
    ASSERT_THAT(slice(data.order, 7, 11),
                UnorderedElementsAre("E0", "E1", "E2", "E3"));
    ASSERT_THAT(data.order[11], "F");
}
