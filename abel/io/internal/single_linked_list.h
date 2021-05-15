//
// Created by liyinbin on 2021/4/19.
//

#ifndef ABEL_IO_INTERNAL_SINGLE_LINKED_LIST_H_
#define ABEL_IO_INTERNAL_SINGLE_LINKED_LIST_H_


#include <cstddef>

#include <utility>

#include "abel/log/logging.h"

namespace abel {
    namespace io_internal {

        struct single_linked_list_entry {
            single_linked_list_entry *next = this;
        };

        // You saw `DoublyLinkedList`, now you see `single_linked_list`-one.
        //
        // For **really** performance sensitive path, this one can be faster than its
        // doubly-linked counterpart.
        //
        // For internal use only. DO NOT USE IT.
        //
        // TODO(bohuli): The interface does not quite match STL's, we might want to
        // align with them.
        template<class T, single_linked_list_entry T::*kEntry>
        class single_linked_list {
        public:
            class iterator;

            class const_iterator;

            // Initialize an empty list.
            constexpr single_linked_list();

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

            // `pop_back` cannot be implemented efficiently on singly-linked list.

            // Insert an element at list's head.
            constexpr void push_front(T *entry) noexcept;

            // Push an element into the list. The element is inserted at the tail of the
            // list.
            constexpr void push_back(T *entry) noexcept;

            // `erase` is not implemented due to the difficulty in implementation.

            // Move all elements from `from` to the tail of this list.
            //
            // TODO(bohuli): Support `splice(pos, from)`.
            constexpr void splice(single_linked_list &&from) noexcept;

            // Swap two lists.
            constexpr void swap(single_linked_list &other) noexcept;

            // Get size of the list.
            constexpr std::size_t size() const noexcept;

            // Test if the list is empty.
            constexpr bool empty() const noexcept;

            // Iterate through the list.
            constexpr iterator begin() noexcept;

            constexpr iterator end() noexcept;

            constexpr const_iterator begin() const noexcept;

            constexpr const_iterator end() const noexcept;
            // TODO(bohuli): `cbegin()` / `cend()`.

            // Movable.
            //
            // Moving into non-empty list is UNDEFINED.
            single_linked_list(single_linked_list &&other) noexcept;

            single_linked_list &operator=(
                    single_linked_list &&other) noexcept;  // CAUTION ABOUT LEAK!

            // Non-copyable
            single_linked_list(const single_linked_list &) = delete;

            single_linked_list &operator=(const single_linked_list &) = delete;

        private:
            // `offsetof(T, kEntry)`.
            inline static const std::uintptr_t kSinglyLinkedListEntryOffset =
                    reinterpret_cast<std::uintptr_t>(&(reinterpret_cast<T *>(0)->*kEntry));

            // Cast from `T*` to pointer to its `single_linked_list_entry` field.
            static constexpr single_linked_list_entry *node_cast(T *ptr) noexcept;

            static constexpr const single_linked_list_entry *node_cast(
                    const T *ptr) noexcept;

            // Cast back from what's returned by `node_cast` to original `T*`.
            static constexpr T *object_cast(single_linked_list_entry *entry) noexcept;

            static constexpr const T *object_cast(
                    const single_linked_list_entry *entry) noexcept;

        private:
            std::size_t size_{};
            single_linked_list_entry *next_{};
            single_linked_list_entry *tail_{};  // For implementing `push_back` efficiently.
        };

        // Mutable iterator.
        template<class T, single_linked_list_entry T::*kEntry>
        class single_linked_list<T, kEntry>::iterator {
        public:
            constexpr iterator() : current_(nullptr) {}

            constexpr iterator &operator++() noexcept {
                current_ = current_->next;
                return *this;
            }

            constexpr T *operator->() const noexcept { return object_cast(current_); }

            constexpr T &operator*() const noexcept { return *object_cast(current_); }

            constexpr bool operator!=(const iterator &iter) const noexcept {
                return current_ != iter.current_;
            }

            // TODO(bohuli): Post-increment / operator ==.
        private:
            friend class single_linked_list<T, kEntry>;

            constexpr explicit iterator(single_linked_list_entry *start) noexcept
                    : current_(start) {}

        private:
            single_linked_list_entry *current_;
        };

// Const iterator.
        template<class T, single_linked_list_entry T::*kEntry>
        class single_linked_list<T, kEntry>::const_iterator {
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

            constexpr bool operator==(const const_iterator &iter) const noexcept {
                return current_ == iter.current_;
            }

            // TODO(bohuli): Post-increment / operator ==.
        private:
            friend class single_linked_list<T, kEntry>;

