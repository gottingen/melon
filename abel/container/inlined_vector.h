// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CONTAINER_INLINED_VECTOR_H_
#define ABEL_CONTAINER_INLINED_VECTOR_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "abel/algorithm/algorithm.h"
#include "abel/base/throw_delegate.h"
#include "abel/base/profile.h"
#include "abel/container/internal/inlined_vector.h"
#include "abel/memory/memory.h"

namespace abel {

// -----------------------------------------------------------------------------
// inlined_vector
// -----------------------------------------------------------------------------
//
// An `abel::inlined_vector` is designed to be a drop-in replacement for
// `std::vector` for use cases where the vector's size is sufficiently small
// that it can be inlined. If the inlined vector does grow beyond its estimated
// capacity, it will trigger an initial allocation on the heap, and will behave
// as a `std:vector`. The API of the `abel::inlined_vector` within this file is
// designed to cover the same API footprint as covered by `std::vector`.
template<typename T, size_t N, typename A = std::allocator<T>>
class inlined_vector {
    static_assert(N > 0, "`abel::inlined_vector` requires an inlined capacity.");

    using Storage = inlined_vector_internal::storage<T, N, A>;

    using AllocatorTraits = typename Storage::AllocatorTraits;
    using RValueReference = typename Storage::RValueReference;
    using MoveIterator = typename Storage::MoveIterator;
    using IsMemcpyOk = typename Storage::IsMemcpyOk;

    template<typename Iterator>
    using iterator_value_adapter =
    typename Storage::template iterator_value_adapter<Iterator>;
    using copy_value_adapter = typename Storage::copy_value_adapter;
    using default_value_adapter = typename Storage::default_value_adapter;

    template<typename Iterator>
    using EnableIfAtLeastForwardIterator = abel::enable_if_t<
            inlined_vector_internal::IsAtLeastForwardIterator<Iterator>::value>;
    template<typename Iterator>
    using DisableIfAtLeastForwardIterator = abel::enable_if_t<
            !inlined_vector_internal::IsAtLeastForwardIterator<Iterator>::value>;

  public:
    using allocator_type = typename Storage::allocator_type;
    using value_type = typename Storage::value_type;
    using pointer = typename Storage::pointer;
    using const_pointer = typename Storage::const_pointer;
    using size_type = typename Storage::size_type;
    using difference_type = typename Storage::difference_type;
    using reference = typename Storage::reference;
    using const_reference = typename Storage::const_reference;
    using iterator = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;
    using reverse_iterator = typename Storage::reverse_iterator;
    using const_reverse_iterator = typename Storage::const_reverse_iterator;

    // ---------------------------------------------------------------------------
    // inlined_vector Constructors and Destructor
    // ---------------------------------------------------------------------------

    // Creates an empty inlined vector with a value-initialized allocator.
    inlined_vector() noexcept(noexcept(allocator_type())): storage_() {}

    // Creates an empty inlined vector with a copy of `alloc`.
    explicit inlined_vector(const allocator_type &alloc) noexcept
            : storage_(alloc) {}

    // Creates an inlined vector with `n` copies of `value_type()`.
    explicit inlined_vector(size_type n,
                           const allocator_type &alloc = allocator_type())
            : storage_(alloc) {
        storage_.initialize(default_value_adapter(), n);
    }

    // Creates an inlined vector with `n` copies of `v`.
    inlined_vector(size_type n, const_reference v,
                  const allocator_type &alloc = allocator_type())
            : storage_(alloc) {
        storage_.initialize(copy_value_adapter(v), n);
    }

    // Creates an inlined vector with copies of the elements of `list`.
    inlined_vector(std::initializer_list<value_type> list,
                  const allocator_type &alloc = allocator_type())
            : inlined_vector(list.begin(), list.end(), alloc) {}

