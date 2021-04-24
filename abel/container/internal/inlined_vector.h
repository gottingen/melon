// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CONTAINER_INTERNAL_INLINED_VECTOR_INTERNAL_H_
#define ABEL_CONTAINER_INTERNAL_INLINED_VECTOR_INTERNAL_H_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>

#include "abel/base/profile.h"
#include "abel/container/internal/compressed_tuple.h"
#include "abel/memory/memory.h"
#include "abel/meta/type_traits.h"
#include "abel/utility/span.h"

namespace abel {

namespace inlined_vector_internal {

template<typename Iterator>
using IsAtLeastForwardIterator = std::is_convertible<
        typename std::iterator_traits<Iterator>::iterator_category,
        std::forward_iterator_tag>;

template<typename AllocatorType,
        typename ValueType =
        typename abel::allocator_traits<AllocatorType>::value_type>
using IsMemcpyOk =
abel::conjunction<std::is_same<AllocatorType, std::allocator<ValueType>>,
        abel::is_trivially_copy_constructible<ValueType>,
        abel::is_trivially_copy_assignable<ValueType>,
        abel::is_trivially_destructible<ValueType>>;

template<typename AllocatorType, typename Pointer, typename SizeType>
void destroy_elements(AllocatorType *alloc_ptr, Pointer destroy_first,
                     SizeType destroy_size) {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;

    if (destroy_first != nullptr) {
        for (auto i = destroy_size; i != 0;) {
            --i;
            AllocatorTraits::destroy(*alloc_ptr, destroy_first + i);
        }

#if !defined(NDEBUG)
        {
            using ValueType = typename AllocatorTraits::value_type;

            // Overwrite unused memory with `0xab` so we can catch uninitialized
            // usage.
            //
            // Cast to `void*` to tell the compiler that we don't care that we might
            // be scribbling on a vtable pointer.
            void *memory_ptr = destroy_first;
            auto memory_size = destroy_size * sizeof(ValueType);
            std::memset(memory_ptr, 0xab, memory_size);
        }
#endif  // !defined(NDEBUG)
    }
}

template<typename AllocatorType, typename Pointer, typename ValueAdapter,
        typename SizeType>
void construct_elements(AllocatorType *alloc_ptr, Pointer construct_first,
                       ValueAdapter *values_ptr, SizeType construct_size) {
    for (SizeType i = 0; i < construct_size; ++i) {
        ABEL_TRY {
            values_ptr->ConstructNext(alloc_ptr, construct_first + i);
        }
        ABEL_CATCH (...) {
            inlined_vector_internal::destroy_elements(alloc_ptr, construct_first, i);
            ABEL_RETHROW;
        }
    }
}

template<typename Pointer, typename ValueAdapter, typename SizeType>
void assign_elements(Pointer assign_first, ValueAdapter *values_ptr,
                    SizeType assign_size) {
    for (SizeType i = 0; i < assign_size; ++i) {
        values_ptr->AssignNext(assign_first + i);
    }
}

template<typename AllocatorType>
struct storage_view {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;
    using Pointer = typename AllocatorTraits::pointer;
    using SizeType = typename AllocatorTraits::size_type;

    Pointer data;
    SizeType size;
    SizeType capacity;
};

template<typename AllocatorType, typename Iterator>
class iterator_value_adapter {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;
    using Pointer = typename AllocatorTraits::pointer;

  public:
    explicit iterator_value_adapter(const Iterator &it) : it_(it) {}

    void ConstructNext(AllocatorType *alloc_ptr, Pointer construct_at) {
        AllocatorTraits::construct(*alloc_ptr, construct_at, *it_);
        ++it_;
    }

    void AssignNext(Pointer assign_at) {
        *assign_at = *it_;
        ++it_;
    }

  private:
    Iterator it_;
};

template<typename AllocatorType>
class copy_value_adapter {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;
    using ValueType = typename AllocatorTraits::value_type;
    using Pointer = typename AllocatorTraits::pointer;
    using ConstPointer = typename AllocatorTraits::const_pointer;

  public:
    explicit copy_value_adapter(const ValueType &v) : ptr_(std::addressof(v)) {}

    void ConstructNext(AllocatorType *alloc_ptr, Pointer construct_at) {
        AllocatorTraits::construct(*alloc_ptr, construct_at, *ptr_);
    }

