//
// Created by liyinbin on 2020/2/1.
//

#include <abel/container/intrusive_list.h>

namespace abel {

    void intrusive_list_base::reverse() ABEL_NOEXCEPT {
        intrusive_list_node *pNode = &_anchor;
        do {
            intrusive_list_node *const pTemp = pNode->next;
            pNode->next = pNode->prev;
            pNode->prev = pTemp;
            pNode = pNode->prev;
        } while (pNode != &_anchor);
    }

    bool intrusive_list_base::validate() const {
        const intrusive_list_node *p = &_anchor;
        const intrusive_list_node *q = p;

        // We do two tests below:
        //
        // 1) Prev and next pointers are symmetric. We check (p->next->prev == p)
        //    for each node, which is enough to verify all links.
        //
        // 2) Loop check. We bump the q pointer at one-half rate compared to the
        //    p pointer; (p == q) if and only if we are at the start (which we
        //    don't check) or if there is a loop somewhere in the list.

        do {
            // validate node (even phase)
            if (p->next->prev != p)
                return false;               // broken linkage detected

            // bump only fast pointer
            p = p->next;
            if (p == &_anchor)
                break;

            if (p == q)
                return false;               // loop detected

            // validate node (odd phase)
            if (p->next->prev != p)
                return false;               // broken linkage detected

            // bump both pointers
            p = p->next;
            q = q->next;

            if (p == q)
                return false;               // loop detected

        } while (p != &_anchor);

        return true;
    }

} //namespace abel