    // Creates an inlined vector with elements constructed from the provided
    // forward iterator range [`first`, `last`).
    //
    // NOTE: the `enable_if` prevents ambiguous interpretation between a call to
    // this constructor with two integral arguments and a call to the above
    // `inlined_vector(size_type, const_reference)` constructor.
    template<typename ForwardIterator,
            EnableIfAtLeastForwardIterator<ForwardIterator> * = nullptr>
    inlined_vector(ForwardIterator first, ForwardIterator last,
                  const allocator_type &alloc = allocator_type())
            : storage_(alloc) {
        storage_.initialize(iterator_value_adapter<ForwardIterator>(first),
                            std::distance(first, last));
    }

    // Creates an inlined vector with elements constructed from the provided input
    // iterator range [`first`, `last`).
    template<typename InputIterator,
            DisableIfAtLeastForwardIterator<InputIterator> * = nullptr>
    inlined_vector(InputIterator first, InputIterator last,
                  const allocator_type &alloc = allocator_type())
            : storage_(alloc) {
        std::copy(first, last, std::back_inserter(*this));
    }

    // Creates an inlined vector by copying the contents of `other` using
    // `other`'s allocator.
    inlined_vector(const inlined_vector &other)
            : inlined_vector(other, *other.storage_.get_alloc_ptr()) {}

    // Creates an inlined vector by copying the contents of `other` using `alloc`.
    inlined_vector(const inlined_vector &other, const allocator_type &alloc)
            : storage_(alloc) {
        if (IsMemcpyOk::value && !other.storage_.get_is_allocated()) {
            storage_.memcpy_from(other.storage_);
        } else {
            storage_.initialize(iterator_value_adapter<const_pointer>(other.data()),
                                other.size());
        }
    }

    // Creates an inlined vector by moving in the contents of `other` without
    // allocating. If `other` contains allocated memory, the newly-created inlined
    // vector will take ownership of that memory. However, if `other` does not
    // contain allocated memory, the newly-created inlined vector will perform
    // element-wise move construction of the contents of `other`.
    //
    // NOTE: since no allocation is performed for the inlined vector in either
    // case, the `noexcept(...)` specification depends on whether moving the
    // underlying objects can throw. It is assumed assumed that...
    //  a) move constructors should only throw due to allocation failure.
    //  b) if `value_type`'s move constructor allocates, it uses the same
    //     allocation function as the inlined vector's allocator.
    // Thus, the move constructor is non-throwing if the allocator is non-throwing
    // or `value_type`'s move constructor is specified as `noexcept`.
    inlined_vector(inlined_vector &&other) noexcept(
    abel::allocator_is_nothrow<allocator_type>::value ||
    std::is_nothrow_move_constructible<value_type>::value)
            : storage_(*other.storage_.get_alloc_ptr()) {
        if (IsMemcpyOk::value) {
            storage_.memcpy_from(other.storage_);

            other.storage_.set_inlined_size(0);
        } else if (other.storage_.get_is_allocated()) {
            storage_.set_allocated_data(other.storage_.get_allocated_data(),
                                      other.storage_.get_allocated_capacity());
            storage_.set_allocated_size(other.storage_.get_size());

            other.storage_.set_inlined_size(0);
        } else {
            iterator_value_adapter<MoveIterator> other_values(
                    MoveIterator(other.storage_.get_inlined_data()));

            inlined_vector_internal::construct_elements(
                    storage_.get_alloc_ptr(), storage_.get_inlined_data(), &other_values,
                    other.storage_.get_size());

            storage_.set_inlined_size(other.storage_.get_size());
        }
    }

    // Creates an inlined vector by moving in the contents of `other` with a copy
    // of `alloc`.
    //
    // NOTE: if `other`'s allocator is not equal to `alloc`, even if `other`
    // contains allocated memory, this move constructor will still allocate. Since
    // allocation is performed, this constructor can only be `noexcept` if the
    // specified allocator is also `noexcept`.
    inlined_vector(inlined_vector &&other, const allocator_type &alloc) noexcept(
    abel::allocator_is_nothrow<allocator_type>::value)
            : storage_(alloc) {
        if (IsMemcpyOk::value) {
            storage_.memcpy_from(other.storage_);

            other.storage_.set_inlined_size(0);
        } else if ((*storage_.get_alloc_ptr() == *other.storage_.get_alloc_ptr()) &&
                   other.storage_.get_is_allocated()) {
            storage_.set_allocated_data(other.storage_.get_allocated_data(),
                                      other.storage_.get_allocated_capacity());
            storage_.set_allocated_size(other.storage_.get_size());

            other.storage_.set_inlined_size(0);
        } else {
            storage_.initialize(
                    iterator_value_adapter<MoveIterator>(MoveIterator(other.data())),
                    other.size());
        }
    }

