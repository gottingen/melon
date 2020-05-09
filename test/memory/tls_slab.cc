//
// Created by liyinbin on 2020/5/10.
//


#include <gtest/gtest.h>
#include <abel/chrono/clock.h>
#include <abel/memory/tls_slab.h>
#include <abel/stats/random/random.h>
#include <abel/chrono/stop_watcher.h>

namespace {
    struct MyObject {};

    int nfoo_dtor = 0;
    struct Foo {
        Foo() {
            abel::bit_gen engine;
            x = static_cast<int>(abel::uniform<uint32_t>(engine)%2);
        }
        ~Foo() {
            ++nfoo_dtor;
        }
        int x;

    };
}

namespace abel {
    template <> struct tls_slab_block_max_size<MyObject> {
        static const size_t value = 128;
    };

    template <> struct tls_slab_block_max_item<MyObject> {
        static const size_t value = 3;
    };

    template <> struct tls_slab_block_max_free_chunk<MyObject> {
        static size_t value() { return 5; }
    };

    template <> struct tls_slab_validator<Foo> {
        static bool validate(const Foo* foo) {
            return foo->x != 0;
        }
    };

}

namespace {
    using namespace abel;

    class tls_slabTest : public ::testing::Test{
    protected:
        tls_slabTest(){
        };
        virtual ~tls_slabTest(){};
        virtual void SetUp() {
            srand(time(0));
        };
        virtual void TearDown() {
        };
    };

    TEST_F(tls_slabTest, atomic_array_init) {
        for (int i = 0; i < 2; ++i) {
            if (i == 0) {
                std::atomic<int> a[2];
                a[0] = 1;
                // The folowing will cause compile error with gcc3.4.5 and the
                // reason is unknown
                // a[1] = 2;
            } else if (i == 2) {
                std::atomic<int> a[2];
                ASSERT_EQ(0, a[0]);
                ASSERT_EQ(0, a[1]);
            }
        }
    }

    int nc = 0;
    int nd = 0;
    std::set<void*> ptr_set;
    struct YellObj {
        YellObj() {
            ++nc;
            ptr_set.insert(this);
            printf("Created %p\n", (void*)this);
        }
        ~YellObj() {
            ++nd;
            ptr_set.erase(this);
            printf("Destroyed %p\n", (void*)this);
        }
        char _dummy[96];
    };

    TEST_F(tls_slabTest, change_config) {
        int a[2];
        printf("%lu\n", ABEL_ARRAYSIZE(a));

        tls_slab_info info = describe_resources<MyObject>();
        tls_slab_info zero_info = { 0, 0, 0, 0, 3, 3, 0 };
        ASSERT_EQ(0, memcmp(&info, &zero_info, sizeof(info)));

        item_id<MyObject> id = { 0 };
        get_resource<MyObject>(&id);
        std::cout << describe_resources<MyObject>() << std::endl;
        return_resource(id);
        std::cout << describe_resources<MyObject>() << std::endl;
    }

    struct NonDefaultCtorObject {
        explicit NonDefaultCtorObject(int value) : _value(value) {}
        NonDefaultCtorObject(int value, int dummy) : _value(value + dummy) {}

        int _value;
    };

    TEST_F(tls_slabTest, sanity) {
        ptr_set.clear();
        item_id<NonDefaultCtorObject> id0 = { 0 };
        get_resource<NonDefaultCtorObject>(&id0, 10);
        ASSERT_EQ(10, address_resource(id0)->_value);
        get_resource<NonDefaultCtorObject>(&id0, 100, 30);
        ASSERT_EQ(130, address_resource(id0)->_value);

        printf("BLOCK_NITEM=%lu\n", tls_slab<YellObj>::BLOCK_NITEM);

        nc = 0;
        nd = 0;
        {
            item_id<YellObj> id1;
            YellObj* o1 = get_resource(&id1);
            ASSERT_TRUE(o1);
            ASSERT_EQ(o1, address_resource(id1));

            ASSERT_EQ(1, nc);
            ASSERT_EQ(0, nd);

            item_id<YellObj> id2;
            YellObj* o2 = get_resource(&id2);
            ASSERT_TRUE(o2);
            ASSERT_EQ(o2, address_resource(id2));

            ASSERT_EQ(2, nc);
            ASSERT_EQ(0, nd);

            return_resource(id1);
            ASSERT_EQ(2, nc);
            ASSERT_EQ(0, nd);

            return_resource(id2);
            ASSERT_EQ(2, nc);
            ASSERT_EQ(0, nd);
        }
        ASSERT_EQ(0, nd);

        clear_resources<YellObj>();
        ASSERT_EQ(2, nd);
        ASSERT_TRUE(ptr_set.empty()) << ptr_set.size();
    }

