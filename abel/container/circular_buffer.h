//
// Created by liyinbin on 2020/1/31.
//

#ifndef ABEL_CONTAINER_CIRCULAR_BUFFER_H_
#define ABEL_CONTAINER_CIRCULAR_BUFFER_H_

#include <abel/base/profile.h>
#include <abel/base/math.h>
#include <abel/memory/transfer.h>
#include <memory>
#include <algorithm>

namespace abel {

    template<typename T, typename Alloc = std::allocator<T>>
    class circular_buffer {
        struct impl : Alloc {
            T *storage = nullptr;
            // begin, end interpreted (mod capacity)
            size_t begin = 0;
            size_t end = 0;
            size_t capacity = 0;
        };
        impl _impl;
    public:
        using value_type = T;
        using size_type = size_t;
        using reference = T &;
        using pointer = T *;
        using const_reference = const T &;
        using const_pointer = const T *;
    public:
        circular_buffer() = default;

        circular_buffer(circular_buffer &&X) noexcept;

        circular_buffer(const circular_buffer &X) = delete;

        ~circular_buffer();

        circular_buffer &operator=(const circular_buffer &) = delete;

        circular_buffer &operator=(circular_buffer &&b) noexcept;

        void push_front(const T &data);

        void push_front(T &&data);

        template<typename... A>
        void emplace_front(A &&... args);

        void push_back(const T &data);

        void push_back(T &&data);

        template<typename... A>
        void emplace_back(A &&... args);

        T &front();

        const T &front() const;

        T &back();

        const T &back() const;

        void pop_front();

        void pop_back();

        bool empty() const;

        size_t size() const;

        size_t capacity() const;

        void reserve(size_t);

        void clear();

        T &operator[](size_t idx);

        const T &operator[](size_t idx) const;

        template<typename Func>
        void for_each(Func func);

        // access an element, may return wrong or destroyed element
        // only useful if you do not rely on data accuracy (e.g. prefetch)
        T &access_element_unsafe(size_t idx);

    private:
        void expand();

        void expand(size_t);

        void maybe_expand(size_t nr = 1);

        size_t mask(size_t idx) const;

        template<typename CB, typename ValueType>
        struct cbiterator : std::iterator<std::random_access_iterator_tag, ValueType> {
            typedef std::iterator<std::random_access_iterator_tag, ValueType> super_t;

            ValueType &operator*() const { return cb->_impl.storage[cb->mask(idx)]; }

            ValueType *operator->() const { return &cb->_impl.storage[cb->mask(idx)]; }

            // prefix
            cbiterator<CB, ValueType> &operator++() {
                idx++;
                return *this;
            }

            // postfix
            cbiterator<CB, ValueType> operator++(int unused) {
                auto v = *this;
                idx++;
                return v;
            }

            // prefix
            cbiterator<CB, ValueType> &operator--() {
                idx--;
                return *this;
            }

            // postfix
            cbiterator<CB, ValueType> operator--(int unused) {
                auto v = *this;
                idx--;
                return v;
            }

            cbiterator<CB, ValueType> operator+(typename super_t::difference_type n) const {
                return cbiterator<CB, ValueType>(cb, idx + n);
            }

            cbiterator<CB, ValueType> operator-(typename super_t::difference_type n) const {
                return cbiterator<CB, ValueType>(cb, idx - n);
            }

            cbiterator<CB, ValueType> &operator+=(typename super_t::difference_type n) {
                idx += n;
                return *this;
            }

            cbiterator<CB, ValueType> &operator-=(typename super_t::difference_type n) {
                idx -= n;
                return *this;
            }

            bool operator==(const cbiterator<CB, ValueType> &rhs) const {
                return idx == rhs.idx;
            }

            bool operator!=(const cbiterator<CB, ValueType> &rhs) const {
                return idx != rhs.idx;
            }

            bool operator<(const cbiterator<CB, ValueType> &rhs) const {
                return idx < rhs.idx;
            }

            bool operator>(const cbiterator<CB, ValueType> &rhs) const {
                return idx > rhs.idx;
            }

            bool operator>=(const cbiterator<CB, ValueType> &rhs) const {
                return idx >= rhs.idx;
            }

            bool operator<=(const cbiterator<CB, ValueType> &rhs) const {
                return idx <= rhs.idx;
            }

            typename super_t::difference_type operator-(const cbiterator<CB, ValueType> &rhs) const {
                return idx - rhs.idx;
            }

        private:
            CB *cb;
            size_t idx;

            cbiterator<CB, ValueType>(CB *b, size_t i) : cb(b), idx(i) {}

            friend class circular_buffer;
        };

        friend class iterator;

    public:
        typedef cbiterator<circular_buffer, T> iterator;
        typedef cbiterator<const circular_buffer, const T> const_iterator;

        iterator begin() {
            return iterator(this, _impl.begin);
        }

        const_iterator begin() const {
            return const_iterator(this, _impl.begin);
        }

        iterator end() {
            return iterator(this, _impl.end);
        }

        const_iterator end() const {
            return const_iterator(this, _impl.end);
        }

        const_iterator cbegin() const {
            return const_iterator(this, _impl.begin);
        }

        const_iterator cend() const {
            return const_iterator(this, _impl.end);
        }