    void AssignNext(Pointer assign_at) { *assign_at = *ptr_; }

  private:
    ConstPointer ptr_;
};

template<typename AllocatorType>
class default_value_adapter {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;
    using ValueType = typename AllocatorTraits::value_type;
    using Pointer = typename AllocatorTraits::pointer;

  public:
    explicit default_value_adapter() {}

    void ConstructNext(AllocatorType *alloc_ptr, Pointer construct_at) {
        AllocatorTraits::construct(*alloc_ptr, construct_at);
    }

    void AssignNext(Pointer assign_at) { *assign_at = ValueType(); }
};

template<typename AllocatorType>
class allocation_transaction {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;
    using Pointer = typename AllocatorTraits::pointer;
    using SizeType = typename AllocatorTraits::size_type;

  public:
    explicit allocation_transaction(AllocatorType *alloc_ptr)
            : alloc_data_(*alloc_ptr, nullptr) {}

    ~allocation_transaction() {
        if (did_allocate()) {
            AllocatorTraits::deallocate(get_allocator(), get_data(), get_capacity());
        }
    }

    allocation_transaction(const allocation_transaction &) = delete;

    void operator=(const allocation_transaction &) = delete;

    AllocatorType &get_allocator() { return alloc_data_.template get<0>(); }

    Pointer &get_data() { return alloc_data_.template get<1>(); }

    SizeType &get_capacity() { return capacity_; }

    bool did_allocate() { return get_data() != nullptr; }

    Pointer allocate(SizeType capacity) {
        get_data() = AllocatorTraits::allocate(get_allocator(), capacity);
        get_capacity() = capacity;
        return get_data();
    }

    void reset() {
        get_data() = nullptr;
        get_capacity() = 0;
    }

  private:
    container_internal::compressed_tuple<AllocatorType, Pointer> alloc_data_;
    SizeType capacity_ = 0;
};

template<typename AllocatorType>
class construction_transaction {
    using AllocatorTraits = abel::allocator_traits<AllocatorType>;
    using Pointer = typename AllocatorTraits::pointer;
    using SizeType = typename AllocatorTraits::size_type;

  public:
    explicit construction_transaction(AllocatorType *alloc_ptr)
            : alloc_data_(*alloc_ptr, nullptr) {}

    ~construction_transaction() {
        if (did_construct()) {
            inlined_vector_internal::destroy_elements(std::addressof(get_allocator()),
                                                     get_data(), get_size());
        }
    }

    construction_transaction(const construction_transaction &) = delete;

    void operator=(const construction_transaction &) = delete;

    AllocatorType &get_allocator() { return alloc_data_.template get<0>(); }

    Pointer &get_data() { return alloc_data_.template get<1>(); }

    SizeType &get_size() { return size_; }

    bool did_construct() { return get_data() != nullptr; }

    template<typename ValueAdapter>
    void construct(Pointer data, ValueAdapter *values_ptr, SizeType size) {
        inlined_vector_internal::construct_elements(std::addressof(get_allocator()),
                                                   data, values_ptr, size);
        get_data() = data;
        get_size() = size;
    }

    void commit() {
        get_data() = nullptr;
        get_size() = 0;
    }

  private:
    container_internal::compressed_tuple<AllocatorType, Pointer> alloc_data_;
    SizeType size_ = 0;
};

template<typename T, size_t N, typename A>
class storage {
  public:
    using AllocatorTraits = abel::allocator_traits<A>;
    using allocator_type = typename AllocatorTraits::allocator_type;
    using value_type = typename AllocatorTraits::value_type;
    using pointer = typename AllocatorTraits::pointer;
    using const_pointer = typename AllocatorTraits::const_pointer;
    using size_type = typename AllocatorTraits::size_type;
    using difference_type = typename AllocatorTraits::difference_type;

    using reference = value_type &;
    using const_reference = const value_type &;
    using RValueReference = value_type &&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using MoveIterator = std::move_iterator<iterator>;
    using IsMemcpyOk = inlined_vector_internal::IsMemcpyOk<allocator_type>;

    using storage_view = inlined_vector_internal::storage_view<allocator_type>;

