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



#include <melon/rpc/simple_data_pool.h>

namespace melon {
    
    SimpleDataPool::SimpleDataPool(const DataFactory* factory)
        : _capacity(0)
        , _size(0)
        , _ncreated(0)
        , _pool(nullptr)
        , _factory(factory) {
    }
    
    SimpleDataPool::~SimpleDataPool() {
        Reset(nullptr);
    }
    
    void SimpleDataPool::Reset(const DataFactory* factory) {
        unsigned saved_size = 0;
        void** saved_pool = nullptr;
        const DataFactory* saved_factory = nullptr;
        {
            MELON_SCOPED_LOCK(_mutex);
            saved_size = _size;
            saved_pool = _pool;
            saved_factory = _factory;
            _capacity = 0;
            _size = 0;
            _ncreated.store(0, std::memory_order_relaxed);
            _pool = nullptr;
            _factory = factory;
        }
        if (saved_pool) {
            if (saved_factory) {
                for (unsigned i = 0; i < saved_size; ++i) {
                    saved_factory->DestroyData(saved_pool[i]);
                }
            }
            free(saved_pool);
        }
    }
    
    void SimpleDataPool::Reserve(unsigned n) {
        if (_capacity >= n) {
            return;
        }
        MELON_SCOPED_LOCK(_mutex);
        if (_capacity >= n) {
            return;
        }
        // Resize.
        const unsigned new_cap = std::max(_capacity * 3 / 2, n);
        void** new_pool = (void**)malloc(new_cap * sizeof(void*));
        if (nullptr == new_pool) {
            return;
        }
        if (_pool) {
            memcpy(new_pool, _pool, _capacity * sizeof(void*));
            free(_pool);
        }
        unsigned i = _capacity;
        _capacity = new_cap;
        _pool = new_pool;
    
        for (; i < n; ++i) {
            void* data = _factory->CreateData();
            if (data == nullptr) {
                break;
            }
            _ncreated.fetch_add(1,  std::memory_order_relaxed);
            _pool[_size++] = data;
        }
    }
    
    void* SimpleDataPool::Borrow() {
        if (_size) {
            MELON_SCOPED_LOCK(_mutex);
            if (_size) {
                return _pool[--_size];
            }
        }
        void* data = _factory->CreateData();
        if (data) {
            _ncreated.fetch_add(1,  std::memory_order_relaxed);
        }
        return data;
    }
    
    void SimpleDataPool::Return(void* data) {
        if (data == nullptr) {
            return;
        }
        if (!_factory->ResetData(data)) {
            return _factory->DestroyData(data); 
        }
        std::unique_lock mu(_mutex);
        if (_capacity == _size) {
            const unsigned new_cap = (_capacity <= 1 ? 128 : (_capacity * 3 / 2));
            void** new_pool = (void**)malloc(new_cap * sizeof(void*));
            if (nullptr == new_pool) {
                mu.unlock();
                return _factory->DestroyData(data);
            }
            if (_pool) {
                memcpy(new_pool, _pool, _capacity * sizeof(void*));
                free(_pool);
            }
            _capacity = new_cap;
            _pool = new_pool;
        }
        _pool[_size++] = data;
    }
    
    SimpleDataPool::Stat SimpleDataPool::stat() const {
        Stat s = { _size, _ncreated.load(std::memory_order_relaxed) };
        return s;
    }

} // namespace melon