        iterator erase(iterator first, iterator last);
    };

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    size_t
    circular_buffer<T, Alloc>::mask(size_t idx) const {
        return idx & (_impl.capacity - 1);
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    bool
    circular_buffer<T, Alloc>::empty() const {
        return _impl.begin == _impl.end;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    size_t
    circular_buffer<T, Alloc>::size() const {
        return _impl.end - _impl.begin;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    size_t
    circular_buffer<T, Alloc>::capacity() const {
        return _impl.capacity;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::reserve(size_t size) {
        if (capacity() < size) {
            // Make sure that the new capacity is a power of two.
            expand(size_t(1) << integer_log2_ceil(size));
        }
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::clear() {
        erase(begin(), end());
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    circular_buffer<T, Alloc>::circular_buffer(circular_buffer &&x) noexcept
            : _impl(std::move(x._impl)) {
        x._impl = {};
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    circular_buffer<T, Alloc> &circular_buffer<T, Alloc>::operator=(circular_buffer &&x) noexcept {
        if (this != &x) {
            this->~circular_buffer();
            new(this) circular_buffer(std::move(x));
        }
        return *this;
    }

    template<typename T, typename Alloc>
    template<typename Func>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::for_each(Func func) {
        auto s = _impl.storage;
        auto m = _impl.capacity - 1;
        for (auto i = _impl.begin; i != _impl.end; ++i) {
            func(s[i & m]);
        }
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    circular_buffer<T, Alloc>::~circular_buffer() {
        for_each([this](T &obj) {
            _impl.destroy(&obj);
        });
        _impl.deallocate(_impl.storage, _impl.capacity);
    }

    template<typename T, typename Alloc>
    void
    circular_buffer<T, Alloc>::expand() {
        expand(std::max<size_t>(_impl.capacity * 2, 1));
    }

    template<typename T, typename Alloc>
    void
    circular_buffer<T, Alloc>::expand(size_t new_cap) {
        auto new_storage = _impl.allocate(new_cap);
        auto p = new_storage;
        try {
            for_each([this, &p](T &obj) {
                transfer(_impl, &obj, p);
                p++;
            });
        } catch (...) {
            while (p != new_storage) {
                _impl.destroy(--p);
            }
            _impl.deallocate(new_storage, new_cap);
            throw;
        }
        p = new_storage;
        for_each([this, &p](T &obj) {
            transfer_undo(_impl, &obj, p++);
        });
        std::swap(_impl.storage, new_storage);
        std::swap(_impl.capacity, new_cap);
        _impl.begin = 0;
        _impl.end = p - _impl.storage;
        _impl.deallocate(new_storage, new_cap);
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::maybe_expand(size_t nr) {
        if (_impl.end - _impl.begin + nr > _impl.capacity) {
            expand();
        }
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::push_front(const T &data) {
        maybe_expand();
        auto p = &_impl.storage[mask(_impl.begin - 1)];
        _impl.construct(p, data);
        --_impl.begin;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::push_front(T &&data) {
        maybe_expand();
        auto p = &_impl.storage[mask(_impl.begin - 1)];
        _impl.construct(p, std::move(data));
        --_impl.begin;
    }

    template<typename T, typename Alloc>
    template<typename... Args>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::emplace_front(Args &&... args) {
        maybe_expand();
        auto p = &_impl.storage[mask(_impl.begin - 1)];
        _impl.construct(p, std::forward<Args>(args)...);
        --_impl.begin;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::push_back(const T &data) {
        maybe_expand();
        auto p = &_impl.storage[mask(_impl.end)];
        _impl.construct(p, data);
        ++_impl.end;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::push_back(T &&data) {
        maybe_expand();
        auto p = &_impl.storage[mask(_impl.end)];
        _impl.construct(p, std::move(data));
        ++_impl.end;
    }

    template<typename T, typename Alloc>
    template<typename... Args>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::emplace_back(Args &&... args) {
        maybe_expand();
        auto p = &_impl.storage[mask(_impl.end)];
        _impl.construct(p, std::forward<Args>(args)...);
        ++_impl.end;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    T &
    circular_buffer<T, Alloc>::front() {
        return _impl.storage[mask(_impl.begin)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    const T &
    circular_buffer<T, Alloc>::front() const {
        return _impl.storage[mask(_impl.begin)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    T &
    circular_buffer<T, Alloc>::back() {
        return _impl.storage[mask(_impl.end - 1)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    const T &
    circular_buffer<T, Alloc>::back() const {
        return _impl.storage[mask(_impl.end - 1)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::pop_front() {
        _impl.destroy(&front());
        ++_impl.begin;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    void
    circular_buffer<T, Alloc>::pop_back() {
        _impl.destroy(&back());
        --_impl.end;
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    T &
    circular_buffer<T, Alloc>::operator[](size_t idx) {
        return _impl.storage[mask(_impl.begin + idx)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    const T &
    circular_buffer<T, Alloc>::operator[](size_t idx) const {
        return _impl.storage[mask(_impl.begin + idx)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    T &
    circular_buffer<T, Alloc>::access_element_unsafe(size_t idx) {
        return _impl.storage[mask(_impl.begin + idx)];
    }

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE
    typename circular_buffer<T, Alloc>::iterator
    circular_buffer<T, Alloc>::erase(iterator first, iterator last) {
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
                _impl.destroy(&*i++);
            }
            _impl.begin = new_start.idx;
            return last;
        } else {
            auto new_end = std::move(last, end(), first);
            auto i = new_end;
            auto e = end();
            while (i < e) {
                _impl.destroy(&*i++);
            }
            _impl.end = new_end.idx;
            return first;
        }
    }

} //namespace abel

#endif //ABEL_CONTAINER_CIRCULAR_BUFFER_H_