    ~inlined_vector() {}

    // ---------------------------------------------------------------------------
    // inlined_vector Member Accessors
    // ---------------------------------------------------------------------------

    // `inlined_vector::empty()`
    //
    // Returns whether the inlined vector contains no elements.
    bool empty() const noexcept { return !size(); }

    // `inlined_vector::size()`
    //
    // Returns the number of elements in the inlined vector.
    size_type size() const noexcept { return storage_.get_size(); }

    // `inlined_vector::max_size()`
    //
    // Returns the maximum number of elements the inlined vector can hold.
    size_type max_size() const noexcept {
        // One bit of the size storage is used to indicate whether the inlined
        // vector contains allocated memory. As a result, the maximum size that the
        // inlined vector can express is half of the max for `size_type`.
        return (std::numeric_limits<size_type>::max)() / 2;
    }

    // `inlined_vector::capacity()`
    //
    // Returns the number of elements that could be stored in the inlined vector
    // without requiring a reallocation.
    //
    // NOTE: for most inlined vectors, `capacity()` should be equal to the
    // template parameter `N`. For inlined vectors which exceed this capacity,
    // they will no longer be inlined and `capacity()` will equal the capactity of
    // the allocated memory.
    size_type capacity() const noexcept {
        return storage_.get_is_allocated() ? storage_.get_allocated_capacity()
                                         : storage_.get_inlined_capacity();
    }

    // `inlined_vector::data()`
    //
    // Returns a `pointer` to the elements of the inlined vector. This pointer
    // can be used to access and modify the contained elements.
    //
    // NOTE: only elements within [`data()`, `data() + size()`) are valid.
    pointer data() noexcept {
        return storage_.get_is_allocated() ? storage_.get_allocated_data()
                                         : storage_.get_inlined_data();
    }

    // Overload of `inlined_vector::data()` that returns a `const_pointer` to the
    // elements of the inlined vector. This pointer can be used to access but not
    // modify the contained elements.
    //
    // NOTE: only elements within [`data()`, `data() + size()`) are valid.
    const_pointer data() const noexcept {
        return storage_.get_is_allocated() ? storage_.get_allocated_data()
                                         : storage_.get_inlined_data();
    }

    // `inlined_vector::operator[](...)`
    //
    // Returns a `reference` to the `i`th element of the inlined vector.
    reference operator[](size_type i) {
        assert(i < size());

        return data()[i];
    }

    // Overload of `inlined_vector::operator[](...)` that returns a
    // `const_reference` to the `i`th element of the inlined vector.
    const_reference operator[](size_type i) const {
        assert(i < size());

        return data()[i];
    }

    // `inlined_vector::at(...)`
    //
    // Returns a `reference` to the `i`th element of the inlined vector.
    //
    // NOTE: if `i` is not within the required range of `inlined_vector::at(...)`,
    // in both debug and non-debug builds, `std::out_of_range` will be thrown.
    reference at(size_type i) {
        if (ABEL_UNLIKELY(i >= size())) {
            throw_std_out_of_range(
                    "`inlined_vector::at(size_type)` failed bounds check");
        }

        return data()[i];
    }

    // Overload of `inlined_vector::at(...)` that returns a `const_reference` to
    // the `i`th element of the inlined vector.
    //
    // NOTE: if `i` is not within the required range of `inlined_vector::at(...)`,
    // in both debug and non-debug builds, `std::out_of_range` will be thrown.
    const_reference at(size_type i) const {
        if (ABEL_UNLIKELY(i >= size())) {
            throw_std_out_of_range(
                    "`inlined_vector::at(size_type) const` failed bounds check");
        }

        return data()[i];
    }

    // `inlined_vector::front()`
    //
    // Returns a `reference` to the first element of the inlined vector.
    reference front() {
        assert(!empty());

        return at(0);
    }