    template<typename Iterator>
    using iterator_value_adapter =
    inlined_vector_internal::iterator_value_adapter<allocator_type, Iterator>;
    using copy_value_adapter =
    inlined_vector_internal::copy_value_adapter<allocator_type>;
    using default_value_adapter =
    inlined_vector_internal::default_value_adapter<allocator_type>;

    using allocation_transaction =
    inlined_vector_internal::allocation_transaction<allocator_type>;
    using construction_transaction =
    inlined_vector_internal::construction_transaction<allocator_type>;

    static size_type next_capacity(size_type current_capacity) {
        return current_capacity * 2;
    }

    static size_type compute_capacity(size_type current_capacity,
                                     size_type requested_capacity) {
        return (std::max)(next_capacity(current_capacity), requested_capacity);
    }

    // ---------------------------------------------------------------------------
    // storage Constructors and Destructor
    // ---------------------------------------------------------------------------

    storage() : metadata_() {}

    explicit storage(const allocator_type &alloc) : metadata_(alloc, {}) {}

    ~storage() {
        pointer data = get_is_allocated() ? get_allocated_data() : get_inlined_data();
        inlined_vector_internal::destroy_elements(get_alloc_ptr(), data, get_size());
        deallocate_if_allocated();
    }

    // ---------------------------------------------------------------------------
    // storage Member Accessors
    // ---------------------------------------------------------------------------

    size_type &get_size_and_is_allocated() { return metadata_.template get<1>(); }

    const size_type &get_size_and_is_allocated() const {
        return metadata_.template get<1>();
    }

    size_type get_size() const { return get_size_and_is_allocated() >> 1; }

    bool get_is_allocated() const { return get_size_and_is_allocated() & 1; }

    pointer get_allocated_data() { return data_.allocated.allocated_data; }

    const_pointer get_allocated_data() const {
        return data_.allocated.allocated_data;
    }

    pointer get_inlined_data() {
        return reinterpret_cast<pointer>(
                std::addressof(data_.inlined.inlined_data[0]));
    }

    const_pointer get_inlined_data() const {
        return reinterpret_cast<const_pointer>(
                std::addressof(data_.inlined.inlined_data[0]));
    }

    size_type get_allocated_capacity() const {
        return data_.allocated.allocated_capacity;
    }

    size_type get_inlined_capacity() const { return static_cast<size_type>(N); }

    storage_view make_storage_view() {
        return get_is_allocated()
               ? storage_view{get_allocated_data(), get_size(),
                             get_allocated_capacity()}
               : storage_view{get_inlined_data(), get_size(), get_inlined_capacity()};
    }

    allocator_type *get_alloc_ptr() {
        return std::addressof(metadata_.template get<0>());
    }

    const allocator_type *get_alloc_ptr() const {
        return std::addressof(metadata_.template get<0>());
    }

    // ---------------------------------------------------------------------------
    // storage Member Mutators
    // ---------------------------------------------------------------------------

    template<typename ValueAdapter>
    void initialize(ValueAdapter values, size_type new_size);

    template<typename ValueAdapter>
    void assign(ValueAdapter values, size_type new_size);

    template<typename ValueAdapter>
    void resize(ValueAdapter values, size_type new_size);

    template<typename ValueAdapter>
    iterator insert(const_iterator pos, ValueAdapter values,
                    size_type insert_count);

    template<typename... Args>
    reference emplace_back(Args &&... args);

    iterator erase(const_iterator from, const_iterator to);

    void reserve(size_type requested_capacity);

    void shrink_to_fit();

    void swap(storage *other_storage_ptr);

    void set_is_allocated() {
        get_size_and_is_allocated() |= static_cast<size_type>(1);
    }

    void unset_is_allocated() {
        get_size_and_is_allocated() &= ((std::numeric_limits<size_type>::max)() - 1);
    }

    void set_size(size_type size) {
        get_size_and_is_allocated() =
                (size << 1) | static_cast<size_type>(get_is_allocated());
    }

    void set_allocated_size(size_type size) {
        get_size_and_is_allocated() = (size << 1) | static_cast<size_type>(1);
    }

    void set_inlined_size(size_type size) {
        get_size_and_is_allocated() = size << static_cast<size_type>(1);
    }

    void add_size(size_type count) {
        get_size_and_is_allocated() += count << static_cast<size_type>(1);
    }

