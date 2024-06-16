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


#include <algorithm>                         // std::sort
#include <melon/utility/atomicops.h>
#include <gtest/gtest.h>
#include <melon/utility/time.h>
#include <melon/base/macros.h>
#include <melon/base/scoped_lock.h>
#include <turbo/log/logging.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/unstable.h>

extern "C" {
int fiber_keytable_pool_size(fiber_keytable_pool_t* pool) {
    fiber_keytable_pool_stat_t s;
    if (fiber_keytable_pool_getstat(pool, &s) != 0) {
        return 0;
    }
    return s.nfree;
}
}

namespace {

// Count tls usages.
struct Counters {
    std::atomic<size_t> ncreate {0};
    std::atomic<size_t> ndestroy {0};
    std::atomic<size_t> nenterthread {0};
    std::atomic<size_t> nleavethread {0};
};

// Wrap same counters into different objects to make sure that different key
// returns different objects as well as aggregate the usages.
struct CountersWrapper {
    CountersWrapper(Counters* c, fiber_key_t key) : _c(c), _key(key) {}
    ~CountersWrapper() {
        if (_c) {
            _c->ndestroy.fetch_add(1, std::memory_order_relaxed);
        }
        CHECK_EQ(0, fiber_key_delete(_key));
    }
private:
    Counters* _c;
    fiber_key_t _key;
};

static void destroy_counters_wrapper(void* arg) {
    delete static_cast<CountersWrapper*>(arg);
}

const size_t NKEY_PER_WORKER = 32;

// NOTE: returns void to use ASSERT
static void worker1_impl(Counters* cs) {
    cs->nenterthread.fetch_add(1, std::memory_order_relaxed);
    fiber_key_t k[NKEY_PER_WORKER];
    CountersWrapper* ws[arraysize(k)];
    for (size_t i = 0; i < arraysize(k); ++i) {
        ASSERT_EQ(0, fiber_key_create(&k[i], destroy_counters_wrapper));
    }
    for (size_t i = 0; i < arraysize(k); ++i) {
        ws[i] = new CountersWrapper(cs, k[i]);
    }
    // Get just-created tls should return NULL.
    for (size_t i = 0; i < arraysize(k); ++i) {
        ASSERT_EQ(NULL, fiber_getspecific(k[i]));
    }
    for (size_t i = 0; i < arraysize(k); ++i) {
        cs->ncreate.fetch_add(1, std::memory_order_relaxed);
        ASSERT_EQ(0, fiber_setspecific(k[i], ws[i]))
            << "i=" << i << " is_fiber=" << !!fiber_self();
            
    }
    // Sleep a while to make some context switches. TLS should be unchanged.
    fiber_usleep(10000);
    
    for (size_t i = 0; i < arraysize(k); ++i) {
        ASSERT_EQ(ws[i], fiber_getspecific(k[i])) << "i=" << i;
    }
    cs->nleavethread.fetch_add(1, std::memory_order_relaxed);
}

static void* worker1(void* arg) {
    worker1_impl(static_cast<Counters*>(arg));
    return NULL;
}

TEST(KeyTest, creating_key_in_parallel) {
    Counters args;
    pthread_t th[8];
    fiber_t bth[8];
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(0, pthread_create(&th[i], NULL, worker1, &args));
    }
    for (size_t i = 0; i < arraysize(bth); ++i) {
        ASSERT_EQ(0, fiber_start_background(&bth[i], NULL, worker1, &args));
    }
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(0, pthread_join(th[i], NULL));
    }
    for (size_t i = 0; i < arraysize(bth); ++i) {
        ASSERT_EQ(0, fiber_join(bth[i], NULL));
    }
    ASSERT_EQ(arraysize(th) + arraysize(bth),
              args.nenterthread.load(std::memory_order_relaxed));
    ASSERT_EQ(arraysize(th) + arraysize(bth),
              args.nleavethread.load(std::memory_order_relaxed));
    ASSERT_EQ(NKEY_PER_WORKER * (arraysize(th) + arraysize(bth)),
              args.ncreate.load(std::memory_order_relaxed));
    ASSERT_EQ(NKEY_PER_WORKER * (arraysize(th) + arraysize(bth)),
              args.ndestroy.load(std::memory_order_relaxed));
}

std::atomic<size_t> seq(1);
std::vector<size_t> seqs;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void dtor2(void* arg) {
    MELON_SCOPED_LOCK(mutex);
    seqs.push_back((size_t)arg);
}

// NOTE: returns void to use ASSERT
static void worker2_impl(fiber_key_t k) {
    ASSERT_EQ(NULL, fiber_getspecific(k));
    ASSERT_EQ(0, fiber_setspecific(k, (void*)seq.fetch_add(1)));
}

static void* worker2(void* arg) {
    worker2_impl(*static_cast<fiber_key_t*>(arg));
    return NULL;
}

