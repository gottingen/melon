// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//



#ifndef MELON_RPC_SIMPLE_DATA_POOL_H_
#define MELON_RPC_SIMPLE_DATA_POOL_H_

#include "melon/utility/scoped_lock.h"
#include "melon/rpc/data_factory.h"


namespace melon {

// As the name says, this is a simple unbounded dynamic-size pool for
// reusing void* data. We're assuming that data consumes considerable
// memory and should be reused as much as possible, thus unlike the
// multi-threaded allocator caching objects thread-locally, we just
// put everything in a global list to maximize sharing. It's currently
// used by Server to reuse session-local data. 
class SimpleDataPool {
public:
    struct Stat {
        unsigned nfree;
        unsigned ncreated;
    };

    explicit SimpleDataPool(const DataFactory* factory);
    ~SimpleDataPool();
    void Reset(const DataFactory* factory);
    void Reserve(unsigned n);
    void* Borrow();
    void Return(void*);
    Stat stat() const;
    
private:
    mutil::Mutex _mutex;
    unsigned _capacity;
    unsigned _size;
    mutil::atomic<unsigned> _ncreated;
    void** _pool;
    const DataFactory* _factory;
};

} // namespace melon

#endif  // MELON_RPC_SIMPLE_DATA_POOL_H_