            constexpr explicit const_iterator(const single_linked_list_entry *start) noexcept
                    : current_(start) {}

        private:
            const single_linked_list_entry *current_;
        };

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr inline single_linked_list<T, kEntry>::single_linked_list() = default;

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr T &single_linked_list<T, kEntry>::front() noexcept {
            DCHECK(!empty(), "Calling `front()` on empty list is undefined.");
            return *object_cast(next_);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr const T &single_linked_list<T, kEntry>::front() const noexcept {
            return const_cast<single_linked_list *>(this)->front();
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr T &single_linked_list<T, kEntry>::back() noexcept {
            DCHECK(!empty(), "Calling `back()` on empty list is undefined.");
            return *object_cast(tail_);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr const T &single_linked_list<T, kEntry>::back() const noexcept {
            return const_cast<single_linked_list *>(this)->back();
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr T *single_linked_list<T, kEntry>::pop_front() noexcept {
            DCHECK(!empty(), "Calling `pop_front()` on empty list is undefined.");
            auto rc = next_;
            next_ = next_->next;
            if (!--size_) {
                tail_ = nullptr;
                DCHECK(!next_);
            }
            return object_cast(rc);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr void single_linked_list<T, kEntry>::push_front(T *entry) noexcept {
            auto ptr = node_cast(entry);
            ptr->next = next_;
            next_ = ptr;
            if (!size_++) {
                tail_ = next_;
            }
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr void single_linked_list<T, kEntry>::push_back(T *entry) noexcept {
            auto ptr = node_cast(entry);
            ptr->next = nullptr;
            if (size_++) {
                tail_ = tail_->next = ptr;
            } else {
                next_ = tail_ = ptr;
            }
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr void single_linked_list<T, kEntry>::splice(
                single_linked_list &&from) noexcept {
            if (empty()) {
                swap(from);
                return;
            }
            if (from.empty()) {
                return;
            }
            tail_->next = from.next_;
            tail_ = from.tail_;
            size_ += from.size_;
            from.next_ = from.tail_ = nullptr;
            from.size_ = 0;
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr void single_linked_list<T, kEntry>::swap(
                single_linked_list &other) noexcept {
            std::swap(next_, other.next_);
            std::swap(tail_, other.tail_);
            std::swap(size_, other.size_);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr std::size_t single_linked_list<T, kEntry>::size() const noexcept {
            return size_;
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr bool single_linked_list<T, kEntry>::empty() const noexcept {
            DCHECK(!size_ == !next_);
            return !size_;
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr typename single_linked_list<T, kEntry>::iterator
        single_linked_list<T, kEntry>::begin() noexcept {
            return iterator(next_);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr typename single_linked_list<T, kEntry>::iterator
        single_linked_list<T, kEntry>::end() noexcept {
            return iterator(nullptr);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr typename single_linked_list<T, kEntry>::const_iterator
        single_linked_list<T, kEntry>::begin() const noexcept {
            return const_iterator(next_);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr typename single_linked_list<T, kEntry>::const_iterator
        single_linked_list<T, kEntry>::end() const noexcept {
            return const_iterator(nullptr);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        single_linked_list<T, kEntry>::single_linked_list(
                single_linked_list &&other) noexcept {
            swap(other);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        single_linked_list<T, kEntry> &single_linked_list<T, kEntry>::operator=(
                single_linked_list &&other) noexcept {
            DCHECK(empty(), "Moving into non-empty list will likely leak.");
            swap(other);
            return *this;
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr single_linked_list_entry *single_linked_list<T, kEntry>::node_cast(
                T *ptr) noexcept {
            return &(ptr->*kEntry);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr const single_linked_list_entry *single_linked_list<T, kEntry>::node_cast(
                const T *ptr) noexcept {
            return &(ptr->*kEntry);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr T *single_linked_list<T, kEntry>::object_cast(
                single_linked_list_entry *entry) noexcept {
            return reinterpret_cast<T *>(reinterpret_cast<std::uintptr_t>(entry) -
                                         kSinglyLinkedListEntryOffset);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr const T *single_linked_list<T, kEntry>::object_cast(
                const single_linked_list_entry *entry) noexcept {
            return reinterpret_cast<const T *>(reinterpret_cast<std::uintptr_t>(entry) -
                                               kSinglyLinkedListEntryOffset);
        }

        template<class T, single_linked_list_entry T::*kEntry>
        constexpr void swap(single_linked_list<T, kEntry> &left,
                            single_linked_list<T, kEntry> &right) noexcept {
            left.swap(right);
        }
    }  // namespace io_internal
}  // namespace abel

#endif  // ABEL_IO_INTERNAL_SINGLE_LINKED_LIST_H_
