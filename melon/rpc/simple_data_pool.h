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



#ifndef MELON_RPC_SIMPLE_DATA_POOL_H_
#define MELON_RPC_SIMPLE_DATA_POOL_H_

#include <melon/base/scoped_lock.h>
#include <melon/rpc/data_factory.h>
#include <melon/utility/atomicops.h>


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