    // Overload of `inlined_vector::front()` that returns a `const_reference` to
    // the first element of the inlined vector.
    const_reference front() const {
        assert(!empty());

        return at(0);
    }

    // `inlined_vector::back()`
    //
    // Returns a `reference` to the last element of the inlined vector.
    reference back() {
        assert(!empty());

        return at(size() - 1);
    }

    // Overload of `inlined_vector::back()` that returns a `const_reference` to the
    // last element of the inlined vector.
    const_reference back() const {
        assert(!empty());

        return at(size() - 1);
    }

    // `inlined_vector::begin()`
    //
    // Returns an `iterator` to the beginning of the inlined vector.
    iterator begin() noexcept { return data(); }

    // Overload of `inlined_vector::begin()` that returns a `const_iterator` to
    // the beginning of the inlined vector.
    const_iterator begin() const noexcept { return data(); }

    // `inlined_vector::end()`
    //
    // Returns an `iterator` to the end of the inlined vector.
    iterator end() noexcept { return data() + size(); }

    // Overload of `inlined_vector::end()` that returns a `const_iterator` to the
    // end of the inlined vector.
    const_iterator end() const noexcept { return data() + size(); }

    // `inlined_vector::cbegin()`
    //
    // Returns a `const_iterator` to the beginning of the inlined vector.
    const_iterator cbegin() const noexcept { return begin(); }

    // `inlined_vector::cend()`
    //
    // Returns a `const_iterator` to the end of the inlined vector.
    const_iterator cend() const noexcept { return end(); }

    // `inlined_vector::rbegin()`
    //
    // Returns a `reverse_iterator` from the end of the inlined vector.
    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    // Overload of `inlined_vector::rbegin()` that returns a
    // `const_reverse_iterator` from the end of the inlined vector.
    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    // `inlined_vector::rend()`
    //
    // Returns a `reverse_iterator` from the beginning of the inlined vector.
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    // Overload of `inlined_vector::rend()` that returns a `const_reverse_iterator`
    // from the beginning of the inlined vector.
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    // `inlined_vector::crbegin()`
    //
    // Returns a `const_reverse_iterator` from the end of the inlined vector.
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }

    // `inlined_vector::crend()`
    //
    // Returns a `const_reverse_iterator` from the beginning of the inlined
    // vector.
    const_reverse_iterator crend() const noexcept { return rend(); }

    // `inlined_vector::get_allocator()`
    //
    // Returns a copy of the inlined vector's allocator.
    allocator_type get_allocator() const { return *storage_.get_alloc_ptr(); }

    // ---------------------------------------------------------------------------
    // inlined_vector Member Mutators
    // ---------------------------------------------------------------------------

    // `inlined_vector::operator=(...)`
    //
    // Replaces the elements of the inlined vector with copies of the elements of
    // `list`.
    inlined_vector &operator=(std::initializer_list<value_type> list) {
        assign(list.begin(), list.end());

        return *this;
    }

    // Overload of `inlined_vector::operator=(...)` that replaces the elements of
    // the inlined vector with copies of the elements of `other`.
    inlined_vector &operator=(const inlined_vector &other) {
        if (ABEL_LIKELY(this != std::addressof(other))) {
            const_pointer other_data = other.data();
            assign(other_data, other_data + other.size());
        }

        return *this;
    }

    // Overload of `inlined_vector::operator=(...)` that moves the elements of
    // `other` into the inlined vector.
    //
    // NOTE: as a result of calling this overload, `other` is left in a valid but
    // unspecified state.
    inlined_vector &operator=(inlined_vector &&other) {
        if (ABEL_LIKELY(this != std::addressof(other))) {
            if (IsMemcpyOk::value || other.storage_.get_is_allocated()) {
                inlined_vector_internal::destroy_elements(storage_.get_alloc_ptr(), data(),
                                                         size());
                storage_.deallocate_if_allocated();
                storage_.memcpy_from(other.storage_);

                other.storage_.set_inlined_size(0);
            } else {
                storage_.assign(iterator_value_adapter<MoveIterator>(
                        MoveIterator(other.storage_.get_inlined_data())),
                                other.size());
            }
        }

        return *this;
    }