    TEST_F(tls_slabTest, validator) {
        nfoo_dtor = 0;
        int nfoo = 0;
        for (int i = 0; i < 100; ++i) {
            item_id<Foo> id = { 0 };
            Foo* foo = get_resource<Foo>(&id);
            if (foo) {
                ASSERT_EQ(1, foo->x);
                ++nfoo;
            }
        }
        ASSERT_EQ(nfoo + nfoo_dtor, 100);
        ASSERT_EQ((size_t)nfoo, describe_resources<Foo>().item_num);
    }

    TEST_F(tls_slabTest, get_int) {
        clear_resources<int>();

        // Perf of this test is affected by previous case.
        const size_t N = 100000;

        abel::stop_watcher tm;
        item_id<int> id;

        // warm up
        if (get_resource(&id)) {
            return_resource(id);
        }
        ASSERT_EQ(0UL, id);
        delete (new int);

        tm.start();
        for (size_t i = 0; i < N; ++i) {
            *get_resource(&id) = i;
        }
        tm.stop();
        printf("get a int takes %.1fns\n", abel::to_double_nanoseconds(tm.elapsed()));

        tm.start();
        for (size_t i = 0; i < N; ++i) {
            *(new int) = i;
        }
        tm.stop();
        printf("new a int takes %luns\n", (uint64_t)abel::to_int64_nanoseconds(tm.elapsed()));

        tm.start();
        for (size_t i = 0; i < N; ++i) {
            id.value = i;
            *tls_slab<int>::unsafe_address_resource(id) = i;
        }
        tm.stop();
        printf("unsafe_address a int takes %.1fns\n", abel::to_double_nanoseconds(tm.elapsed()));

        tm.start();
        for (size_t i = 0; i < N; ++i) {
            id.value = i;
            *address_resource(id) = i;
        }
        tm.stop();
        printf("address a int takes %.1fns\n", abel::to_double_nanoseconds(tm.elapsed()));

        std::cout << describe_resources<int>() << std::endl;
        clear_resources<int>();
        std::cout << describe_resources<int>() << std::endl;
    }


    struct SilentObj {
        char buf[sizeof(YellObj)];
    };

    TEST_F(tls_slabTest, get_perf) {
        const size_t N = 10000;
        std::vector<SilentObj*> new_list;
        new_list.reserve(N);
        item_id<SilentObj> id;

        abel::stop_watcher tm1, tm2;

        // warm up
        if (get_resource(&id)) {
            return_resource(id);
        }
        delete (new SilentObj);

        // Run twice, the second time will be must faster.
        for (size_t j = 0; j < 2; ++j) {

            tm1.start();
            for (size_t i = 0; i < N; ++i) {
                get_resource(&id);
            }
            tm1.stop();
            printf("get a SilentObj takes %lluns\n", (uint64_t)abel::to_int64_nanoseconds(tm1.elapsed()));
            //clear_resources<SilentObj>(); // free all blocks

            tm2.start();
            for (size_t i = 0; i < N; ++i) {
                new_list.push_back(new SilentObj);
            }
            tm2.stop();
            printf("new a SilentObj takes %lluns\n", (uint64_t)abel::to_int64_nanoseconds(tm2.elapsed()));
            for (size_t i = 0; i < new_list.size(); ++i) {
                delete new_list[i];
            }
            new_list.clear();
        }

        std::cout << describe_resources<SilentObj>() << std::endl;
    }

    struct D { int val[1]; };