TEST(KeyTest, use_one_key_in_different_threads) {
    fiber_key_t k;
    ASSERT_EQ(0, fiber_key_create(&k, dtor2)) << berror();
    seqs.clear();

    pthread_t th[16];
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(0, pthread_create(&th[i], NULL, worker2, &k));
    }
    fiber_t bth[1];
    for (size_t i = 0; i < arraysize(bth); ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&bth[i], NULL, worker2, &k));
    }
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(0, pthread_join(th[i], NULL));
    }
    for (size_t i = 0; i < arraysize(bth); ++i) {
        ASSERT_EQ(0, fiber_join(bth[i], NULL));
    }
    ASSERT_EQ(arraysize(th) + arraysize(bth), seqs.size());
    std::sort(seqs.begin(), seqs.end());
    ASSERT_EQ(seqs.end(), std::unique(seqs.begin(), seqs.end()));
    ASSERT_EQ(arraysize(th) + arraysize(bth) - 1, *(seqs.end()-1) - *seqs.begin());

    ASSERT_EQ(0, fiber_key_delete(k));
}

struct Keys {
    fiber_key_t valid_key;
    fiber_key_t invalid_key;
};

void* const DUMMY_PTR = (void*)1;

void use_invalid_keys_impl(const Keys* keys) {
    ASSERT_EQ(NULL, fiber_getspecific(keys->invalid_key));
    // valid key returns NULL as well.
    ASSERT_EQ(NULL, fiber_getspecific(keys->valid_key));

    // both pthread_setspecific(of nptl) and fiber_setspecific should find
    // the key is invalid.
    ASSERT_EQ(EINVAL, fiber_setspecific(keys->invalid_key, DUMMY_PTR));
    ASSERT_EQ(0, fiber_setspecific(keys->valid_key, DUMMY_PTR));

    // Print error again.
    ASSERT_EQ(NULL, fiber_getspecific(keys->invalid_key));
    ASSERT_EQ(DUMMY_PTR, fiber_getspecific(keys->valid_key));
}

void* use_invalid_keys(void* args) {
    use_invalid_keys_impl(static_cast<const Keys*>(args));
    return NULL;
}

TEST(KeyTest, use_invalid_keys) {
    Keys keys;
    ASSERT_EQ(0, fiber_key_create(&keys.valid_key, NULL));
    // intended to be a created but invalid key.
    keys.invalid_key.index = keys.valid_key.index;
    keys.invalid_key.version = 123;

    pthread_t th;
    fiber_t bth;
    ASSERT_EQ(0, pthread_create(&th, NULL, use_invalid_keys, &keys));
    ASSERT_EQ(0, fiber_start_urgent(&bth, NULL, use_invalid_keys, &keys));
    ASSERT_EQ(0, pthread_join(th, NULL));
    ASSERT_EQ(0, fiber_join(bth, NULL));
    ASSERT_EQ(0, fiber_key_delete(keys.valid_key));
}

TEST(KeyTest, reuse_key) {
    fiber_key_t key;
    ASSERT_EQ(0, fiber_key_create(&key, NULL));
    ASSERT_EQ(NULL, fiber_getspecific(key));
    ASSERT_EQ(0, fiber_setspecific(key, (void*)1));
    ASSERT_EQ(0, fiber_key_delete(key)); // delete key before clearing TLS.
    fiber_key_t key2;
    ASSERT_EQ(0, fiber_key_create(&key2, NULL));
    ASSERT_EQ(key.index, key2.index);
    // The slot is not NULL, the impl must check version and return NULL.
    ASSERT_EQ(NULL, fiber_getspecific(key2));
}

// NOTE: sid is short for 'set in dtor'.
struct SidData {
    fiber_key_t key;
    int seq;
    int end_seq;
};

static void sid_dtor(void* tls){
    SidData* data = (SidData*)tls;
    // Should already be set NULL.
    ASSERT_EQ(NULL, fiber_getspecific(data->key));
    if (++data->seq < data->end_seq){
        ASSERT_EQ(0, fiber_setspecific(data->key, data));
    }
}

static void sid_thread_impl(SidData* data) {
    ASSERT_EQ(0, fiber_setspecific(data->key, data));
};

static void* sid_thread(void* args) {
    sid_thread_impl((SidData*)args);
    return NULL;
}

TEST(KeyTest, set_in_dtor) {
    fiber_key_t key;
    ASSERT_EQ(0, fiber_key_create(&key, sid_dtor));

    SidData pth_data = { key, 0, 3 };
    SidData bth_data = { key, 0, 3 };
    SidData bth2_data = { key, 0, 3 };
    
    pthread_t pth;
    fiber_t bth;
    fiber_t bth2;
    ASSERT_EQ(0, pthread_create(&pth, NULL, sid_thread, &pth_data));
    ASSERT_EQ(0, fiber_start_urgent(&bth, NULL, sid_thread, &bth_data));
    ASSERT_EQ(0, fiber_start_urgent(&bth2, &FIBER_ATTR_PTHREAD,
                                      sid_thread, &bth2_data));

    ASSERT_EQ(0, pthread_join(pth, NULL));
    ASSERT_EQ(0, fiber_join(bth, NULL));
    ASSERT_EQ(0, fiber_join(bth2, NULL));
        
    ASSERT_EQ(0, fiber_key_delete(key));

    EXPECT_EQ(pth_data.end_seq, pth_data.seq);
    EXPECT_EQ(bth_data.end_seq, bth_data.seq);
    EXPECT_EQ(bth2_data.end_seq, bth2_data.seq);
}

