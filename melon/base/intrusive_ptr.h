#ifndef MUTIL_INTRUSIVE_PTR_HPP
#define MUTIL_INTRUSIVE_PTR_HPP

//  Copyright (c) 2001, 2002 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/intrusive_ptr.html for documentation.
//
//  intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//  
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.

#include <functional>
#include <cstddef>
#include <ostream>
#include <melon/base/build_config.h>
#include <melon/utility/containers/hash_tables.h>

namespace mutil {

namespace detail {

// NOTE: sp_convertible is different from mutil::is_convertible
// (in type_traits.h) that it converts pointers only. Using
// mutil::is_convertible results in ctor/dtor issues.
template< class Y, class T > struct sp_convertible {
    typedef char (&yes) [1];
    typedef char (&no)  [2];

    static yes f( T* );
    static no  f( ... );

    enum _vt { value = sizeof((f)(static_cast<Y*>(0))) == sizeof(yes) };
};
template< class Y, class T > struct sp_convertible<Y, T[]> {
    enum _vt { value = false };
};
template< class Y, class T > struct sp_convertible<Y[], T[]> {
    enum _vt { value = sp_convertible<Y[1], T[1]>::value };
};
template<class Y, std::size_t N, class T> struct sp_convertible<Y[N], T[]> {
    enum _vt { value = sp_convertible<Y[1], T[1]>::value };
};

struct sp_empty {};
template< bool > struct sp_enable_if_convertible_impl;
template<> struct sp_enable_if_convertible_impl<true> { typedef sp_empty type; };
template<> struct sp_enable_if_convertible_impl<false> {};
template< class Y, class T > struct sp_enable_if_convertible
    : public sp_enable_if_convertible_impl<sp_convertible<Y, T>::value> {};

} // namespace detail

template<class T> class intrusive_ptr {
private:
    typedef intrusive_ptr this_type;
public:
    typedef T element_type;
    intrusive_ptr() MELON_NOEXCEPT : px(0) {}

    intrusive_ptr(T * p, bool add_ref = true): px(p) {
        if(px != 0 && add_ref) intrusive_ptr_add_ref(px);
    }

    template<class U>
    intrusive_ptr(const intrusive_ptr<U>& rhs,
                  typename detail::sp_enable_if_convertible<U,T>::type = detail::sp_empty())
        : px(rhs.get()) {
        if(px != 0) intrusive_ptr_add_ref(px);
    }

    intrusive_ptr(const intrusive_ptr& rhs): px(rhs.px) {
        if(px != 0) intrusive_ptr_add_ref(px);
    }

    ~intrusive_ptr() {
        if(px != 0) intrusive_ptr_release(px);
    }

    template<class U> intrusive_ptr & operator=(const intrusive_ptr<U>& rhs) {
        this_type(rhs).swap(*this);
        return *this;
    }

// Move support
#if defined(MUTIL_CXX11_ENABLED)
    intrusive_ptr(intrusive_ptr && rhs) MELON_NOEXCEPT : px(rhs.px) {
        rhs.px = 0;
    }

    intrusive_ptr & operator=(intrusive_ptr && rhs) MELON_NOEXCEPT {
        this_type(static_cast< intrusive_ptr && >(rhs)).swap(*this);
        return *this;
    }
#endif

    intrusive_ptr & operator=(const intrusive_ptr& rhs) {
        this_type(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr & operator=(T * rhs) {
        this_type(rhs).swap(*this);
        return *this;
    }

    void reset() MELON_NOEXCEPT {
        this_type().swap(*this);
    }

    void reset(T * rhs) {
        this_type(rhs).swap(*this);
    }

    void reset(T * rhs, bool add_ref) {
        this_type(rhs, add_ref).swap(*this);
    }

    T * get() const MELON_NOEXCEPT {
        return px;
    }

    T * detach() MELON_NOEXCEPT {
        T * ret = px;
        px = 0;
        return ret;
    }

    T & operator*() const {
        return *px;
    }

    T * operator->() const {
        return px;
    }

    // implicit conversion to "bool"
#if defined(MUTIL_CXX11_ENABLED)
    explicit operator bool () const MELON_NOEXCEPT {
        return px != 0;
    }
#elif defined(__CINT__)
    operator bool () const MELON_NOEXCEPT {
        return px != 0;
    }
#elif (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 304))
    typedef element_type * (this_type::*unspecified_bool_type)() const;
    operator unspecified_bool_type() const MELON_NOEXCEPT {
        return px == 0? 0: &this_type::get;
    }
#else
    typedef element_type * this_type::*unspecified_bool_type;
    operator unspecified_bool_type() const MELON_NOEXCEPT {
        return px == 0? 0: &this_type::px;
    }
#endif

    // operator! is redundant, but some compilers need it
    bool operator! () const MELON_NOEXCEPT {
        return px == 0;
    }

    void swap(intrusive_ptr & rhs) MELON_NOEXCEPT {
        T * tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

private:
    T * px;
};

template<class T, class U>
inline bool operator==(const intrusive_ptr<T>& a, const intrusive_ptr<U>& b) {
    return a.get() == b.get();
}

template<class T, class U>
inline bool operator!=(const intrusive_ptr<T>& a, const intrusive_ptr<U>& b) {
    return a.get() != b.get();
}

template<class T, class U>
inline bool operator==(const intrusive_ptr<T>& a, U * b) {
    return a.get() == b;
}

template<class T, class U>
inline bool operator!=(const intrusive_ptr<T>& a, U * b) {
    return a.get() != b;
}

template<class T, class U>
inline bool operator==(T * a, const intrusive_ptr<U>& b) {
    return a == b.get();
}

template<class T, class U>
inline bool operator!=(T * a, const intrusive_ptr<U>& b) {
    return a != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96
// Resolve the ambiguity between our op!= and the one in rel_ops
template<class T>
inline bool operator!=(const intrusive_ptr<T>& a, const intrusive_ptr<T>& b) {
    return a.get() != b.get();
}
#endif

#if defined(MUTIL_CXX11_ENABLED)
template<class T>
inline bool operator==(const intrusive_ptr<T>& p, std::nullptr_t) MELON_NOEXCEPT {
    return p.get() == 0;
}
template<class T>
inline bool operator==(std::nullptr_t, const intrusive_ptr<T>& p) MELON_NOEXCEPT {
    return p.get() == 0;
}

template<class T>
inline bool operator!=(const intrusive_ptr<T>& p, std::nullptr_t) MELON_NOEXCEPT {
    return p.get() != 0;
}
template<class T>
inline bool operator!=(std::nullptr_t, const intrusive_ptr<T>& p) MELON_NOEXCEPT {
    return p.get() != 0;
}
#endif  // MUTIL_CXX11_ENABLED

template<class T>
inline bool operator<(const intrusive_ptr<T>& a, const intrusive_ptr<T>& b) {
    return std::less<T *>()(a.get(), b.get());
}

template<class T> void swap(intrusive_ptr<T> & lhs, intrusive_ptr<T> & rhs) {
    lhs.swap(rhs);
}

// mem_fn support

template<class T> T * get_pointer(const intrusive_ptr<T>& p) {
    return p.get();
}

template<class T, class U> intrusive_ptr<T> static_pointer_cast(const intrusive_ptr<U>& p) {
    return static_cast<T *>(p.get());
}

template<class T, class U> intrusive_ptr<T> const_pointer_cast(const intrusive_ptr<U>& p) {
    return const_cast<T *>(p.get());
}

template<class T, class U> intrusive_ptr<T> dynamic_pointer_cast(const intrusive_ptr<U>& p) {
    return dynamic_cast<T *>(p.get());
}

template<class Y> std::ostream & operator<< (std::ostream & os, const intrusive_ptr<Y>& p) {
    os << p.get();
    return os;
}

} // namespace mutil

// hash_value
namespace MUTIL_HASH_NAMESPACE {

#if defined(COMPILER_GCC)
template<typename T>
struct hash<mutil::intrusive_ptr<T> > {
    std::size_t operator()(const mutil::intrusive_ptr<T>& p) const {
        return hash<T*>()(p.get());
    }
};
#elif defined(COMPILER_MSVC)
template<typename T>
inline size_t hash_value(const mutil::intrusive_ptr<T>& sp) {
    return hash_value(p.get());
}
#endif  // COMPILER

}  // namespace MUTIL_HASH_NAMESPACE

#endif  // MUTIL_INTRUSIVE_PTR_HPP
