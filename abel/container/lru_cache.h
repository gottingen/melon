// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CONTAINER_LRU_CACHE_H_
#define ABEL_CONTAINER_LRU_CACHE_H_

#include <stdio.h>
#include <assert.h>
#include <mutex>
#include "abel/container/flat_hash_map.h"
#include "abel/status/status.h"

namespace abel {

    template<typename T1, typename T2>
    struct lru_handle {
        T1 key;
        T2 value;
        size_t charge;
        lru_handle *next;
        lru_handle *prev;
    };

    template<typename T1, typename T2>
    class handle_table {
    public:
        handle_table();

        ~handle_table();

        size_t table_size();

        lru_handle<T1, T2> *lookup(const T1 &key);

        lru_handle<T1, T2> *remove(const T1 &key);

        lru_handle<T1, T2> *insert(const T1 &key, lru_handle<T1, T2> *const handle);

    private:
        abel::flat_hash_map<T1, lru_handle<T1, T2> *> _table;
    };

    template<typename T1, typename T2>
    handle_table<T1, T2>::handle_table() {
    }

    template<typename T1, typename T2>
    handle_table<T1, T2>::~handle_table() {
    }

    template<typename T1, typename T2>
    size_t handle_table<T1, T2>::table_size() {
        return _table.size();
    }

    template<typename T1, typename T2>
    lru_handle<T1, T2> *handle_table<T1, T2>::lookup(const T1 &key) {
        if (_table.find(key) != _table.end()) {
            return _table[key];
        } else {
            return nullptr;
        }
    }

    template<typename T1, typename T2>
    lru_handle<T1, T2> *handle_table<T1, T2>::remove(const T1 &key) {
        lru_handle<T1, T2> *old = nullptr;
        if (_table.find(key) != _table.end()) {
            old = _table[key];
            _table.erase(key);
        }
        return old;
    }

    template<typename T1, typename T2>
    lru_handle<T1, T2> *handle_table<T1, T2>::insert(const T1 &key,
                                                     lru_handle<T1, T2> *const handle) {
        lru_handle<T1, T2> *old = nullptr;
        if (_table.find(key) != _table.end()) {
            old = _table[key];
            _table.erase(key);
        }
        _table.insert({key, handle});
        return old;
    }


    template<typename T1, typename T2>
    class lru_cache {
    public:
        lru_cache();

        ~lru_cache();

        size_t size();

        size_t total_charge();

        size_t capacity();

        void set_capacity(size_t capacity);

        status lookup(const T1 &key, T2 *value);

        status insert(const T1 &key, const T2 &value, size_t charge = 1);

        status remove(const T1 &key);

        status clear();

        // Just for test
        bool lru_and_handle_table_consistent();

        bool lru_as_expected(const std::vector <std::pair<T1, T2>> &expect);

    private:
        void lru_trim();

        void lru_remove(lru_handle<T1, T2> *const e);

        void lru_append(lru_handle<T1, T2> *const e);

        void lru_move_to_head(lru_handle<T1, T2> *const e);

        bool finish_erase(lru_handle<T1, T2> *const e);

        // Initialized before use.
        size_t _capacity;
        size_t _usage;
        size_t _size;

        std::mutex _mutex;

        // Dummy head of LRU list.
        // lru.prev is newest entry, lru.next is oldest entry.
        lru_handle<T1, T2> _lru_handle;

        handle_table<T1, T2> _handle_table;
    };

    template<typename T1, typename T2>
    lru_cache<T1, T2>::lru_cache()
            : _capacity(0),
              _usage(0),
              _size(0) {
        // Make empty circular linked lists.
        _lru_handle.next = &_lru_handle;
        _lru_handle.prev = &_lru_handle;
    }

    template<typename T1, typename T2>
    lru_cache<T1, T2>::~lru_cache() {
        clear();
    }

    template<typename T1, typename T2>
    size_t lru_cache<T1, T2>::size() {
        std::unique_lock lk(_mutex);
        return _size;
    }

    template<typename T1, typename T2>
    size_t lru_cache<T1, T2>::total_charge() {
        std::unique_lock lk(_mutex);
        return _usage;
    }

    template<typename T1, typename T2>
    size_t lru_cache<T1, T2>::capacity() {
        std::unique_lock lk(_mutex);
        return _capacity;
    }

    template<typename T1, typename T2>
    void lru_cache<T1, T2>::set_capacity(size_t capacity) {
        std::unique_lock lk(_mutex);
        _capacity = capacity;
        lru_trim();
    }

