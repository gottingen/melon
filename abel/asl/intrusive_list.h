//
// Created by liyinbin on 2020/2/1.
//

#ifndef ABEL_ASL_INTRUSIVE_LIST_H_
#define ABEL_ASL_INTRUSIVE_LIST_H_

#include <abel/base/profile.h>
#include <iterator>
#include <algorithm>
#include <cstddef>

namespace abel {

    enum iterator_status_flag {
        isf_none = 0x00, /// This is called none and not called invalid because it is not strictly the opposite of invalid.
        isf_valid = 0x01, /// The iterator is valid, which means it is in the range of [begin, end].
        isf_current =
        0x02, /// The iterator is valid and points to the same element it did when created. For example, if an iterator points to vector::begin() but an element is inserted at the front, the iterator is valid but not current. Modification of elements in place do not make iterators non-current.
        isf_can_dereference =
        0x04  /// The iterator is dereferencable, which means it is in the range of [begin, end). It may or may not be current.
    };

    struct intrusive_list_node {
        intrusive_list_node *next;
        intrusive_list_node *prev;

        intrusive_list_node() {
            next = prev = nullptr;
        }

        ~intrusive_list_node() {
            ABEL_ASSERT_MSG(!next, "next not null");
            ABEL_ASSERT_MSG(!prev, "next not null");
        }

    };

    template<typename T, typename Pointer, typename Reference>
    class intrusive_list_iterator {
    public:
        typedef intrusive_list_iterator<T, Pointer, Reference> this_type;
        typedef intrusive_list_iterator<T, T *, T &> iterator;
        typedef intrusive_list_iterator<T, const T *, const T &> const_iterator;
        typedef T value_type;
        typedef T node_type;
        typedef ptrdiff_t difference_type;
        typedef Pointer pointer;
        typedef Reference reference;
        typedef std::bidirectional_iterator_tag iterator_category;

    public:
        pointer _node;

    public:
        intrusive_list_iterator();

        explicit intrusive_list_iterator(pointer pNode);

        intrusive_list_iterator(const iterator &x);

        reference operator*() const;

        pointer operator->() const;

        intrusive_list_iterator &operator++();

        intrusive_list_iterator &operator--();

        intrusive_list_iterator operator++(int);

        intrusive_list_iterator operator--(int);

    }; // class intrusive_list_iterator

    class intrusive_list_base {
    public:
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

    protected:
        intrusive_list_node _anchor;

    public:
        intrusive_list_base();

        ~intrusive_list_base();

        bool empty() const ABEL_NOEXCEPT;

        size_t size() const ABEL_NOEXCEPT;

        void clear() ABEL_NOEXCEPT;

        void pop_front();

        void pop_back();

        void reverse() ABEL_NOEXCEPT;

        bool validate() const;

    }; // class intrusive_list_base

    template<typename T = intrusive_list_node>
    class intrusive_list : public intrusive_list_base {
    public:
        typedef intrusive_list<T> this_type;
        typedef intrusive_list_base base_type;
        typedef T node_type;
        typedef T value_type;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef intrusive_list_iterator<T, T *, T &> iterator;
        typedef intrusive_list_iterator<T, const T *, const T &> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    public:
        intrusive_list();

        intrusive_list(const this_type &x);

        this_type &operator=(const this_type &x);

        void swap(this_type &);

        iterator begin() ABEL_NOEXCEPT;

        const_iterator begin() const ABEL_NOEXCEPT;

        const_iterator cbegin() const ABEL_NOEXCEPT;

        iterator end() ABEL_NOEXCEPT;

        const_iterator end() const ABEL_NOEXCEPT;

        const_iterator cend() const ABEL_NOEXCEPT;

        reverse_iterator rbegin() ABEL_NOEXCEPT;

        const_reverse_iterator rbegin() const ABEL_NOEXCEPT;

        const_reverse_iterator crbegin() const ABEL_NOEXCEPT;

        reverse_iterator rend() ABEL_NOEXCEPT;

        const_reverse_iterator rend() const ABEL_NOEXCEPT;

        const_reverse_iterator crend() const ABEL_NOEXCEPT;

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        void push_front(value_type &x);

        void push_back(value_type &x);

        bool contains(const value_type &x) const;

        iterator locate(value_type &x);

        const_iterator locate(const value_type &x) const;

        iterator insert(const_iterator pos, value_type &x);

        iterator erase(const_iterator pos);