    void subtract_size(size_type count) {
        assert(count <= get_size());

        get_size_and_is_allocated() -= count << static_cast<size_type>(1);
    }

    void set_allocated_data(pointer data, size_type capacity) {
        data_.allocated.allocated_data = data;
        data_.allocated.allocated_capacity = capacity;
    }

    void acquire_allocated_data(allocation_transaction *allocation_tx_ptr) {
        set_allocated_data(allocation_tx_ptr->get_data(),
                         allocation_tx_ptr->get_capacity());

        allocation_tx_ptr->reset();
    }

    void memcpy_from(const storage &other_storage) {
        assert(IsMemcpyOk::value || other_storage.get_is_allocated());

        get_size_and_is_allocated() = other_storage.get_size_and_is_allocated();
        data_ = other_storage.data_;
    }

    void deallocate_if_allocated() {
        if (get_is_allocated()) {
            AllocatorTraits::deallocate(*get_alloc_ptr(), get_allocated_data(),
                                        get_allocated_capacity());
        }
    }

  private:
    using Metadata =
    container_internal::compressed_tuple<allocator_type, size_type>;

    struct Allocated {
        pointer allocated_data;
        size_type allocated_capacity;
    };

    struct Inlined {
        alignas(value_type) char inlined_data[sizeof(value_type[N])];
    };

    union Data {
        Allocated allocated;
        Inlined inlined;
    };

