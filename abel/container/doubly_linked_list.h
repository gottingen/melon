// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CONTAINER_DOUBLY_LINKED_LIST_H_
#define ABEL_CONTAINER_DOUBLY_LINKED_LIST_H_

#include <cstddef>

#include <utility>

#include "abel/log/logging.h"

namespace abel {

    struct doubly_linked_list_entry {
        doubly_linked_list_entry *prev = this;
        doubly_linked_list_entry *next = this;
    };

    template<class T, doubly_linked_list_entry T::*kEntry>
    class doubly_linked_list {
    public:
        class iterator;

        class const_iterator;

        // Initialize an empty list.
        constexpr doubly_linked_list();

        // Access first element in the list.
        //
        // Precondition: `!empty()` holds.
        constexpr T &front() noexcept;

        constexpr const T &front() const noexcept;

        // Access last element in the list.
        //
        // Precondition: `!empty()` holds.
        constexpr T &back() noexcept;

        constexpr const T &back() const noexcept;

        // Pop the first element (at head) in the list, or `nullptr` if the list is
        // empty.
        constexpr T *pop_front() noexcept;

        // Pop the last element (at the tail) in the list, or `nullptr` is the list is
        // empty.
        constexpr T *pop_back() noexcept;

        // Insert an element at list's head.
        constexpr void push_front(T *entry) noexcept;

        // Push an element into the list. The element is inserted at the tail of the
        // list.
        constexpr void push_back(T *entry) noexcept;

        // After `erase`, the list node is initialized to `{this, this}`. This is
        // needed to detect if the node is not in the list.
        //
        // Returns `true` if `entry` was successfully removed, `false` if it's not in
        // the list (i.e., both pointer point to itself).
        constexpr bool erase(T *entry) noexcept;

        constexpr void splice(doubly_linked_list &&from) noexcept;

        // Swap two lists.
        constexpr void swap(doubly_linked_list &other) noexcept;

        // Get size of the list.
        constexpr std::size_t size() const noexcept;

        // Test if the list is empty.
        constexpr bool empty() const noexcept;

        // Iterate through the list.
        constexpr iterator begin() noexcept;

        constexpr iterator end() noexcept;

        constexpr const_iterator begin() const noexcept;

        constexpr const_iterator end() const noexcept;

        // Non-movable, non-copyable
        doubly_linked_list(const doubly_linked_list &) = delete;

        doubly_linked_list &operator=(const doubly_linked_list &) = delete;

    private:
        // `offsetof(T, kEntry)`.
        inline static const std::uintptr_t kDoublyLinkedListEntryOffset =
                reinterpret_cast<std::uintptr_t>(&(reinterpret_cast<T *>(0)->*kEntry));

        // Cast from `T*` to pointer to its `doubly_linked_list_entry` field.
        static constexpr doubly_linked_list_entry *node_cast(T *ptr) noexcept;

        static constexpr const doubly_linked_list_entry *node_cast(
                const T *ptr) noexcept;

        // Cast back from what's returned by `node_cast` to original `T*`.
        static constexpr T *object_cast(doubly_linked_list_entry *entry) noexcept;

        static constexpr const T *object_cast(
                const doubly_linked_list_entry *entry) noexcept;

    private:
        std::size_t size_{};
        doubly_linked_list_entry head_;
    };

// Mutable iterator.
    template<class T, doubly_linked_list_entry T::*kEntry>
    class doubly_linked_list<T, kEntry>::iterator {
    public:
        constexpr iterator() : current_(nullptr) {}

        // TODO(yibinli): Implicit conversion from `const_iterator`.
        constexpr iterator &operator++() noexcept {
            current_ = current_->next;
            return *this;
        }

        constexpr T *operator->() const noexcept { return object_cast(current_); }

        constexpr T &operator*() const noexcept { return *object_cast(current_); }

        constexpr bool operator!=(const iterator &iter) const noexcept {
            return current_ != iter.current_;
        }

        // TODO(yibinli): Post-increment / decrement / operator ==.
    private:
        friend class doubly_linked_list<T, kEntry>;

        constexpr explicit iterator(doubly_linked_list_entry *start) noexcept
                : current_(start) {}

    private:
        doubly_linked_list_entry *current_;
    };

// Const iterator.
    template<class T, doubly_linked_list_entry T::*kEntry>
    class doubly_linked_list<T, kEntry>::const_iterator {
    public:
        constexpr const_iterator() : current_(nullptr) {}

        constexpr /* implicit */ const_iterator(const iterator &from)
                : current_(from.current_) {}

        constexpr const_iterator &operator++() noexcept {
            current_ = current_->next;
            return *this;
        }

        constexpr const T *operator->() const noexcept {
            return object_cast(current_);
        }

        constexpr const T &operator*() const noexcept {
            return *object_cast(current_);
        }

        constexpr bool operator!=(const const_iterator &iter) const noexcept {
            return current_ != iter.current_;
        }

        // TODO(yibinli): Post-increment / decrement / operator ==.
    private:
        friend class doubly_linked_list<T, kEntry>;

        constexpr explicit const_iterator(const doubly_linked_list_entry *start) noexcept
                : current_(start) {}

