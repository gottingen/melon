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



#ifndef MELON_RPC_SHARED_OBJECT_H_
#define MELON_RPC_SHARED_OBJECT_H_

#include "melon/utility/intrusive_ptr.hpp"                   // mutil::intrusive_ptr
#include "melon/utility/atomicops.h"


namespace melon {

// Inherit this class to be intrusively shared. Comparing to shared_ptr,
// intrusive_ptr saves one malloc (for shared_count) and gets better cache
// locality when the ref/deref are frequent, in the cost of lack of weak_ptr
// and worse interface.
class SharedObject {
friend void intrusive_ptr_add_ref(SharedObject*);
friend void intrusive_ptr_release(SharedObject*);

public:
    SharedObject() : _nref(0) { }
    int ref_count() const { return _nref.load(mutil::memory_order_relaxed); }
    
    // Add ref and returns the ref_count seen before added.
    // The effect is basically same as mutil::intrusive_ptr<T>(obj).detach()
    // except that the latter one does not return the seen ref_count which is
    // useful in some scenarios.
    int AddRefManually()
    { return _nref.fetch_add(1, mutil::memory_order_relaxed); }

    // Remove one ref, if the ref_count hit zero, delete this object.
    // Same as mutil::intrusive_ptr<T>(obj, false).reset(NULL)
    void RemoveRefManually() {
        if (_nref.fetch_sub(1, mutil::memory_order_release) == 1) {
            mutil::atomic_thread_fence(mutil::memory_order_acquire);
            delete this;
        }
    }

protected:
    virtual ~SharedObject() { }
private:
    mutil::atomic<int> _nref;
};

inline void intrusive_ptr_add_ref(SharedObject* obj) {
    obj->AddRefManually();
}

inline void intrusive_ptr_release(SharedObject* obj) {
    obj->RemoveRefManually();
}

} // namespace melon


#endif // MELON_RPC_SHARED_OBJECT_H_
