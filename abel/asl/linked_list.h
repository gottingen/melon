//
// Created by liyinbin on 2020/3/31.
//

#ifndef ABEL_ASL_LINKED_LIST_H_
#define ABEL_ASL_LINKED_LIST_H_

#include <abel/base/profile.h>

namespace abel {


    template<typename T>
    class linked_node {
    public:
        // linked_node are self-referential as default.
        linked_node() : previous_(this), next_(this) {}

        linked_node(linked_node<T> *previous, linked_node<T> *next)
                : previous_(previous), next_(next) {}

        // Insert |this| into the linked list, before |e|.
        void insert_before(linked_node<T> *e) {
            this->next_ = e;
            this->previous_ = e->previous_;
            e->previous_->next_ = this;
            e->previous_ = this;
        }

        // Insert |this| as a circular linked list into the linked list, before |e|.
        void insert_before_as_list(linked_node<T> *e) {
            linked_node<T> *prev = this->previous_;
            prev->next_ = e;
            this->previous_ = e->previous_;
            e->previous_->next_ = this;
            e->previous_ = prev;
        }

        // Insert |this| into the linked list, after |e|.
        void insert_after(linked_node<T> *e) {
            this->next_ = e->next_;
            this->previous_ = e;
            e->next_->previous_ = this;
            e->next_ = this;
        }

        // Insert |this| as a circular list into the linked list, after |e|.
        void insert_after_as_list(linked_node<T> *e) {
            linked_node<T> *prev = this->previous_;
            prev->next_ = e->next_;
            this->previous_ = e;
            e->next_->previous_ = prev;
            e->next_ = this;
        }

        // Remove |this| from the linked list.
        void remove_from_list() {
            this->previous_->next_ = this->next_;
            this->next_->previous_ = this->previous_;
            // next() and previous() return non-NULL if and only this node is not in any
            // list.
            this->next_ = this;
            this->previous_ = this;
        }

        linked_node<T> *previous() const {
            return previous_;
        }

        linked_node<T> *next() const {
            return next_;
        }

        // Cast from the node-type to the value type.
        const T *value() const {
            return static_cast<const T *>(this);
        }

        T *value() {
            return static_cast<T *>(this);
        }

    private:
        linked_node<T> *previous_;
        linked_node<T> *next_;
        ABEL_NON_COPYABLE(linked_node);
    };

    template<typename T>
    class linked_list {
    public:
        // The "root" node is self-referential, and forms the basis of a circular
        // list (root_.next() will point back to the start of the list,
        // and root_->previous() wraps around to the end of the list).
        linked_list() {}

        // Appends |e| to the end of the linked list.
        void append(linked_node<T> *e) {
            e->insert_before(&root_);
        }

        linked_node<T> *head() const {
            return root_.next();
        }

        linked_node<T> *tail() const {
            return root_.previous();
        }

        const linked_node<T> *end() const {
            return &root_;
        }

        bool empty() const { return head() == end(); }

    private:
        linked_node<T> root_;
        ABEL_NON_COPYABLE(linked_list);
    };
} //namespace abel
#endif //ABEL_ASL_LINKED_LIST_H_