        iterator erase(const_iterator pos, const_iterator last);

        reverse_iterator erase(const_reverse_iterator pos);

        reverse_iterator erase(const_reverse_iterator pos, const_reverse_iterator last);

        static void remove(value_type &value);

        void splice(const_iterator pos, value_type &x);

        void splice(const_iterator pos, intrusive_list &x);

        void splice(const_iterator pos, intrusive_list &x, const_iterator i);

        void splice(const_iterator pos, intrusive_list &x, const_iterator first, const_iterator last);

    public:

        void merge(this_type &x);

        template<typename Compare>
        void merge(this_type &x, Compare compare);

        void unique();

        template<typename BinaryPredicate>
        void unique(BinaryPredicate);

        void sort();

        template<typename Compare>
        void sort(Compare compare);

    public:
        int validate_iterator(const_iterator i) const;

    }; // intrusive_list


    template<typename T, typename Pointer, typename Reference>
    inline intrusive_list_iterator<T, Pointer, Reference>::intrusive_list_iterator()
            :_node(nullptr) {

    }

    template<typename T, typename Pointer, typename Reference>
    inline intrusive_list_iterator<T, Pointer, Reference>::intrusive_list_iterator(pointer pNode)
            : _node(pNode) {
    }

    template<typename T, typename Pointer, typename Reference>
    inline intrusive_list_iterator<T, Pointer, Reference>::intrusive_list_iterator(const iterator &x)
            : _node(x._node) {
        // Empty
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename intrusive_list_iterator<T, Pointer, Reference>::reference
    intrusive_list_iterator<T, Pointer, Reference>::operator*() const {
        return *_node;
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename intrusive_list_iterator<T, Pointer, Reference>::pointer
    intrusive_list_iterator<T, Pointer, Reference>::operator->() const {
        return _node;
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type &
    intrusive_list_iterator<T, Pointer, Reference>::operator++() {
        _node = static_cast<node_type *>(_node->next);
        return *this;
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type
    intrusive_list_iterator<T, Pointer, Reference>::operator++(int) {
        intrusive_list_iterator it(*this);
        _node = static_cast<node_type *>(_node->next);
        return it;
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type &
    intrusive_list_iterator<T, Pointer, Reference>::operator--() {
        _node = static_cast<node_type *>(_node->prev);
        return *this;
    }

    template<typename T, typename Pointer, typename Reference>
    inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type
    intrusive_list_iterator<T, Pointer, Reference>::operator--(int) {
        intrusive_list_iterator it(*this);
        _node = static_cast<node_type *>(_node->prev);
        return it;
    }

// The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
// Thus we provide additional template paremeters here to support this. The defect report does not
// require us to support comparisons between reverse_iterators and const_reverse_iterators.
    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator==(const intrusive_list_iterator<T, PointerA, ReferenceA> &a,
                           const intrusive_list_iterator<T, PointerB, ReferenceB> &b) {
        return a._node == b._node;
    }

    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator!=(const intrusive_list_iterator<T, PointerA, ReferenceA> &a,
                           const intrusive_list_iterator<T, PointerB, ReferenceB> &b) {
        return a._node != b._node;
    }

// We provide a version of operator!= for the case where the iterators are of the
// same type. This helps prevent ambiguity errors in the presence of rel_ops.
    template<typename T, typename Pointer, typename Reference>
    inline bool operator!=(const intrusive_list_iterator<T, Pointer, Reference> &a,
                           const intrusive_list_iterator<T, Pointer, Reference> &b) {
        return a._node != b._node;
    }

    inline intrusive_list_base::intrusive_list_base() {
        _anchor.next = _anchor.prev = &_anchor;
    }

    inline intrusive_list_base::~intrusive_list_base() {
        clear();
        _anchor.next = _anchor.prev = NULL;
    }

    inline bool intrusive_list_base::empty() const ABEL_NOEXCEPT {
        return _anchor.prev == &_anchor;
    }

    inline intrusive_list_base::size_type intrusive_list_base::size() const ABEL_NOEXCEPT {
        const intrusive_list_node *p = &_anchor;
        size_type n = (size_type) -1;

        do {
            ++n;
            p = p->next;
        } while (p != &_anchor);

        return n;
    }

    inline void intrusive_list_base::clear() ABEL_NOEXCEPT {

        intrusive_list_node *pNode = _anchor.next;

        while (pNode != &_anchor) {
            intrusive_list_node *const pNextNode = pNode->next;
            pNode->next = pNode->prev = NULL;
            pNode = pNextNode;
        }

        _anchor.next = _anchor.prev = &_anchor;
    }

    inline void intrusive_list_base::pop_front() {

        intrusive_list_node *const pNode = _anchor.next;

        _anchor.next->next->prev = &_anchor;
        _anchor.next = _anchor.next->next;

        if (pNode != &_anchor)
            pNode->next = pNode->prev = NULL;

    }

    inline void intrusive_list_base::pop_back() {
        intrusive_list_node *const pNode = _anchor.prev;

        _anchor.prev->prev->next = &_anchor;
        _anchor.prev = _anchor.prev->prev;

        if (pNode != &_anchor)
            pNode->next = pNode->prev = NULL;

    }

    template<typename T>
    inline intrusive_list<T>::intrusive_list() {
    }

    template<typename T>
    inline intrusive_list<T>::intrusive_list(const this_type & /*x*/)
            : intrusive_list_base() {

    }

    template<typename T>
    inline typename intrusive_list<T>::this_type &intrusive_list<T>::operator=(const this_type & /*x*/) {
        return *this;
    }

    template<typename T>
    inline typename intrusive_list<T>::iterator intrusive_list<T>::begin() ABEL_NOEXCEPT {
        return iterator(static_cast<T *>(_anchor.next));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_iterator intrusive_list<T>::begin() const ABEL_NOEXCEPT {
        return const_iterator(static_cast<T *>(_anchor.next));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_iterator intrusive_list<T>::cbegin() const ABEL_NOEXCEPT {
        return const_iterator(static_cast<T *>(_anchor.next));
    }

    template<typename T>
    inline typename intrusive_list<T>::iterator intrusive_list<T>::end() ABEL_NOEXCEPT {
        return iterator(static_cast<T *>(&_anchor));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_iterator intrusive_list<T>::end() const ABEL_NOEXCEPT {
        return const_iterator(static_cast<const T *>(&_anchor));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_iterator intrusive_list<T>::cend() const ABEL_NOEXCEPT {
        return const_iterator(static_cast<const T *>(&_anchor));
    }

    template<typename T>
    inline typename intrusive_list<T>::reverse_iterator intrusive_list<T>::rbegin() ABEL_NOEXCEPT {
        return reverse_iterator(iterator(static_cast<T *>(&_anchor)));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_reverse_iterator intrusive_list<T>::rbegin() const ABEL_NOEXCEPT {
        return const_reverse_iterator(const_iterator(static_cast<const T *>(&_anchor)));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_reverse_iterator intrusive_list<T>::crbegin() const ABEL_NOEXCEPT {
        return const_reverse_iterator(const_iterator(static_cast<const T *>(&_anchor)));
    }

    template<typename T>
    inline typename intrusive_list<T>::reverse_iterator intrusive_list<T>::rend() ABEL_NOEXCEPT {
        return reverse_iterator(iterator(static_cast<T *>(_anchor.next)));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_reverse_iterator intrusive_list<T>::rend() const ABEL_NOEXCEPT {
        return const_reverse_iterator(const_iterator(static_cast<const T *>(_anchor.next)));
    }

    template<typename T>
    inline typename intrusive_list<T>::const_reverse_iterator intrusive_list<T>::crend() const ABEL_NOEXCEPT {
        return const_reverse_iterator(const_iterator(static_cast<const T *>(_anchor.next)));
    }

    template<typename T>
    inline typename intrusive_list<T>::reference intrusive_list<T>::front() {
        ABEL_ASSERT_MSG(_anchor.next == &_anchor, "intrusive_list::front(): empty list.");
        return *static_cast<T *>(_anchor.next);
    }

    template<typename T>
    inline typename intrusive_list<T>::const_reference intrusive_list<T>::front() const {
        ABEL_ASSERT_MSG(_anchor.next == &_anchor, "intrusive_list::front(): empty list.");
        return *static_cast<const T *>(_anchor.next);
    }

    template<typename T>
    inline typename intrusive_list<T>::reference intrusive_list<T>::back() {
        ABEL_ASSERT_MSG(_anchor.next == &_anchor, "intrusive_list::front(): empty list.");
        return *static_cast<T *>(_anchor.prev);
    }

    template<typename T>
    inline typename intrusive_list<T>::const_reference intrusive_list<T>::back() const {
        ABEL_ASSERT_MSG(_anchor.next == &_anchor, "intrusive_list::front(): empty list.");
        return *static_cast<const T *>(_anchor.prev);
    }

    template<typename T>
    inline void intrusive_list<T>::push_front(value_type &x) {

        ABEL_ASSERT_MSG(!x.next || !x.prev, "intrusive_list::front(): empty list.");
        x.next = _anchor.next;
        x.prev = &_anchor;
        _anchor.next = &x;
        x.next->prev = &x;
    }

    template<typename T>
    inline void intrusive_list<T>::push_back(value_type &x) {
        ABEL_ASSERT_MSG((!x.next || !x.prev), "intrusive_list::push_back(): not empty list.");
        x.prev = _anchor.prev;
        x.next = &_anchor;
        _anchor.prev = &x;
        x.prev->next = &x;
    }

    template<typename T>
    inline bool intrusive_list<T>::contains(const value_type &x) const {
        for (const intrusive_list_node *p = _anchor.next; p != &_anchor; p = p->next) {
            if (p == &x)
                return true;
        }

        return false;
    }

    template<typename T>
    inline typename intrusive_list<T>::iterator intrusive_list<T>::locate(value_type &x) {
        for (intrusive_list_node *p = (T *) _anchor.next; p != &_anchor; p = p->next) {
            if (p == &x)
                return iterator(static_cast<T *>(p));
        }

        return iterator((T *) &_anchor);
    }

    template<typename T>
    inline typename intrusive_list<T>::const_iterator intrusive_list<T>::locate(const value_type &x) const {
        for (const intrusive_list_node *p = _anchor.next; p != &_anchor; p = p->next) {
            if (p == &x)
                return const_iterator(static_cast<const T *>(p));
        }

        return const_iterator((T *) &_anchor);
    }

    template<typename T>
    inline typename intrusive_list<T>::iterator intrusive_list<T>::insert(const_iterator pos, value_type &x) {

        ABEL_ASSERT_MSG(!x.next || !x.prev, "intrusive_list::front(): empty list.");
        intrusive_list_node &next = *const_cast<node_type *>(pos._node);
        intrusive_list_node &prev = *static_cast<node_type *>(next.prev);
        prev.next = next.prev = &x;
        x.prev = &prev;
        x.next = &next;

        return iterator(&x);
    }

    template<typename T>
    inline typename intrusive_list<T>::iterator
    intrusive_list<T>::erase(const_iterator pos) {
        intrusive_list_node &prev = *static_cast<node_type *>(pos._node->prev);
        intrusive_list_node &next = *static_cast<node_type *>(pos._node->next);
        prev.next = &next;
        next.prev = &prev;

        iterator ii(const_cast<node_type *>(pos._node));
        ii._node->prev = ii._node->next = NULL;

        return iterator(static_cast<node_type *>(&next));
    }

    template<typename T>
    inline typename intrusive_list<T>::iterator
    intrusive_list<T>::erase(const_iterator first, const_iterator last) {
        intrusive_list_node &prev = *static_cast<node_type *>(first._node->prev);
        intrusive_list_node &next = *const_cast<node_type *>(last._node);

        intrusive_list_node *pCur = const_cast<node_type *>(first._node);

        while (pCur != &next) {
            intrusive_list_node *const pCurNext = pCur->next;
            pCur->prev = pCur->next = NULL;
            pCur = pCurNext;
        }

        prev.next = &next;
        next.prev = &prev;

        return iterator(const_cast<node_type *>(last._node));
    }

    template<typename T>
    inline typename intrusive_list<T>::reverse_iterator
    intrusive_list<T>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }

    template<typename T>
    inline typename intrusive_list<T>::reverse_iterator
    intrusive_list<T>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }

    template<typename T>
    void intrusive_list<T>::swap(intrusive_list &x) {
        // swap anchors
        intrusive_list_node temp(_anchor);
        _anchor = x._anchor;
        x._anchor = temp;

        // Fixup node pointers into the anchor, since the addresses of
        // the anchors must stay the same with each list.
        if (_anchor.next == &x._anchor)
            _anchor.next = _anchor.prev = &_anchor;
        else
            _anchor.next->prev = _anchor.prev->next = &_anchor;

        if (x._anchor.next == &_anchor)
            x._anchor.next = x._anchor.prev = &x._anchor;
        else
            x._anchor.next->prev = x._anchor.prev->next = &x._anchor;

        temp.prev = temp.next = NULL;
    }

    template<typename T>
    void intrusive_list<T>::splice(const_iterator pos, value_type &value) {
// Note that splice(pos, x, pos) and splice(pos+1, x, pos)
// are valid and need to be handled correctly.

        if (pos._node != &value) {
// Unlink item from old list.
            intrusive_list_node &oldNext = *value.next;
            intrusive_list_node &oldPrev = *value.prev;
            oldNext.prev = &oldPrev;
            oldPrev.next = &oldNext;

// Relink item into new list.
            intrusive_list_node &newNext = *const_cast<node_type *>(pos._node);
            intrusive_list_node &newPrev = *newNext.prev;

            newPrev.next = &value;
            newNext.prev = &value;
            value.prev = &newPrev;
            value.next = &newNext;
        }
    }

    template<typename T>
    void intrusive_list<T>::splice(const_iterator pos, intrusive_list &x) {
// Note: &x == this is prohibited, so self-insertion is not a problem.
        if (x._anchor.next != &x._anchor) {
            intrusive_list_node &next = *const_cast<node_type *>(pos._node);
            intrusive_list_node &prev = *static_cast<node_type *>(next.prev);
            intrusive_list_node &insertPrev = *static_cast<node_type *>(x._anchor.next);
            intrusive_list_node &insertNext = *static_cast<node_type *>(x._anchor.prev);

            prev.next = &insertPrev;
            insertPrev.prev = &prev;
            insertNext.next = &next;
            next.prev = &insertNext;
            x._anchor.prev = x._anchor.next = &x._anchor;
        }
    }

    template<typename T>
    void intrusive_list<T>::splice(const_iterator pos, intrusive_list & /*x*/, const_iterator i) {

        iterator ii(const_cast<node_type *>(i._node)); // Make a temporary non-const version.

        if (pos != ii) {
// Unlink item from old list.
            intrusive_list_node &oldNext = *ii._node->next;
            intrusive_list_node &oldPrev = *ii._node->prev;
            oldNext.prev = &oldPrev;
            oldPrev.next = &oldNext;

// Relink item into new list.
            intrusive_list_node &newNext = *const_cast<node_type *>(pos._node);
            intrusive_list_node &newPrev = *newNext.prev;

            newPrev.next = ii._node;
            newNext.prev = ii._node;
            ii._node->prev = &newPrev;
            ii._node->next = &newNext;
        }
    }

    template<typename T>
    void
    intrusive_list<T>::splice(const_iterator pos, intrusive_list & /*x*/, const_iterator first, const_iterator last) {
// Note: &x == this is prohibited, so self-insertion is not a problem.
        if (first != last) {
            intrusive_list_node &insertPrev = *const_cast<node_type *>(first._node);
            intrusive_list_node &insertNext = *static_cast<node_type *>(last._node->prev);

// remove from old list
            insertNext.next->prev = insertPrev.prev;
            insertPrev.prev->next = insertNext.next;

// insert into this list
            intrusive_list_node &next = *const_cast<node_type *>(pos._node);
            intrusive_list_node &prev = *static_cast<node_type *>(next.prev);

            prev.next = &insertPrev;
            insertPrev.prev = &prev;
            insertNext.next = &next;
            next.prev = &insertNext;
        }
    }

    template<typename T>
    inline void intrusive_list<T>::remove(value_type &value) {
        intrusive_list_node &prev = *value.prev;
        intrusive_list_node &next = *value.next;
        prev.next = &next;
        next.prev = &prev;

        value.prev = value.next = NULL;

    }

    template<typename T>
    void intrusive_list<T>::merge(this_type &x) {
        if (this != &x) {
            iterator first(begin());
            iterator firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while ((first != last) && (firstX != lastX)) {
                if (*firstX < *first) {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                } else
                    ++first;
            }

            if (firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }

    template<typename T>
    template<typename Compare>
    void intrusive_list<T>::merge(this_type &x, Compare compare) {
        if (this != &x) {
            iterator first(begin());
            iterator firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while ((first != last) && (firstX != lastX)) {
                if (compare(*firstX, *first)) {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                } else {
                    ++first;
                }
            }

            if (firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }

    template<typename T>
    void intrusive_list<T>::unique() {
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (*first == *next)
                    erase(next);
                else
                    first = next;
                next = first;
            }
        }
    }

    template<typename T>
    template<typename BinaryPredicate>
    void intrusive_list<T>::unique(BinaryPredicate predicate) {
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (predicate(*first, *next))
                    erase(next);
                else
                    first = next;
                next = first;
            }
        }
    }

    template<typename T>
    void intrusive_list<T>::sort() {

        if ((static_cast<node_type *>(_anchor.next) != &_anchor) &&
            (static_cast<node_type *>(_anchor.next) != static_cast<node_type *>(_anchor.prev))) {
            // Split the array into 2 roughly equal halves.
            this_type leftList;     // This should cause no memory allocation.
            this_type rightList;

            iterator mid(begin()), tail(end());

            while ((mid != tail) && (++mid != tail))
                --tail;

            // Move the left half of this into leftList and the right half into rightList.
            leftList.splice(leftList.begin(), *this, begin(), mid);
            rightList.splice(rightList.begin(), *this);

            // Sort the sub-lists.
            leftList.sort();
            rightList.sort();

            // Merge the two halves into this list.
            splice(begin(), leftList);
            merge(rightList);
        }
    }

    template<typename T>
    template<typename Compare>
    void intrusive_list<T>::sort(Compare compare) {
        // We implement the algorithm employed by Chris Caulfield whereby we use recursive
        // function calls to sort the list. The sorting of a very large list may fail due to stack overflow
        // if the stack is exhausted. The limit depends on the platform and the avaialble stack space.

        // Easier-to-understand version of the 'if' statement:
        // iterator i(begin());
        // if((i != end()) && (++i != end())) // If the size is >= 2 (without calling the more expensive size() function)...

        // Faster, more inlinable version of the 'if' statement:
        if ((static_cast<node_type *>(_anchor.next) != &_anchor) &&
            (static_cast<node_type *>(_anchor.next) != static_cast<node_type *>(_anchor.prev))) {
            // Split the array into 2 roughly equal halves.
            this_type leftList;     // This should cause no memory allocation.
            this_type rightList;

            iterator mid(begin()), tail(end());

            while ((mid != tail) && (++mid != tail))
                --tail;

            // Move the left half of this into leftList and the right half into rightList.
            leftList.splice(leftList.begin(), *this, begin(), mid);
            rightList.splice(rightList.begin(), *this);

            // Sort the sub-lists.
            leftList.sort(compare);
            rightList.sort(compare);

            // Merge the two halves into this list.
            splice(begin(), leftList);
            merge(rightList, compare);
        }
    }

    template<typename T>
    inline int intrusive_list<T>::validate_iterator(const_iterator i) const {
// To do: Come up with a more efficient mechanism of doing this.

        for (const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp) {
            if (temp == i)
                return (isf_valid | isf_current | isf_can_dereference);
        }

        if (i == end()) {
            return (isf_valid | isf_current);
        }

        return isf_none;
    }

    template<typename T>
    bool operator==(const intrusive_list<T> &a, const intrusive_list<T> &b) {
        // If we store an mSize member for intrusive_list, we want to take advantage of it here.
        typename intrusive_list<T>::const_iterator ia = a.begin();
        typename intrusive_list<T>::const_iterator ib = b.begin();
        typename intrusive_list<T>::const_iterator enda = a.end();
        typename intrusive_list<T>::const_iterator endb = b.end();

        while ((ia != enda) && (ib != endb) && (*ia == *ib)) {
            ++ia;
            ++ib;
        }
        return (ia == enda) && (ib == endb);
    }

    template<typename T>
    bool operator!=(const intrusive_list<T> &a, const intrusive_list<T> &b) {
        return !(a == b);
    }

    template<typename T>
    bool operator<(const intrusive_list<T> &a, const intrusive_list<T> &b) {
        return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template<typename T>
    bool operator>(const intrusive_list<T> &a, const intrusive_list<T> &b) {
        return b < a;
    }

    template<typename T>
    bool operator<=(const intrusive_list<T> &a, const intrusive_list<T> &b) {
        return !(b < a);
    }

    template<typename T>
    bool operator>=(const intrusive_list<T> &a, const intrusive_list<T> &b) {
        return !(a < b);
    }

    template<typename T>
    void swap(intrusive_list<T> &a, intrusive_list<T> &b) {
        a.swap(b);
    }

} //namespace abel


#endif //ABEL_ASL_INTRUSIVE_LIST_H_