    // `inlined_vector::assign(...)`
    //
    // Replaces the contents of the inlined vector with `n` copies of `v`.
    void assign(size_type n, const_reference v) {
        storage_.assign(copy_value_adapter(v), n);
    }

    // Overload of `inlined_vector::assign(...)` that replaces the contents of the
    // inlined vector with copies of the elements of `list`.
    void assign(std::initializer_list<value_type> list) {
        assign(list.begin(), list.end());
    }

    // Overload of `inlined_vector::assign(...)` to replace the contents of the
    // inlined vector with the range [`first`, `last`).
    //
    // NOTE: this overload is for iterators that are "forward" category or better.
    template<typename ForwardIterator,
            EnableIfAtLeastForwardIterator<ForwardIterator> * = nullptr>
    void assign(ForwardIterator first, ForwardIterator last) {
        storage_.assign(iterator_value_adapter<ForwardIterator>(first),
                        std::distance(first, last));
    }

    // Overload of `inlined_vector::assign(...)` to replace the contents of the
    // inlined vector with the range [`first`, `last`).
    //
    // NOTE: this overload is for iterators that are "input" category.
    template<typename InputIterator,
            DisableIfAtLeastForwardIterator<InputIterator> * = nullptr>
    void assign(InputIterator first, InputIterator last) {
        size_type i = 0;
        for (; i < size() && first != last; ++i, static_cast<void>(++first)) {
            at(i) = *first;
        }

        erase(data() + i, data() + size());
        std::copy(first, last, std::back_inserter(*this));
    }

    // `inlined_vector::resize(...)`
    //
    // Resizes the inlined vector to contain `n` elements.
    //
    // NOTE: if `n` is smaller than `size()`, extra elements are destroyed. If `n`
    // is larger than `size()`, new elements are value-initialized.
    void resize(size_type n) { storage_.resize(default_value_adapter(), n); }

    // Overload of `inlined_vector::resize(...)` that resizes the inlined vector to
    // contain `n` elements.
    //
    // NOTE: if `n` is smaller than `size()`, extra elements are destroyed. If `n`
    // is larger than `size()`, new elements are copied-constructed from `v`.
    void resize(size_type n, const_reference v) {
        storage_.resize(copy_value_adapter(v), n);
    }

    // `inlined_vector::insert(...)`
    //
    // Inserts a copy of `v` at `pos`, returning an `iterator` to the newly
    // inserted element.
    iterator insert(const_iterator pos, const_reference v) {
        return emplace(pos, v);
    }

    // Overload of `inlined_vector::insert(...)` that inserts `v` at `pos` using
    // move semantics, returning an `iterator` to the newly inserted element.
    iterator insert(const_iterator pos, RValueReference v) {
        return emplace(pos, std::move(v));
    }

    // Overload of `inlined_vector::insert(...)` that inserts `n` contiguous copies
    // of `v` starting at `pos`, returning an `iterator` pointing to the first of
    // the newly inserted elements.
    iterator insert(const_iterator pos, size_type n, const_reference v) {
        assert(pos >= begin());
        assert(pos <= end());

        if (ABEL_LIKELY(n != 0)) {
            value_type dealias = v;
            return storage_.insert(pos, copy_value_adapter(dealias), n);
        } else {
            return const_cast<iterator>(pos);
        }
    }

    // Overload of `inlined_vector::insert(...)` that inserts copies of the
    // elements of `list` starting at `pos`, returning an `iterator` pointing to
    // the first of the newly inserted elements.
    iterator insert(const_iterator pos, std::initializer_list<value_type> list) {
        return insert(pos, list.begin(), list.end());
    }

    // Overload of `inlined_vector::insert(...)` that inserts the range [`first`,
    // `last`) starting at `pos`, returning an `iterator` pointing to the first
    // of the newly inserted elements.
    //
    // NOTE: this overload is for iterators that are "forward" category or better.
    template<typename ForwardIterator,
            EnableIfAtLeastForwardIterator<ForwardIterator> * = nullptr>
    iterator insert(const_iterator pos, ForwardIterator first,
                    ForwardIterator last) {
        assert(pos >= begin());
        assert(pos <= end());

        if (ABEL_LIKELY(first != last)) {
            return storage_.insert(pos, iterator_value_adapter<ForwardIterator>(first),
                                   std::distance(first, last));
        } else {
            return const_cast<iterator>(pos);
        }
    }

