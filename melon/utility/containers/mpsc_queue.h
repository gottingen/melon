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


// A Multiple Producer, Single Consumer Queue.
// It allows multiple threads to enqueue, and allows one thread
// (and only one thread) to dequeue.

#ifndef MUTIL_MPSC_QUEUE_H
#define MUTIL_MPSC_QUEUE_H

#include <melon/utility/object_pool.h>
#include <melon/utility/type_traits.h>

namespace mutil {

template <typename T>
struct MELON_CACHELINE_ALIGNMENT MPSCQueueNode {
    static MPSCQueueNode* const UNCONNECTED;

    MPSCQueueNode* next{NULL};
    char data_mem[sizeof(T)]{};

};

template <typename T>
MPSCQueueNode<T>* const MPSCQueueNode<T>::UNCONNECTED = (MPSCQueueNode<T>*)(intptr_t)-1;

// Default allocator for MPSCQueueNode.
template <typename T>
class DefaultAllocator {
public:
    void* Alloc() { return malloc(sizeof(MPSCQueueNode<T>)); }
    void Free(void* p) { free(p); }
};

// Allocator using ObjectPool for MPSCQueueNode.
template <typename T>
class ObjectPoolAllocator {
public:
    void* Alloc() { return get_object<MPSCQueueNode<T>>(); }
    void Free(void* p) { return_object(p); }
};


template <typename T, typename Alloc = DefaultAllocator<T>>
class MPSCQueue {
public:
    MPSCQueue()
        : _head(NULL)
        , _cur_enqueue_node(NULL)
        , _cur_dequeue_node(NULL) {}

    ~MPSCQueue();

    // Enqueue data to the queue.
    void Enqueue(typename add_const_reference<T>::type data);
    void Enqueue(T&& data);

    // Dequeue data from the queue.
    bool Dequeue(T& data);

private:
    // Reverse the list until old_head.
    void ReverseList(MPSCQueueNode<T>* old_head);

    void EnqueueImpl(MPSCQueueNode<T>* node);
    bool DequeueImpl(T* data);

    Alloc _alloc;
    atomic<MPSCQueueNode<T>*> _head;
    atomic<MPSCQueueNode<T>*> _cur_enqueue_node;
    MPSCQueueNode<T>* _cur_dequeue_node;
};

template <typename T, typename Alloc>
MPSCQueue<T, Alloc>::~MPSCQueue() {
    while (DequeueImpl(NULL));
}

template <typename T, typename Alloc>
void MPSCQueue<T, Alloc>::Enqueue(typename add_const_reference<T>::type data) {
    auto node = (MPSCQueueNode<T>*)_alloc.Alloc();
    node->next = MPSCQueueNode<T>::UNCONNECTED;
    new ((void*)&node->data_mem) T(data);
    EnqueueImpl(node);
}

template <typename T, typename Alloc>
void MPSCQueue<T, Alloc>::Enqueue(T&& data) {
    auto node = (MPSCQueueNode<T>*)_alloc.Alloc();
    node->next = MPSCQueueNode<T>::UNCONNECTED;
    new ((void*)&node->data_mem) T(std::forward<T>(data));
    EnqueueImpl(node);
}

template <typename T, typename Alloc>
void MPSCQueue<T, Alloc>::EnqueueImpl(MPSCQueueNode<T>* node) {
    MPSCQueueNode<T>* prev = _head.exchange(node, memory_order_release);
    if (prev) {
        node->next = prev;
        return;
    }
    node->next = NULL;
    _cur_enqueue_node.store(node, memory_order_relaxed);
}

template <typename T, typename Alloc>
bool MPSCQueue<T, Alloc>::Dequeue(T& data) {
    return DequeueImpl(&data);
}

template <typename T, typename Alloc>
bool MPSCQueue<T, Alloc>::DequeueImpl(T* data) {
    MPSCQueueNode<T>* node;
    if (_cur_dequeue_node) {
        node = _cur_dequeue_node;
    } else {
        node = _cur_enqueue_node.load(memory_order_relaxed);
    }
    if (!node) {
        return false;
    }

    _cur_enqueue_node.store(NULL, memory_order_relaxed);
    if (data) {
        auto mem = (T* const)node->data_mem;
        *data = std::move(*mem);
    }
    MPSCQueueNode<T>* old_node = node;
    if (!node->next) {
        ReverseList(node);
    }
    _cur_dequeue_node = node->next;
    return_object(old_node);

    return true;
}

template <typename T, typename Alloc>
void MPSCQueue<T, Alloc>::ReverseList(MPSCQueueNode<T>* old_head) {
    // Try to set _write_head to NULL to mark that it is done.
    MPSCQueueNode<T>* new_head = old_head;
    MPSCQueueNode<T>* desired = NULL;
    if (_head.compare_exchange_strong(
        new_head, desired, memory_order_acquire)) {
        // No one added new requests.
        return;
    }
    MCHECK_NE(new_head, old_head);
    // Above acquire fence pairs release fence of exchange in Enqueue() to make
    // sure that we see all fields of requests set.

    // Someone added new requests.
    // Reverse the list until old_head.
    MPSCQueueNode<T>* tail = NULL;
    MPSCQueueNode<T>* p = new_head;
    do {
        while (p->next == MPSCQueueNode<T>::UNCONNECTED) {
            // TODO(gejun): elaborate this
            sched_yield();
        }
        MPSCQueueNode<T>* const saved_next = p->next;
        p->next = tail;
        tail = p;
        p = saved_next;
        MCHECK(p);
    } while (p != old_head);

    // Link old list with new list.
    old_head->next = tail;
}

}

#endif // MUTIL_MPSC_QUEUE_H
