//
// Created by liyinbin on 2020/2/2.
//

#ifndef ABEL_CONTAINER_FIXED_CIRCULAR_BUFFER_H_
#define ABEL_CONTAINER_FIXED_CIRCULAR_BUFFER_H_


#include <abel/base/profile.h>
#include <abel/utility/utility.h>
#include <type_traits>
#include <cstddef>
#include <iterator>
#include <utility>

namespace abel {

template<typename T, size_t Capacity>
class fixed_circular_buffer {
    size_t _begin = 0;
    size_t _end = 0;
    union maybe_storage {
        T data;
        maybe_storage () noexcept { }
        ~maybe_storage () { }
    };
    maybe_storage _storage[Capacity];
private:
    static size_t mask (size_t idx) { return idx % Capacity; }
    T *obj (size_t idx) { return &_storage[mask(idx)].data; }
    const T *obj (size_t idx) const { return &_storage[mask(idx)].data; }
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of two");
    static_assert(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value,
                  "fixed_circular_buffer only supports nothrow-move value types");
    using value_type = T;
    using size_type = size_t;
    using reference = T &;
    using pointer = T *;
    using const_reference = const T &;
    using const_pointer = const T *;
    using difference_type = ssize_t;
public:
    template<typename ValueType>
    class cbiterator {
        using holder = typename std::conditional<std::is_const<ValueType>::value, const maybe_storage, maybe_storage>::type;
        holder *_start;
        size_t _idx;
    private:
        cbiterator (holder *start, size_t idx) noexcept : _start(start), _idx(idx) { }
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = ValueType;
        using difference_type = ssize_t;
        using pointer = ValueType *;
        using reference = ValueType &;
    public:
        cbiterator ();
        ValueType &operator * () const { return _start[mask(_idx)].data; }
        ValueType *operator -> () const { return &operator *(); }
        // prefix
        cbiterator &operator ++ () {
            ++_idx;
            return *this;
        }
        // postfix
        cbiterator operator ++ (int) {
            auto v = *this;
            ++_idx;
            return v;
        }
        // prefix
        cbiterator &operator -- () {
            --_idx;
            return *this;
        }
        // postfix
        cbiterator operator -- (int) {
            auto v = *this;
            --_idx;
            return v;
        }
        cbiterator operator + (difference_type n) const {
            return cbiterator {_start, _idx + n};
        }
        friend cbiterator operator + (difference_type n, cbiterator i) {
            return i + n;
        }
        cbiterator operator - (difference_type n) const {
            return cbiterator {_start, _idx - n};
        }
        cbiterator &operator += (difference_type n) {
            _idx += n;
            return *this;
        }
        cbiterator &operator -= (difference_type n) {
            _idx -= n;
            return *this;
        }
        bool operator == (const cbiterator &rhs) const {
            return _idx == rhs._idx;
        }
        bool operator != (const cbiterator &rhs) const {
            return _idx != rhs._idx;
        }
        bool operator < (const cbiterator &rhs) const {
            return ssize_t(_idx - rhs._idx) < 0;
        }
        bool operator > (const cbiterator &rhs) const {
            return ssize_t(_idx - rhs._idx) > 0;
        }
        bool operator <= (const cbiterator &rhs) const {
            return ssize_t(_idx - rhs._idx) <= 0;
        }
        bool operator >= (const cbiterator &rhs) const {
            return ssize_t(_idx - rhs._idx) >= 0;
        }
        difference_type operator - (const cbiterator &rhs) const {
            return _idx - rhs._idx;
        }
        friend class fixed_circular_buffer;
    };
public:
    using iterator = cbiterator<T>;
    using const_iterator = cbiterator<const T>;
public:
    fixed_circular_buffer () = default;
    fixed_circular_buffer (fixed_circular_buffer &&x) noexcept;
    ~fixed_circular_buffer ();
    fixed_circular_buffer &operator = (fixed_circular_buffer &&x) noexcept;
    void push_front (const T &data);
    void push_front (T &&data);
    template<typename... A>
    T &emplace_front (A &&... args);
    void push_back (const T &data);
    void push_back (T &&data);
    template<typename... A>
    T &emplace_back (A &&... args);
    T &front ();
    T &back ();
    void pop_front ();
    void pop_back ();
    bool empty () const;
    size_t size () const;
    size_t capacity () const;
    T &operator [] (size_t idx);
    void clear ();
    iterator begin () {
        return iterator(_storage, _begin);
    }
    const_iterator begin () const {
        return const_iterator(_storage, _begin);
    }
    iterator end () {
        return iterator(_storage, _end);
    }
    const_iterator end () const {
        return const_iterator(_storage, _end);
    }
    const_iterator cbegin () const {
        return const_iterator(_storage, _begin);
    }
    const_iterator cend () const {
        return const_iterator(_storage, _end);
    }
    iterator erase (iterator first, iterator last);
};

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
bool
fixed_circular_buffer<T, Capacity>::empty () const {
    return _begin == _end;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
size_t
fixed_circular_buffer<T, Capacity>::size () const {
    return _end - _begin;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
size_t
fixed_circular_buffer<T, Capacity>::capacity () const {
    return Capacity;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
fixed_circular_buffer<T,
                               Capacity>::fixed_circular_buffer (fixed_circular_buffer &&x) noexcept
    : _begin(abel::exchange(x._begin, 0)), _end(abel::exchange(x._end, 0)) {
    for (auto i = _begin; i != _end; ++i) {
        new(&_storage[i].data) T(std::move(x._storage[i].data));
    }
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
fixed_circular_buffer<T, Capacity> &
fixed_circular_buffer<T, Capacity>::operator = (fixed_circular_buffer &&x) noexcept {
    if (this != &x) {
        this->~fixed_circular_buffer();
        new(this) fixed_circular_buffer(std::move(x));
    }
    return *this;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
fixed_circular_buffer<T, Capacity>::~fixed_circular_buffer () {
    for (auto i = _begin; i != _end; ++i) {
        _storage[i].data.~T();
    }
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::push_front (const T &data) {
    new(obj(_begin - 1)) T(data);
    --_begin;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::push_front (T &&data) {
    new(obj(_begin - 1)) T(std::move(data));
    --_begin;
}

template<typename T, size_t Capacity>
template<typename... Args>
ABEL_FORCE_INLINE
T &
fixed_circular_buffer<T, Capacity>::emplace_front (Args &&... args) {
    auto p = new(obj(_begin - 1)) T(std::forward<Args>(args)...);
    --_begin;
    return *p;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::push_back (const T &data) {
    new(obj(_end)) T(data);
    ++_end;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::push_back (T &&data) {
    new(obj(_end)) T(std::move(data));
    ++_end;
}

template<typename T, size_t Capacity>
template<typename... Args>
ABEL_FORCE_INLINE
T &
fixed_circular_buffer<T, Capacity>::emplace_back (Args &&... args) {
    auto p = new(obj(_end)) T(std::forward<Args>(args)...);
    ++_end;
    return *p;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
T &
fixed_circular_buffer<T, Capacity>::front () {
    return *obj(_begin);
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
T &
fixed_circular_buffer<T, Capacity>::back () {
    return *obj(_end - 1);
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::pop_front () {
    obj(_begin)->~T();
    ++_begin;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::pop_back () {
    obj(_end - 1)->~T();
    --_end;
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
T &
fixed_circular_buffer<T, Capacity>::operator [] (size_t idx) {
    return *obj(_begin + idx);
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
typename fixed_circular_buffer<T, Capacity>::iterator
fixed_circular_buffer<T, Capacity>::erase (iterator first, iterator last) {
    static_assert(std::is_nothrow_move_assignable<T>::value, "erase() assumes move assignment does not throw");
    if (first == last) {
        return last;
    }
    // Move to the left or right depending on which would result in least amount of moves.
    // This also guarantees that iterators will be stable when removing from either front or back.
    if (std::distance(begin(), first) < std::distance(last, end())) {
        auto new_start = std::move_backward(begin(), first, last);
        auto i = begin();
        while (i < new_start) {
            *i++.~T();
        }
        _begin = new_start.idx;
        return last;
    } else {
        auto new_end = std::move(last, end(), first);
        auto i = new_end;
        auto e = end();
        while (i < e) {
            *i++.~T();
        }
        _end = new_end.idx;
        return first;
    }
}

template<typename T, size_t Capacity>
ABEL_FORCE_INLINE
void
fixed_circular_buffer<T, Capacity>::clear () {
    for (auto i = _begin; i != _end; ++i) {
        obj(i)->~T();
    }
    _begin = _end = 0;
}

} //namespace abel

#endif //ABEL_CONTAINER_FIXED_CIRCULAR_BUFFER_H_