    void* get_and_return_int(void*) {
        // Perf of this test is affected by previous case.
        const size_t N = 100000;
        std::vector<item_id<D> > v;
        v.reserve(N);
        abel::stop_watcher tm0, tm1, tm2;
        item_id<D> id = {0};
        D tmp = D();
        int sr = 0;

        // warm up
        tm0.start();
        if (get_resource(&id)) {
            return_resource(id);
        }
        tm0.stop();

        printf("[%lu] warmup=%lu\n", pthread_self(), (uint64_t)abel::to_int64_nanoseconds(tm0.elapsed()));

        for (int j = 0; j < 5; ++j) {
            v.clear();
            sr = 0;

            tm1.start();
            for (size_t i = 0; i < N; ++i) {
                *get_resource(&id) = tmp;
                v.push_back(id);
            }
            tm1.stop();

            std::random_shuffle(v.begin(), v.end());

            tm2.start();
            for (size_t i = 0; i < v.size(); ++i) {
                sr += return_resource(v[i]);
            }
            tm2.stop();

            if (0 != sr) {
                printf("%d return_resource failed\n", sr);
            }

            printf("[%lu:%d] get<D>=%.1f return<D>=%.1f\n",
                   pthread_self(), j, abel::to_double_nanoseconds(tm1.elapsed()), abel::to_double_nanoseconds(tm1.elapsed()));
        }
        return NULL;
    }

    void* new_and_delete_int(void*) {
        const size_t N = 100000;
        std::vector<D*> v2;
        v2.reserve(N);
        abel::stop_watcher tm0, tm1, tm2;
        D tmp = D();

        for (int j = 0; j < 3; ++j) {
            v2.clear();

            // warm up
            delete (new D);

            tm1.start();
            for (size_t i = 0; i < N; ++i) {
                D *p = new D;
                *p = tmp;
                v2.push_back(p);
            }
            tm1.stop();

            std::random_shuffle(v2.begin(), v2.end());

            tm2.start();
            for (size_t i = 0; i < v2.size(); ++i) {
                delete v2[i];
            }
            tm2.stop();

            printf("[%lu:%d] new<D>=%.1f delete<D>=%.1f\n",
                   pthread_self(), j, abel::to_double_nanoseconds(tm1.elapsed()), abel::to_double_nanoseconds(tm2.elapsed()));
        }

        return NULL;
    }

    TEST_F(tls_slabTest, get_and_return_int_single_thread) {
        get_and_return_int(NULL);
        new_and_delete_int(NULL);
    }

    TEST_F(tls_slabTest, get_and_return_int_multiple_threads) {
        pthread_t tid[16];
        for (size_t i = 0; i < ABEL_ARRAYSIZE(tid); ++i) {
            ASSERT_EQ(0, pthread_create(&tid[i], NULL, get_and_return_int, NULL));
        }
        for (size_t i = 0; i < ABEL_ARRAYSIZE(tid); ++i) {
            pthread_join(tid[i], NULL);
        }

        pthread_t tid2[16];
        for (size_t i = 0; i < ABEL_ARRAYSIZE(tid2); ++i) {
            ASSERT_EQ(0, pthread_create(&tid2[i], NULL, new_and_delete_int, NULL));
        }
        for (size_t i = 0; i < ABEL_ARRAYSIZE(tid2); ++i) {
            pthread_join(tid2[i], NULL);
        }

        std::cout << describe_resources<D>() << std::endl;
        clear_resources<D>();
        tls_slab_info info = describe_resources<D>();
        tls_slab_info zero_info = { 0, 0, 0, 0,
                                    tls_slab_block_max_item<D>::value,
                                    tls_slab_block_max_item<D>::value, 0, 0};
        ASSERT_EQ(0, memcmp(&info, &zero_info, sizeof(info) - sizeof(size_t)));
    }

    TEST_F(tls_slabTest, verify_get) {
        clear_resources<int>();
        std::cout << describe_resources<int>() << std::endl;

        std::vector<std::pair<int*, item_id<int> > > v;
        v.reserve(100000);
        item_id<int> id = { 0 };
        for (int i = 0; (size_t)i < v.capacity(); ++i)  {
            int* p = get_resource(&id);
            *p = i;
            v.push_back(std::make_pair(p, id));
        }
        int i;
        for (i = 0; (size_t)i < v.size() && *v[i].first == i; ++i);
        ASSERT_EQ(v.size(), (size_t)i);
        for (i = 0; (size_t)i < v.size() && v[i].second == (size_t)i; ++i);
        ASSERT_EQ(v.size(), (size_t)i) << "i=" << i << ", " << v[i].second;

        clear_resources<int>();
    }
} // namespace
