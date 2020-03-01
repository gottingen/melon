//
// Created by liyinbin on 2020/2/1.
//


#include <gtest/gtest.h>
#include <abel/container/intrusive_list.h>
#include <iterator>
#include <cstdio>
#include <stdarg.h>

template<typename InputIterator, typename StackValue>
bool VerifySequence(InputIterator first, InputIterator last, StackValue /*unused*/, const char *pName, ...) {
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    int argIndex = 0;
    int seqIndex = 0;
    bool bReturnValue = true;
    StackValue next;

    va_list args;
    va_start(args, pName);

    for (; first != last; ++first, ++argIndex, ++seqIndex) {
        next = va_arg(args, StackValue);

        if ((next == StackValue(-1)) || !(value_type(next) == *first)) {
            if (pName)
                ::printf("[%s] Mismatch at index %d\n", pName, argIndex);
            else
                ::printf("Mismatch at index %d\n", argIndex);
            bReturnValue = false;
        }
    }

    for (; first != last; ++first)
        ++seqIndex;

    if (bReturnValue) {
        next = va_arg(args, StackValue);

        if (!(next == StackValue(-1))) {
            do {
                ++argIndex;
                next = va_arg(args, StackValue);
            } while (!(next == StackValue(-1)));

            if (pName)
                ::printf("[%s] Too many elements: expected %d, found %d\n", pName, argIndex, seqIndex);
            else
                ::printf("Too many elements: expected %d, found %d\n", argIndex, seqIndex);
            bReturnValue = false;
        }
    }

    va_end(args);

    return bReturnValue;
}

using namespace abel;

namespace {
    struct IntNode : public abel::intrusive_list_node {
        int mX;

        IntNode(int x = 0)
                : mX(x) {}

        operator int() const { return mX; }
    };

    class ListInit {
    public:
        ListInit(abel::intrusive_list<IntNode> &container, IntNode *pNodeArray)
                : mpContainer(&container), mpNodeArray(pNodeArray) {
            mpContainer->clear();
        }

        ListInit &operator+=(int x) {
            mpNodeArray->mX = x;
            mpContainer->push_back(*mpNodeArray++);
            return *this;
        }

        ListInit &operator,(int x) {
            mpNodeArray->mX = x;
            mpContainer->push_back(*mpNodeArray++);
            return *this;
        }

    protected:
        abel::intrusive_list<IntNode> *mpContainer;
        IntNode *mpNodeArray;
    };

}

template
class abel::intrusive_list<IntNode>;