    template<typename T1, typename T2>
    status lru_cache<T1, T2>::lookup(const T1 &key, T2 *const value) {
        std::unique_lock lk(_mutex);
        lru_handle<T1, T2> *handle = _handle_table.lookup(key);
        if (handle != nullptr) {
            lru_move_to_head(handle);
            *value = handle->value;
        }
        return (handle == nullptr) ? status::not_found("not found") : status::ok();
    }

    template<typename T1, typename T2>
    status lru_cache<T1, T2>::insert(const T1 &key, const T2 &value, size_t charge) {
        std::unique_lock lk(_mutex);
        if (_capacity == 0) {
            return status::corruption("capacity is empty");
        } else {
            lru_handle<T1, T2> *handle = new lru_handle<T1, T2>();
            handle->key = key;
            handle->value = value;
            handle->charge = charge;
            lru_append(handle);
            _size++;
            _usage += charge;
            finish_erase(_handle_table.insert(key, handle));
            lru_trim();
        }
        return status::ok();
    }

    template<typename T1, typename T2>
    status lru_cache<T1, T2>::remove(const T1 &key) {
        std::unique_lock lk(_mutex);
        bool erased = finish_erase(_handle_table.remove(key));
        return erased ? status::ok() : status::not_found("not found");
    }

    template<typename T1, typename T2>
    status lru_cache<T1, T2>::clear() {
        std::unique_lock lk(_mutex);
        lru_handle<T1, T2> *old = nullptr;
        while (_lru_handle.next != &_lru_handle) {
            old = _lru_handle.next;
            bool erased = finish_erase(_handle_table.remove(old->key));
            if (!erased) {   // to avoid unused variable when compiled NDEBUG
                assert(erased);
            }
        }
        return status::ok();
    }

    template<typename T1, typename T2>
    bool lru_cache<T1, T2>::lru_and_handle_table_consistent() {
        size_t count = 0;
        std::unique_lock lk(_mutex);
        lru_handle<T1, T2> *handle = nullptr;
        lru_handle<T1, T2> *current = _lru_handle.prev;
        while (current != &_lru_handle) {
            handle = _handle_table.lookup(current->key);
            if (handle == nullptr || handle != current) {
                return false;
            } else {
                count++;
                current = current->prev;
            }
        }
        return count == _handle_table.table_size();
    }

    template<typename T1, typename T2>
    bool lru_cache<T1, T2>::lru_as_expected(
            const std::vector <std::pair<T1, T2>> &expect) {
        if (size() != expect.size()) {
            return false;
        } else {
            size_t idx = 0;
            lru_handle<T1, T2> *current = _lru_handle.prev;
            while (current != &_lru_handle) {
                if (current->key != expect[idx].first
                    || current->value != expect[idx].second) {
                    return false;
                } else {
                    idx++;
                    current = current->prev;
                }
            }
        }
        return true;
    }

    template<typename T1, typename T2>
    void lru_cache<T1, T2>::lru_trim() {
        lru_handle<T1, T2> *old = nullptr;
        while (_usage > _capacity && _lru_handle.next != &_lru_handle) {
            old = _lru_handle.next;
            bool erased = finish_erase(_handle_table.remove(old->key));
            if (!erased) {   // to avoid unused variable when compiled NDEBUG
                assert(erased);
            }
        }
    }

    template<typename T1, typename T2>
    void lru_cache<T1, T2>::lru_remove(lru_handle<T1, T2> *const e) {
        e->next->prev = e->prev;
        e->prev->next = e->next;
    }

    template<typename T1, typename T2>
    void lru_cache<T1, T2>::lru_append(lru_handle<T1, T2> *const e) {
        // Make "e" newest entry by inserting just before _lru_handle
        e->next = &_lru_handle;
        e->prev = _lru_handle.prev;
        e->prev->next = e;
        e->next->prev = e;
    }

    template<typename T1, typename T2>
    void lru_cache<T1, T2>::lru_move_to_head(lru_handle<T1, T2> *const e) {
        lru_remove(e);
        lru_append(e);
    }

    template<typename T1, typename T2>
    bool lru_cache<T1, T2>::finish_erase(lru_handle<T1, T2> *const e) {
        bool erased = false;
        if (e != nullptr) {
            lru_remove(e);
            _size--;
            _usage -= e->charge;
            delete e;
            erased = true;
        }
        return erased;
    }

}  // namespace abel

#endif  // ABEL_CONTAINER_LRU_CACHE_H_
