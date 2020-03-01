//
// Created by liyinbin on 2020/2/2.
//

#ifndef ABEL_FUNCTIONAL_INDIRECT_H_
#define ABEL_FUNCTIONAL_INDIRECT_H_

#include <memory>

namespace abel {

// This header defines functors for comparing and hashing pointers by pointed-to values instead of pointer addresses.
//
// Examples:
//
//  std::multiset<shared_ptr<sstring>, indirect_less<shared_ptr<sstring>>> _multiset;
//
//  std::unordered_map<shared_ptr<sstring>, bool,
//      indirect_hash<shared_ptr<sstring>>, indirect_equal_to<shared_ptr<sstring>>> _unordered_map;
//

    template<typename Pointer, typename Equal = std::equal_to<typename std::pointer_traits<Pointer>::element_type>>
    struct indirect_equal_to {
        Equal _eq;

        indirect_equal_to(Equal eq = Equal()) : _eq(std::move(eq)) {}

        bool operator()(const Pointer &i1, const Pointer &i2) const {
            if (bool(i1) ^ bool(i2)) {
                return false;
            }
            return !i1 || _eq(*i1, *i2);
        }
    };

    template<typename Pointer, typename Less = std::less<typename std::pointer_traits<Pointer>::element_type>>
    struct indirect_less {
        Less _cmp;

        indirect_less(Less cmp = Less()) : _cmp(std::move(cmp)) {}

        bool operator()(const Pointer &i1, const Pointer &i2) const {
            if (i1 && i2) {
                return _cmp(*i1, *i2);
            }
            return !i1 && i2;
        }
    };

    template<typename Pointer, typename Hash = std::hash<typename std::pointer_traits<Pointer>::element_type>>
    struct indirect_hash {
        Hash _h;

        indirect_hash(Hash h = Hash()) : _h(std::move(h)) {}

        size_t operator()(const Pointer &p) const {
            if (p) {
                return _h(*p);
            }
            return 0;
        }
    };

}
#endif //ABEL_FUNCTIONAL_INDIRECT_H_
