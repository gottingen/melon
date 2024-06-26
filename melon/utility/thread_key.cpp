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


#include "thread_key.h"
#include "pthread.h"
#include <deque>
#include <melon/utility/thread_local.h>

namespace mutil {

// Check whether an entry is unused.
#define KEY_UNUSED(p) (((p) & 1) == 0)

// Check whether a key is usable.  We cannot reuse an allocated key if
// the sequence counter would overflow after the next destroy call.
// This would mean that we potentially free memory for a key with the
// same sequence. This is *very* unlikely to happen, A program would
// have to create and destroy a key 2^31 times. If it should happen we
// simply don't use this specific key anymore.
#define KEY_USABLE(p) (((size_t) (p)) < ((size_t) ((p) + 2)))

static const uint32_t THREAD_KEY_RESERVE = 8096;
pthread_mutex_t g_thread_key_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t g_id = 0;
static std::deque<size_t>* g_free_ids = NULL;
static std::vector<ThreadKeyInfo>* g_thread_keys = NULL;
static __thread std::vector<ThreadKeyTLS>* g_tls_data = NULL;

ThreadKey& ThreadKey::operator=(ThreadKey&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    _id = other._id;
    _seq = other._seq;
    other.Reset();
    return *this;
}

bool ThreadKey::Valid() const {
    return _id != InvalidID && !KEY_UNUSED(_seq);
}

static void DestroyTlsData() {
    if (!g_tls_data) {
        return;
    }
    std::vector<ThreadKeyInfo> dummy_keys;
    {
        MELON_SCOPED_LOCK(g_thread_key_mutex);
        if (MELON_LIKELY(g_thread_keys)) {
            dummy_keys.insert(dummy_keys.end(), g_thread_keys->begin(), g_thread_keys->end());
        }
    }
    for (size_t i = 0; i < g_tls_data->size(); ++i) {
        if (!KEY_UNUSED(dummy_keys[i].seq) && dummy_keys[i].dtor) {
            dummy_keys[i].dtor((*g_tls_data)[i].data);
        }
    }
    delete g_tls_data;
    g_tls_data = NULL;
}

static std::deque<size_t>* GetGlobalFreeIds() {
    if (MELON_UNLIKELY(!g_free_ids)) {
        g_free_ids = new (std::nothrow) std::deque<size_t>();
        if (MELON_UNLIKELY(!g_free_ids)) {
            abort();
        }
    }

    return g_free_ids;
}

int thread_key_create(ThreadKey& thread_key, DtorFunction dtor) {
    MELON_SCOPED_LOCK(g_thread_key_mutex);
    size_t id;
    auto free_ids = GetGlobalFreeIds();
    if (!free_ids) {
        return ENOMEM;
    }

    if (!free_ids->empty()) {
        id = free_ids->back();
        free_ids->pop_back();
    } else {
        if (g_id >= ThreadKey::InvalidID) {
            // No more available ids.
            return EAGAIN;
        }
        id = g_id++;
        if(MELON_UNLIKELY(!g_thread_keys)) {
            g_thread_keys = new (std::nothrow) std::vector<ThreadKeyInfo>;
            if(MELON_UNLIKELY(!g_thread_keys)) {
                return ENOMEM;
            }
            g_thread_keys->reserve(THREAD_KEY_RESERVE);
        }
        g_thread_keys->resize(id + 1);
    }

    ++((*g_thread_keys)[id].seq);
    (*g_thread_keys)[id].dtor = dtor;
    thread_key._id = id;
    thread_key._seq = (*g_thread_keys)[id].seq;

    return 0;
}

int thread_key_delete(ThreadKey& thread_key) {
    if (MELON_UNLIKELY(!thread_key.Valid())) {
        return EINVAL;
    }

    MELON_SCOPED_LOCK(g_thread_key_mutex);
    size_t id = thread_key._id;
    size_t seq = thread_key._seq;
    if (id >= g_thread_keys->size() ||
        seq != (*g_thread_keys)[id].seq ||
        KEY_UNUSED((*g_thread_keys)[id].seq)) {
        thread_key.Reset();
        return EINVAL;
    }

    if (MELON_UNLIKELY(!GetGlobalFreeIds())) {
        return ENOMEM;
    }

    ++((*g_thread_keys)[id].seq);
    // Collect the usable key id for reuse.
    if (KEY_USABLE((*g_thread_keys)[id].seq)) {
        GetGlobalFreeIds()->push_back(id);
    }
    thread_key.Reset();

    return 0;
}

int thread_setspecific(ThreadKey& thread_key, void* data) {
    if (MELON_UNLIKELY(!thread_key.Valid())) {
        return EINVAL;
    }
    size_t id = thread_key._id;
    size_t seq = thread_key._seq;
    if (MELON_UNLIKELY(!g_tls_data)) {
        g_tls_data = new (std::nothrow) std::vector<ThreadKeyTLS>;
        if (MELON_UNLIKELY(!g_tls_data)) {
            return ENOMEM;
        }
        g_tls_data->reserve(THREAD_KEY_RESERVE);
        // Register the destructor of tls_data in this thread.
        mutil::thread_atexit(DestroyTlsData);
    }

    if (id >= g_tls_data->size()) {
        g_tls_data->resize(id + 1);
    }

    (*g_tls_data)[id].seq  = seq;
    (*g_tls_data)[id].data = data;

    return 0;
}

void* thread_getspecific(ThreadKey& thread_key) {
    if (MELON_UNLIKELY(!thread_key.Valid())) {
        return NULL;
    }
    size_t id = thread_key._id;
    size_t seq = thread_key._seq;
    if (MELON_UNLIKELY(!g_tls_data ||
                       id >= g_tls_data->size() ||
                       (*g_tls_data)[id].seq != seq)){
        return NULL;
    }

    return (*g_tls_data)[id].data;
}

} // namespace mutil