    Metadata metadata_;
    Data data_;
};

template<typename T, size_t N, typename A>
template<typename ValueAdapter>
auto storage<T, N, A>::initialize(ValueAdapter values, size_type new_size)
-> void {
    // Only callable from constructors!
    assert(!get_is_allocated());
    assert(get_size() == 0);

    pointer construct_data;
    if (new_size > get_inlined_capacity()) {
        // Because this is only called from the `inlined_vector` constructors, it's
        // safe to take on the allocation with size `0`. If `construct_elements(...)`
        // throws, deallocation will be automatically handled by `~storage()`.
        size_type new_capacity = compute_capacity(get_inlined_capacity(), new_size);
        construct_data = AllocatorTraits::allocate(*get_alloc_ptr(), new_capacity);
        set_allocated_data(construct_data, new_capacity);
        set_is_allocated();
    } else {
        construct_data = get_inlined_data();
    }

    inlined_vector_internal::construct_elements(get_alloc_ptr(), construct_data,
                                               &values, new_size);

    // Since the initial size was guaranteed to be `0` and the allocated bit is
    // already correct for either case, *adding* `new_size` gives us the correct
    // result faster than setting it directly.
    add_size(new_size);
}

template<typename T, size_t N, typename A>
template<typename ValueAdapter>
auto storage<T, N, A>::assign(ValueAdapter values, size_type new_size) -> void {
    storage_view storage_view = make_storage_view();

    allocation_transaction allocation_tx(get_alloc_ptr());

    abel::span<value_type> assign_loop;
    abel::span<value_type> construct_loop;
    abel::span<value_type> destroy_loop;

    if (new_size > storage_view.capacity) {
        size_type new_capacity = compute_capacity(storage_view.capacity, new_size);
        construct_loop = {allocation_tx.allocate(new_capacity), new_size};
        destroy_loop = {storage_view.data, storage_view.size};
    } else if (new_size > storage_view.size) {
        assign_loop = {storage_view.data, storage_view.size};
        construct_loop = {storage_view.data + storage_view.size,
                          new_size - storage_view.size};
    } else {
        assign_loop = {storage_view.data, new_size};
        destroy_loop = {storage_view.data + new_size, storage_view.size - new_size};
    }

    inlined_vector_internal::assign_elements(assign_loop.data(), &values,
                                            assign_loop.size());

    inlined_vector_internal::construct_elements(
            get_alloc_ptr(), construct_loop.data(), &values, construct_loop.size());

    inlined_vector_internal::destroy_elements(get_alloc_ptr(), destroy_loop.data(),
                                             destroy_loop.size());

    if (allocation_tx.did_allocate()) {
        deallocate_if_allocated();
        acquire_allocated_data(&allocation_tx);
        set_is_allocated();
    }

    set_size(new_size);
}

template<typename T, size_t N, typename A>
template<typename ValueAdapter>
auto storage<T, N, A>::resize(ValueAdapter values, size_type new_size) -> void {
    storage_view storage_view = make_storage_view();

    iterator_value_adapter<MoveIterator> move_values(
            MoveIterator(storage_view.data));

    allocation_transaction allocation_tx(get_alloc_ptr());
    construction_transaction construction_tx(get_alloc_ptr());

    abel::span<value_type> construct_loop;
    abel::span<value_type> move_construct_loop;
    abel::span<value_type> destroy_loop;

    if (new_size > storage_view.capacity) {
        size_type new_capacity = compute_capacity(storage_view.capacity, new_size);
        pointer new_data = allocation_tx.allocate(new_capacity);
        construct_loop = {new_data + storage_view.size,
                          new_size - storage_view.size};
        move_construct_loop = {new_data, storage_view.size};
        destroy_loop = {storage_view.data, storage_view.size};
    } else if (new_size > storage_view.size) {
        construct_loop = {storage_view.data + storage_view.size,
                          new_size - storage_view.size};
    } else {
        destroy_loop = {storage_view.data + new_size, storage_view.size - new_size};
    }

    construction_tx.construct(construct_loop.data(), &values,
                              construct_loop.size());

    inlined_vector_internal::construct_elements(
            get_alloc_ptr(), move_construct_loop.data(), &move_values,
            move_construct_loop.size());

    inlined_vector_internal::destroy_elements(get_alloc_ptr(), destroy_loop.data(),
                                             destroy_loop.size());

    construction_tx.commit();
    if (allocation_tx.did_allocate()) {
        deallocate_if_allocated();
        acquire_allocated_data(&allocation_tx);
        set_is_allocated();
    }

    set_size(new_size);
}

template<typename T, size_t N, typename A>
template<typename ValueAdapter>
auto storage<T, N, A>::insert(const_iterator pos, ValueAdapter values,
                              size_type insert_count) -> iterator {
    storage_view storage_view = make_storage_view();

    size_type insert_index =
            std::distance(const_iterator(storage_view.data), pos);
    size_type insert_end_index = insert_index + insert_count;
    size_type new_size = storage_view.size + insert_count;

    if (new_size > storage_view.capacity) {
        allocation_transaction allocation_tx(get_alloc_ptr());
        construction_transaction construction_tx(get_alloc_ptr());
        construction_transaction move_construciton_tx(get_alloc_ptr());

        iterator_value_adapter<MoveIterator> move_values(
                MoveIterator(storage_view.data));

        size_type new_capacity = compute_capacity(storage_view.capacity, new_size);
        pointer new_data = allocation_tx.allocate(new_capacity);

        construction_tx.construct(new_data + insert_index, &values, insert_count);

        move_construciton_tx.construct(new_data, &move_values, insert_index);

        inlined_vector_internal::construct_elements(
                get_alloc_ptr(), new_data + insert_end_index, &move_values,
                storage_view.size - insert_index);

        inlined_vector_internal::destroy_elements(get_alloc_ptr(), storage_view.data,
                                                 storage_view.size);

        construction_tx.commit();
        move_construciton_tx.commit();
        deallocate_if_allocated();
        acquire_allocated_data(&allocation_tx);

        set_allocated_size(new_size);
        return iterator(new_data + insert_index);
    } else {
        size_type move_construction_destination_index =
                (std::max)(insert_end_index, storage_view.size);

        construction_transaction move_construction_tx(get_alloc_ptr());

        iterator_value_adapter<MoveIterator> move_construction_values(
                MoveIterator(storage_view.data +
                             (move_construction_destination_index - insert_count)));
        abel::span<value_type> move_construction = {
                storage_view.data + move_construction_destination_index,
                new_size - move_construction_destination_index};

        pointer move_assignment_values = storage_view.data + insert_index;
        abel::span<value_type> move_assignment = {
                storage_view.data + insert_end_index,
                move_construction_destination_index - insert_end_index};

        abel::span<value_type> insert_assignment = {move_assignment_values,
                                                    move_construction.size()};

        abel::span<value_type> insert_construction = {
                insert_assignment.data() + insert_assignment.size(),
                insert_count - insert_assignment.size()};

        move_construction_tx.construct(move_construction.data(),
                                       &move_construction_values,
                                       move_construction.size());

        for (pointer destination = move_assignment.data() + move_assignment.size(),
                     last_destination = move_assignment.data(),
                     source = move_assignment_values + move_assignment.size();;) {
            --destination;
            --source;
            if (destination < last_destination) break;
            *destination = std::move(*source);
        }

        inlined_vector_internal::assign_elements(insert_assignment.data(), &values,
                                                insert_assignment.size());

        inlined_vector_internal::construct_elements(
                get_alloc_ptr(), insert_construction.data(), &values,
                insert_construction.size());

        move_construction_tx.commit();

        add_size(insert_count);
        return iterator(storage_view.data + insert_index);
    }
}

template<typename T, size_t N, typename A>
template<typename... Args>
auto storage<T, N, A>::emplace_back(Args &&... args) -> reference {
    storage_view storage_view = make_storage_view();

    allocation_transaction allocation_tx(get_alloc_ptr());

    iterator_value_adapter<MoveIterator> move_values(
            MoveIterator(storage_view.data));

    pointer construct_data;
    if (storage_view.size == storage_view.capacity) {
        size_type new_capacity = next_capacity(storage_view.capacity);
        construct_data = allocation_tx.allocate(new_capacity);
    } else {
        construct_data = storage_view.data;
    }

    pointer last_ptr = construct_data + storage_view.size;

    AllocatorTraits::construct(*get_alloc_ptr(), last_ptr,
                               std::forward<Args>(args)...);

    if (allocation_tx.did_allocate()) {
        ABEL_TRY {
            inlined_vector_internal::construct_elements(
                    get_alloc_ptr(), allocation_tx.get_data(), &move_values,
                    storage_view.size);
        }
        ABEL_CATCH (...) {
            AllocatorTraits::destroy(*get_alloc_ptr(), last_ptr);
            ABEL_RETHROW;
        }

        inlined_vector_internal::destroy_elements(get_alloc_ptr(), storage_view.data,
                                                 storage_view.size);

        deallocate_if_allocated();
        acquire_allocated_data(&allocation_tx);
        set_is_allocated();
    }

    add_size(1);
    return *last_ptr;
}

template<typename T, size_t N, typename A>
auto storage<T, N, A>::erase(const_iterator from, const_iterator to)
-> iterator {
    storage_view storage_view = make_storage_view();

    size_type erase_size = std::distance(from, to);
    size_type erase_index =
            std::distance(const_iterator(storage_view.data), from);
    size_type erase_end_index = erase_index + erase_size;

    iterator_value_adapter<MoveIterator> move_values(
            MoveIterator(storage_view.data + erase_end_index));

    inlined_vector_internal::assign_elements(storage_view.data + erase_index,
                                            &move_values,
                                            storage_view.size - erase_end_index);

    inlined_vector_internal::destroy_elements(
            get_alloc_ptr(), storage_view.data + (storage_view.size - erase_size),
            erase_size);

    subtract_size(erase_size);
    return iterator(storage_view.data + erase_index);
}

template<typename T, size_t N, typename A>
auto storage<T, N, A>::reserve(size_type requested_capacity) -> void {
    storage_view storage_view = make_storage_view();

    if (ABEL_UNLIKELY(requested_capacity <= storage_view.capacity)) return;

    allocation_transaction allocation_tx(get_alloc_ptr());

    iterator_value_adapter<MoveIterator> move_values(
            MoveIterator(storage_view.data));

    size_type new_capacity =
            compute_capacity(storage_view.capacity, requested_capacity);
    pointer new_data = allocation_tx.allocate(new_capacity);

    inlined_vector_internal::construct_elements(get_alloc_ptr(), new_data,
                                               &move_values, storage_view.size);

    inlined_vector_internal::destroy_elements(get_alloc_ptr(), storage_view.data,
                                             storage_view.size);

    deallocate_if_allocated();
    acquire_allocated_data(&allocation_tx);
    set_is_allocated();
}

template<typename T, size_t N, typename A>
auto storage<T, N, A>::shrink_to_fit() -> void {
    // May only be called on allocated instances!
    assert(get_is_allocated());

    storage_view storage_view{get_allocated_data(), get_size(),
                             get_allocated_capacity()};

    if (ABEL_UNLIKELY(storage_view.size == storage_view.capacity)) return;

    allocation_transaction allocation_tx(get_alloc_ptr());

    iterator_value_adapter<MoveIterator> move_values(
            MoveIterator(storage_view.data));

    pointer construct_data;
    if (storage_view.size > get_inlined_capacity()) {
        size_type new_capacity = storage_view.size;
        construct_data = allocation_tx.allocate(new_capacity);
    } else {
        construct_data = get_inlined_data();
    }

    ABEL_TRY {
        inlined_vector_internal::construct_elements(get_alloc_ptr(), construct_data,
                                                   &move_values, storage_view.size);
    }
    ABEL_CATCH (...) {
        set_allocated_data(storage_view.data, storage_view.capacity);
        ABEL_RETHROW;
    }

    inlined_vector_internal::destroy_elements(get_alloc_ptr(), storage_view.data,
                                             storage_view.size);

    AllocatorTraits::deallocate(*get_alloc_ptr(), storage_view.data,
                                storage_view.capacity);

    if (allocation_tx.did_allocate()) {
        acquire_allocated_data(&allocation_tx);
    } else {
        unset_is_allocated();
    }
}

template<typename T, size_t N, typename A>
auto storage<T, N, A>::swap(storage *other_storage_ptr) -> void {
    using std::swap;
    assert(this != other_storage_ptr);

    if (get_is_allocated() && other_storage_ptr->get_is_allocated()) {
        swap(data_.allocated, other_storage_ptr->data_.allocated);
    } else if (!get_is_allocated() && !other_storage_ptr->get_is_allocated()) {
        storage *small_ptr = this;
        storage *large_ptr = other_storage_ptr;
        if (small_ptr->get_size() > large_ptr->get_size()) swap(small_ptr, large_ptr);

        for (size_type i = 0; i < small_ptr->get_size(); ++i) {
            swap(small_ptr->get_inlined_data()[i], large_ptr->get_inlined_data()[i]);
        }

        iterator_value_adapter<MoveIterator> move_values(
                MoveIterator(large_ptr->get_inlined_data() + small_ptr->get_size()));

        inlined_vector_internal::construct_elements(
                large_ptr->get_alloc_ptr(),
                small_ptr->get_inlined_data() + small_ptr->get_size(), &move_values,
                large_ptr->get_size() - small_ptr->get_size());

        inlined_vector_internal::destroy_elements(
                large_ptr->get_alloc_ptr(),
                large_ptr->get_inlined_data() + small_ptr->get_size(),
                large_ptr->get_size() - small_ptr->get_size());
    } else {
        storage *allocated_ptr = this;
        storage *inlined_ptr = other_storage_ptr;
        if (!allocated_ptr->get_is_allocated()) swap(allocated_ptr, inlined_ptr);

        storage_view allocated_storage_view{allocated_ptr->get_allocated_data(),
                                           allocated_ptr->get_size(),
                                           allocated_ptr->get_allocated_capacity()};

        iterator_value_adapter<MoveIterator> move_values(
                MoveIterator(inlined_ptr->get_inlined_data()));

        ABEL_TRY {
            inlined_vector_internal::construct_elements(
                    inlined_ptr->get_alloc_ptr(), allocated_ptr->get_inlined_data(),
                    &move_values, inlined_ptr->get_size());
        } ABEL_CATCH (...) {
            allocated_ptr->set_allocated_data(allocated_storage_view.data,
                                            allocated_storage_view.capacity);
            ABEL_RETHROW;
        }

        inlined_vector_internal::destroy_elements(inlined_ptr->get_alloc_ptr(),
                                                 inlined_ptr->get_inlined_data(),
                                                 inlined_ptr->get_size());

        inlined_ptr->set_allocated_data(allocated_storage_view.data,
                                      allocated_storage_view.capacity);
    }

    swap(get_size_and_is_allocated(), other_storage_ptr->get_size_and_is_allocated());
    swap(*get_alloc_ptr(), *other_storage_ptr->get_alloc_ptr());
}

}  // namespace inlined_vector_internal

}  // namespace abel

#endif  // ABEL_CONTAINER_INTERNAL_INLINED_VECTOR_INTERNAL_H_