    private:
        const doubly_linked_list_entry *current_;
    };

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr inline doubly_linked_list<T, kEntry>::doubly_linked_list() = default;

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr T &doubly_linked_list<T, kEntry>::front() noexcept {
        DCHECK_NE(head_.next, &head_);
        return *object_cast(head_.next);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr const T &doubly_linked_list<T, kEntry>::front() const noexcept {
        return const_cast<doubly_linked_list *>(this)->front();
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr T &doubly_linked_list<T, kEntry>::back() noexcept {
        DCHECK_NE(head_.prev, &head_);
        return *object_cast(head_.prev);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr const T &doubly_linked_list<T, kEntry>::back() const noexcept {
        return const_cast<doubly_linked_list *>(this)->back();
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr T *doubly_linked_list<T, kEntry>::pop_front() noexcept {
        if (ABEL_UNLIKELY(head_.prev == &head_)) {
            // Empty then.
            DCHECK_EQ(head_.prev, head_.next);
            return nullptr;
        }
        auto rc = head_.next;
        rc->prev->next = rc->next;
        rc->next->prev = rc->prev;
        rc->next = rc->prev = rc;  // Mark it as removed.
        --size_;
        return object_cast(rc);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr T *doubly_linked_list<T, kEntry>::pop_back() noexcept {
        if (ABEL_UNLIKELY(head_.prev == &head_)) {
            // Empty then.
            DCHECK_EQ(head_.prev, head_.next);
            return nullptr;
        }
        auto rc = head_.prev;
        rc->prev->next = rc->next;
        rc->next->prev = rc->prev;
        rc->next = rc->prev = rc;  // Mark it as removed.
        --size_;
        return object_cast(rc);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr void doubly_linked_list<T, kEntry>::push_front(T *entry) noexcept {
        auto ptr = node_cast(entry);
        ptr->prev = &head_;
        ptr->next = head_.next;
        // Add the node at the front.
        ptr->prev->next = ptr;
        ptr->next->prev = ptr;
        ++size_;
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr void doubly_linked_list<T, kEntry>::push_back(T *entry) noexcept {
        auto ptr = node_cast(entry);
        ptr->prev = head_.prev;
        ptr->next = &head_;
        // Add the node to the tail.
        ptr->prev->next = ptr;
        ptr->next->prev = ptr;
        ++size_;
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr bool doubly_linked_list<T, kEntry>::erase(T *entry) noexcept {
        auto ptr = node_cast(entry);
        // Ensure that we're indeed in the list.
        if (ptr->prev == ptr) {
            DCHECK_EQ(ptr->prev, ptr->next);
            return false;
        }
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
        ptr->next = ptr->prev = ptr;
        --size_;
        return true;
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr void doubly_linked_list<T, kEntry>::splice(
            doubly_linked_list &&from) noexcept {
        if (from.empty()) {
            return;
        }
        auto &&other_front = from.head_.next;
        auto &&other_back = from.head_.prev;
        // Link `from`'s head with our tail.
        other_front->prev = head_.prev;
        head_.prev->next = other_front;
        // Link `from`'s tail with `head_`.
        other_back->next = &head_;
        head_.prev = other_back;
        // Update size.
        size_ += std::exchange(from.size_, 0);
        // Clear `from`.
        from.head_.prev = from.head_.next = &from.head_;
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr void doubly_linked_list<T, kEntry>::swap(
            doubly_linked_list &other) noexcept {
        bool e1 = empty(), e2 = other.empty();
        std::swap(head_, other.head_);
        std::swap(size_, other.size_);
        if (!e2) {
            head_.prev->next = &head_;
            head_.next->prev = &head_;
        } else {
            head_.prev = head_.next = &head_;
        }
        if (!e1) {
            other.head_.prev->next = &other.head_;
            other.head_.next->prev = &other.head_;
        } else {
            other.head_.prev = other.head_.next = &other.head_;
        }
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr std::size_t doubly_linked_list<T, kEntry>::size() const noexcept {
        return size_;
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr bool doubly_linked_list<T, kEntry>::empty() const noexcept {
        DCHECK(!size_ == (head_.prev == &head_));
        return !size_;
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr typename doubly_linked_list<T, kEntry>::iterator
    doubly_linked_list<T, kEntry>::begin() noexcept {
        return iterator(head_.next);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr typename doubly_linked_list<T, kEntry>::iterator
    doubly_linked_list<T, kEntry>::end() noexcept {
        return iterator(&head_);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr typename doubly_linked_list<T, kEntry>::const_iterator
    doubly_linked_list<T, kEntry>::begin() const noexcept {
        return const_iterator(head_.next);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr typename doubly_linked_list<T, kEntry>::const_iterator
    doubly_linked_list<T, kEntry>::end() const noexcept {
        return const_iterator(&head_);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr doubly_linked_list_entry *doubly_linked_list<T, kEntry>::node_cast(
            T *ptr) noexcept {
        return &(ptr->*kEntry);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr const doubly_linked_list_entry *doubly_linked_list<T, kEntry>::node_cast(
            const T *ptr) noexcept {
        return &(ptr->*kEntry);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr T *doubly_linked_list<T, kEntry>::object_cast(
            doubly_linked_list_entry *entry) noexcept {
        return reinterpret_cast<T *>(reinterpret_cast<std::uintptr_t>(entry) -
                                     kDoublyLinkedListEntryOffset);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr const T *doubly_linked_list<T, kEntry>::object_cast(
            const doubly_linked_list_entry *entry) noexcept {
        return reinterpret_cast<const T *>(reinterpret_cast<std::uintptr_t>(entry) -
                                           kDoublyLinkedListEntryOffset);
    }

    template<class T, doubly_linked_list_entry T::*kEntry>
    constexpr void swap(doubly_linked_list<T, kEntry> &left,
                        doubly_linked_list<T, kEntry> &right) noexcept {
        left.swap(right);
    }

}  // namespace abel

#endif  // ABEL_CONTAINER_DOUBLY_LINKED_LIST_H_