    // Overload of `inlined_vector::insert(...)` that inserts the range [`first`,
    // `last`) starting at `pos`, returning an `iterator` pointing to the first
    // of the newly inserted elements.
    //
    // NOTE: this overload is for iterators that are "input" category.
    template<typename InputIterator,
            DisableIfAtLeastForwardIterator<InputIterator> * = nullptr>
    iterator insert(const_iterator pos, InputIterator first, InputIterator last) {
        assert(pos >= begin());
        assert(pos <= end());

        size_type index = std::distance(cbegin(), pos);
        for (size_type i = index; first != last; ++i, static_cast<void>(++first)) {
            insert(data() + i, *first);
        }

        return iterator(data() + index);
    }

    // `inlined_vector::emplace(...)`
    //
    // Constructs and inserts an element using `args...` in the inlined vector at
    // `pos`, returning an `iterator` pointing to the newly emplaced element.
    template<typename... Args>
    iterator emplace(const_iterator pos, Args &&... args) {
        assert(pos >= begin());
        assert(pos <= end());

        value_type dealias(std::forward<Args>(args)...);
        return storage_.insert(pos,
                               iterator_value_adapter<MoveIterator>(
                                       MoveIterator(std::addressof(dealias))),
                               1);
    }

    // `inlined_vector::emplace_back(...)`
    //
    // Constructs and inserts an element using `args...` in the inlined vector at
    // `end()`, returning a `reference` to the newly emplaced element.
    template<typename... Args>
    reference emplace_back(Args &&... args) {
        return storage_.emplace_back(std::forward<Args>(args)...);
    }

    // `inlined_vector::push_back(...)`
    //
    // Inserts a copy of `v` in the inlined vector at `end()`.
    void push_back(const_reference v) { static_cast<void>(emplace_back(v)); }

    // Overload of `inlined_vector::push_back(...)` for inserting `v` at `end()`
    // using move semantics.
    void push_back(RValueReference v) {
        static_cast<void>(emplace_back(std::move(v)));
    }

    // `inlined_vector::pop_back()`
    //
    // Destroys the element at `back()`, reducing the size by `1`.
    void pop_back() noexcept {
        assert(!empty());

        AllocatorTraits::destroy(*storage_.get_alloc_ptr(), data() + (size() - 1));
        storage_.subtract_size(1);
    }

    // `inlined_vector::erase(...)`
    //
    // Erases the element at `pos`, returning an `iterator` pointing to where the
    // erased element was located.
    //
    // NOTE: may return `end()`, which is not dereferencable.
    iterator erase(const_iterator pos) {
        assert(pos >= begin());
        assert(pos < end());

        return storage_.erase(pos, pos + 1);
    }

    // Overload of `inlined_vector::erase(...)` that erases every element in the
    // range [`from`, `to`), returning an `iterator` pointing to where the first
    // erased element was located.
    //
    // NOTE: may return `end()`, which is not dereferencable.
    iterator erase(const_iterator from, const_iterator to) {
        assert(from >= begin());
        assert(from <= to);
        assert(to <= end());

        if (ABEL_LIKELY(from != to)) {
            return storage_.erase(from, to);
        } else {
            return const_cast<iterator>(from);
        }
    }

    // `inlined_vector::clear()`
    //
    // Destroys all elements in the inlined vector, setting the size to `0` and
    // deallocating any held memory.
    void clear() noexcept {
        inlined_vector_internal::destroy_elements(storage_.get_alloc_ptr(), data(),
                                                 size());
        storage_.deallocate_if_allocated();

        storage_.set_inlined_size(0);
    }

    // `inlined_vector::reserve(...)`
    //
    // Ensures that there is enough room for at least `n` elements.
    void reserve(size_type n) { storage_.reserve(n); }