struct SBAData {
    fiber_key_t key;
    int level;
    int ndestroy;
};

struct SBATLS {
    int* ndestroy;
    
    static void deleter(void* d) {
        SBATLS* tls = (SBATLS*)d;
        ++*tls->ndestroy;
        delete tls;
    }
};

void* set_before_anybth(void* args);

void set_before_anybth_impl(SBAData* data) {
    ASSERT_EQ(NULL, fiber_getspecific(data->key));
    SBATLS *tls = new SBATLS;
    tls->ndestroy = &data->ndestroy;
    ASSERT_EQ(0, fiber_setspecific(data->key, tls));
    ASSERT_EQ(tls, fiber_getspecific(data->key));
    if (data->level++ == 0) {
        fiber_t bth;
        ASSERT_EQ(0, fiber_start_urgent(&bth, NULL, set_before_anybth, data));
        ASSERT_EQ(0, fiber_join(bth, NULL));
        ASSERT_EQ(1, data->ndestroy);
    } else {
        fiber_usleep(1000);
    }
    ASSERT_EQ(tls, fiber_getspecific(data->key));
}

void* set_before_anybth(void* args) {
    set_before_anybth_impl((SBAData*)args);
    return NULL;
}

TEST(KeyTest, set_tls_before_creating_any_fiber) {
    fiber_key_t key;
    ASSERT_EQ(0, fiber_key_create(&key, SBATLS::deleter));
    pthread_t th;
    SBAData data;
    data.key = key;
    data.level = 0;
    data.ndestroy = 0;
    ASSERT_EQ(0, pthread_create(&th, NULL, set_before_anybth, &data));
    ASSERT_EQ(0, pthread_join(th, NULL));
    ASSERT_EQ(0, fiber_key_delete(key));
    ASSERT_EQ(2, data.level);
    ASSERT_EQ(2, data.ndestroy);
}

struct PoolData {
    fiber_key_t key;
    PoolData* expected_data;
    int seq;
    int end_seq;
};

static void pool_thread_impl(PoolData* data) {
    ASSERT_EQ(data->expected_data, (PoolData*)fiber_getspecific(data->key));
    if (NULL == fiber_getspecific(data->key)) {
        ASSERT_EQ(0, fiber_setspecific(data->key, data));
    }
};

static void* pool_thread(void* args) {
    pool_thread_impl((PoolData*)args);
    return NULL;
}

static void pool_dtor(void* tls){
    PoolData* data = (PoolData*)tls;
    // Should already be set NULL.
    ASSERT_EQ(NULL, fiber_getspecific(data->key));
    if (++data->seq < data->end_seq){
        ASSERT_EQ(0, fiber_setspecific(data->key, data));
    }
}

TEST(KeyTest, using_pool) {
    fiber_key_t key;
    ASSERT_EQ(0, fiber_key_create(&key, pool_dtor));

    fiber_keytable_pool_t pool;
    ASSERT_EQ(0, fiber_keytable_pool_init(&pool));
    ASSERT_EQ(0, fiber_keytable_pool_size(&pool));

    fiber_attr_t attr;
    ASSERT_EQ(0, fiber_attr_init(&attr));
    attr.keytable_pool = &pool;

    fiber_attr_t attr2 = attr;
    attr2.stack_type = FIBER_STACKTYPE_PTHREAD;

    PoolData bth_data = { key, NULL, 0, 3 };
    fiber_t bth;
    ASSERT_EQ(0, fiber_start_urgent(&bth, &attr, pool_thread, &bth_data));
    ASSERT_EQ(0, fiber_join(bth, NULL));
    ASSERT_EQ(0, bth_data.seq);
    ASSERT_EQ(1, fiber_keytable_pool_size(&pool));

    PoolData bth2_data = { key, &bth_data, 0, 3 };
    fiber_t bth2;
    ASSERT_EQ(0, fiber_start_urgent(&bth2, &attr2, pool_thread, &bth2_data));
    ASSERT_EQ(0, fiber_join(bth2, NULL));
    ASSERT_EQ(0, bth2_data.seq);
    ASSERT_EQ(1, fiber_keytable_pool_size(&pool));
        
    ASSERT_EQ(0, fiber_keytable_pool_destroy(&pool));
    
    EXPECT_EQ(bth_data.end_seq, bth_data.seq);
    EXPECT_EQ(0, bth2_data.seq);

    ASSERT_EQ(0, fiber_key_delete(key));
}

}  // namespace