TEST(intrusive_list, all) {
    int i = 0;
    {
        const size_t offset = offsetof(abel::intrusive_list_node, prev);
        EXPECT_TRUE(offset == sizeof(abel::intrusive_list_node *));
    }

    {
        IntNode nodes[20];

        abel::intrusive_list<IntNode> ilist;

        // Enforce that the intrusive_list copy ctor is visible. If it is not,
        // then the class is not a POD type as it is supposed to be.
        delete new abel::intrusive_list<IntNode>(ilist);

#ifndef __GNUC__ // GCC warns on this, though strictly specaking it is allowed to.
        // Enforce that offsetof() can be used with an intrusive_list in a struct;
            // it requires a POD type. Some compilers will flag warnings or even errors
            // when this is violated.
            struct Test {
                abel::intrusive_list<IntNode> m;
            };
            (void)offsetof(Test, m);
#endif

        // begin / end
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "ctor()", -1));


        // push_back
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4, 5, 6, 7, 8, 9;
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "push_back()", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1));


        // iterator / begin
        abel::intrusive_list<IntNode>::iterator it = ilist.begin();
        EXPECT_TRUE(it->mX == 0);
        ++it;
        EXPECT_TRUE(it->mX == 1);
        ++it;
        EXPECT_TRUE(it->mX == 2);
        ++it;
        EXPECT_TRUE(it->mX == 3);


        // const_iterator / begin
        const abel::intrusive_list<IntNode> cilist;
        abel::intrusive_list<IntNode>::const_iterator cit;
        for (cit = cilist.begin(); cit != cilist.end(); ++cit)
            EXPECT_TRUE(cit == cilist.end()); // This is guaranteed to be false.


        // reverse_iterator / rbegin
        abel::intrusive_list<IntNode>::reverse_iterator itr = ilist.rbegin();
        EXPECT_TRUE(itr->mX == 9);
        ++itr;
        EXPECT_TRUE(itr->mX == 8);
        ++itr;
        EXPECT_TRUE(itr->mX == 7);
        ++itr;
        EXPECT_TRUE(itr->mX == 6);


        // iterator++/--
        {
            abel::intrusive_list<IntNode>::iterator it1(ilist.begin());
            abel::intrusive_list<IntNode>::iterator it2(ilist.begin());

            ++it1;
            ++it2;
            /*
            if ((it1 != it2++) || (++it1 != it2))
               // EXPECT_TRUE(!"[iterator::increment] fail\n");

            if ((it1 != it2--) || (--it1 != it2))
                EXPECT_TRUE(!"[iterator::decrement] fail\n");
                */
        }


        // clear / empty
        EXPECT_TRUE(!ilist.empty());

        ilist.clear();
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "clear()", -1));
        EXPECT_TRUE(ilist.empty());


        // splice
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4, 5, 6, 7, 8, 9;

        ilist.splice(++ilist.begin(), ilist, --ilist.end());
        EXPECT_TRUE(VerifySequence(ilist.begin(),
                                   ilist.end(),
                                   int(),
                                   "splice(single)",
                                   0,
                                   9,
                                   1,
                                   2,
                                   3,
                                   4,
                                   5,
                                   6,
                                   7,
                                   8,
                                   -1));

        intrusive_list<IntNode> ilist2;
        ListInit(ilist2, nodes + 10) += 10, 11, 12, 13, 14, 15, 16, 17, 18, 19;

        ilist.splice(++ ++ilist.begin(), ilist2);
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "splice(whole)", -1));
        EXPECT_TRUE(VerifySequence(ilist.begin(),
                                   ilist.end(),
                                   int(),
                                   "splice(whole)",
                                   0,
                                   9,
                                   10,
                                   11,
                                   12,
                                   13,
                                   14,
                                   15,
                                   16,
                                   17,
                                   18,
                                   19,
                                   1,
                                   2,
                                   3,
                                   4,
                                   5,
                                   6,
                                   7,
                                   8,
                                   -1));

        ilist.splice(ilist.begin(), ilist, ++ ++ilist.begin(), -- --ilist.end());
        EXPECT_TRUE(VerifySequence(ilist.begin(),
                                   ilist.end(),
                                   int(),
                                   "splice(range)",
                                   10,
                                   11,
                                   12,
                                   13,
                                   14,
                                   15,
                                   16,
                                   17,
                                   18,
                                   19,
                                   1,
                                   2,
                                   3,
                                   4,
                                   5,
                                   6,
                                   0,
                                   9,
                                   7,
                                   8,
                                   -1));

        ilist.clear();
        ilist.swap(ilist2);
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "swap(empty)", -1));
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "swap(empty)", -1));

        ilist2.push_back(nodes[0]);
        ilist.splice(ilist.begin(), ilist2);
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "splice(single)", 0, -1));
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "splice(single)", -1));


        // splice(single) -- evil case (splice at or right after current position)
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4;
        ilist.splice(++ ++ilist.begin(), *++ ++ilist.begin());
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "splice(single)", 0, 1, 2, 3, 4, -1));
        ilist.splice(++ ++ ++ilist.begin(), *++ ++ilist.begin());
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "splice(single)", 0, 1, 2, 3, 4, -1));


        // splice(range) -- evil case (splice right after current position)
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4;
        ilist.splice(++ ++ilist.begin(), ilist, ++ilist.begin(), ++ ++ilist.begin());
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "splice(range)", 0, 1, 2, 3, 4, -1));


        // push_front / push_back
        ilist.clear();
        ilist2.clear();
        for (i = 4; i >= 0; --i)
            ilist.push_front(nodes[i]);
        for (i = 5; i < 10; ++i)
            ilist2.push_back(nodes[i]);

        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "push_front()", 0, 1, 2, 3, 4, -1));
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "push_back()", 5, 6, 7, 8, 9, -1));

        for (i = 4; i >= 0; --i) {
            ilist.pop_front();
            ilist2.pop_back();
        }

        EXPECT_TRUE(ilist.empty() && ilist2.empty());
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "pop_front()", -1));
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "pop_back()", -1));


        // contains / locate
        for (i = 0; i < 5; ++i)
            ilist.push_back(nodes[i]);

        EXPECT_TRUE(ilist.contains(nodes[2]));
        EXPECT_TRUE(!ilist.contains(nodes[7]));

        it = ilist.locate(nodes[3]);
        EXPECT_TRUE(it->mX == 3);

        it = ilist.locate(nodes[8]);
        EXPECT_TRUE(it == ilist.end());


        // reverse
        ilist.reverse();
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "push_front()", 4, 3, 2, 1, 0, -1));


        // validate / validate_iterator
        EXPECT_TRUE(ilist.validate());
        it = ilist.locate(nodes[3]);
        EXPECT_TRUE((ilist.validate_iterator(it) & (isf_valid | isf_can_dereference)) != 0);
        EXPECT_TRUE(ilist.validate_iterator(intrusive_list<IntNode>::iterator(NULL)) == isf_none);


        // swap()
        ilist.swap(ilist2);
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "swap()", -1));
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "swap()", 4, 3, 2, 1, 0, -1));


        // erase()
        ListInit(ilist2, nodes) += 0, 1, 2, 3, 4;
        ListInit(ilist, nodes + 5) += 5, 6, 7, 8, 9;
        ilist.erase(++ ++ilist.begin());
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "erase(single)", 5, 6, 8, 9, -1));

        ilist.erase(ilist.begin(), ilist.end());
        EXPECT_TRUE(VerifySequence(ilist.begin(), ilist.end(), int(), "erase(all)", -1));

        ilist2.erase(++ilist2.begin(), -- --ilist2.end());
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "erase(range)", 0, 3, 4, -1));


        // size
        EXPECT_TRUE(ilist2.size() == 3);


        // pop_front / pop_back
        ilist2.pop_front();
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "pop_front()", 3, 4, -1));

        ilist2.pop_back();
        EXPECT_TRUE(VerifySequence(ilist2.begin(), ilist2.end(), int(), "pop_back()", 3, -1));
    }

    {
        // Test copy construction and assignment.
        // The following *should* not compile.

        intrusive_list<IntNode> ilist1;
        intrusive_list<IntNode> ilist2(ilist1);
        ilist1 = ilist2;
    }

    {
        // void sort()
        // void sort(Compare compare)

        const int kSize = 10;
        IntNode nodes[kSize];

        intrusive_list<IntNode> listEmpty;
        listEmpty.sort();
        EXPECT_TRUE(VerifySequence(listEmpty.begin(), listEmpty.end(), int(), "list::sort", -1));

        intrusive_list<IntNode> list1;
        ListInit(list1, nodes) += 1;
        list1.sort();
        EXPECT_TRUE(VerifySequence(list1.begin(), list1.end(), int(), "list::sort", 1, -1));
        list1.clear();

        intrusive_list<IntNode> list4;
        ListInit(list4, nodes) += 1, 9, 2, 3;
        list4.sort();
        EXPECT_TRUE(VerifySequence(list4.begin(), list4.end(), int(), "list::sort", 1, 2, 3, 9, -1));
        list4.clear();

        intrusive_list<IntNode> listA;
        ListInit(listA, nodes) += 1, 9, 2, 3, 5, 7, 4, 6, 8, 0;
        listA.sort();
        EXPECT_TRUE(VerifySequence(listA.begin(), listA.end(), int(), "list::sort", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1));
        listA.clear();

        intrusive_list<IntNode> listB;
        ListInit(listB, nodes) += 1, 9, 2, 3, 5, 7, 4, 6, 8, 0;
        listB.sort(std::less<int>());
        EXPECT_TRUE(VerifySequence(listB.begin(), listB.end(), int(), "list::sort", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1));
        listB.clear();
    }

    {
        // void merge(this_type& x);
        // void merge(this_type& x, Compare compare);

        const int kSize = 8;
        IntNode nodesA[kSize];
        IntNode nodesB[kSize];

        intrusive_list<IntNode> listA;
        ListInit(listA, nodesA) += 1, 2, 3, 4, 4, 5, 9, 9;

        intrusive_list<IntNode> listB;
        ListInit(listB, nodesB) += 1, 2, 3, 4, 4, 5, 9, 9;

        listA.merge(listB);
        EXPECT_TRUE(VerifySequence(listA.begin(),
                                   listA.end(),
                                   int(),
                                   "list::merge",
                                   1,
                                   1,
                                   2,
                                   2,
                                   3,
                                   3,
                                   4,
                                   4,
                                   4,
                                   4,
                                   5,
                                   5,
                                   9,
                                   9,
                                   9,
                                   9,
                                   -1));
        EXPECT_TRUE(VerifySequence(listB.begin(), listB.end(), int(), "list::merge", -1));
    }

    {
        // void unique();
        // void unique(BinaryPredicate);

        const int kSize = 8;
        IntNode nodesA[kSize];
        IntNode nodesB[kSize];

        intrusive_list<IntNode> listA;
        ListInit(listA, nodesA) += 1, 2, 3, 4, 4, 5, 9, 9;
        listA.unique();
        EXPECT_TRUE(VerifySequence(listA.begin(), listA.end(), int(), "list::unique", 1, 2, 3, 4, 5, 9, -1));

        intrusive_list<IntNode> listB;
        ListInit(listB, nodesB) += 1, 2, 3, 4, 4, 5, 9, 9;
        listB.unique(std::equal_to<int>());
        EXPECT_TRUE(VerifySequence(listA.begin(), listA.end(), int(), "list::unique", 1, 2, 3, 4, 5, 9, -1));
    }

}