    // `inlined_vector::shrink_to_fit()`
    //
    // Reduces memory usage by freeing unused memory. After being called, calls to
    // `capacity()` will be equal to `max(N, size())`.
    //
    // If `size() <= N` and the inlined vector contains allocated memory, the
    // elements will all be moved to the inlined space and the allocated memory
    // will be deallocated.
    //
    // If `size() > N` and `size() < capacity()`, the elements will be moved to a
    // smaller allocation.
    void shrink_to_fit() {
        if (storage_.get_is_allocated()) {
            storage_.shrink_to_fit();
        }
    }

    // `inlined_vector::swap(...)`
    //
    // Swaps the contents of the inlined vector with `other`.
    void swap(inlined_vector &other) {
        if (ABEL_LIKELY(this != std::addressof(other))) {
            storage_.swap(std::addressof(other.storage_));
        }
    }

  private:
    template<typename H, typename TheT, size_t TheN, typename TheA>
    friend H abel_hash_value(H h, const abel::inlined_vector<TheT, TheN, TheA> &a);

    Storage storage_;
};

// -----------------------------------------------------------------------------
// inlined_vector Non-Member Functions
// -----------------------------------------------------------------------------

// `swap(...)`
//
// Swaps the contents of two inlined vectors.
template<typename T, size_t N, typename A>
void swap(abel::inlined_vector<T, N, A> &a,
          abel::inlined_vector<T, N, A> &b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

// `operator==(...)`
//
// Tests for value-equality of two inlined vectors.
template<typename T, size_t N, typename A>
bool operator==(const abel::inlined_vector<T, N, A> &a,
                const abel::inlined_vector<T, N, A> &b) {
    auto a_data = a.data();
    auto b_data = b.data();
    return abel::equal(a_data, a_data + a.size(), b_data, b_data + b.size());
}

// `operator!=(...)`
//
// Tests for value-inequality of two inlined vectors.
template<typename T, size_t N, typename A>
bool operator!=(const abel::inlined_vector<T, N, A> &a,
                const abel::inlined_vector<T, N, A> &b) {
    return !(a == b);
}

// `operator<(...)`
//
// Tests whether the value of an inlined vector is less than the value of
// another inlined vector using a lexicographical comparison algorithm.
template<typename T, size_t N, typename A>
bool operator<(const abel::inlined_vector<T, N, A> &a,
               const abel::inlined_vector<T, N, A> &b) {
    auto a_data = a.data();
    auto b_data = b.data();
    return std::lexicographical_compare(a_data, a_data + a.size(), b_data,
                                        b_data + b.size());
}

// `operator>(...)`
//
// Tests whether the value of an inlined vector is greater than the value of
// another inlined vector using a lexicographical comparison algorithm.
template<typename T, size_t N, typename A>
bool operator>(const abel::inlined_vector<T, N, A> &a,
               const abel::inlined_vector<T, N, A> &b) {
    return b < a;
}

// `operator<=(...)`
//
// Tests whether the value of an inlined vector is less than or equal to the
// value of another inlined vector using a lexicographical comparison algorithm.
template<typename T, size_t N, typename A>
bool operator<=(const abel::inlined_vector<T, N, A> &a,
                const abel::inlined_vector<T, N, A> &b) {
    return !(b < a);
}

// `operator>=(...)`
//
// Tests whether the value of an inlined vector is greater than or equal to the
// value of another inlined vector using a lexicographical comparison algorithm.
template<typename T, size_t N, typename A>
bool operator>=(const abel::inlined_vector<T, N, A> &a,
                const abel::inlined_vector<T, N, A> &b) {
    return !(a < b);
}

// `abel_hash_value(...)`
//
// Provides `abel::hash` support for `abel::inlined_vector`. It is uncommon to
// call this directly.
template<typename H, typename T, size_t N, typename A>
H abel_hash_value(H h, const abel::inlined_vector<T, N, A> &a) {
    auto size = a.size();
    return H::combine(H::combine_contiguous(std::move(h), a.data(), size), size);
}


}  // namespace abel

#endif  // ABEL_CONTAINER_INLINED_VECTOR_H_
