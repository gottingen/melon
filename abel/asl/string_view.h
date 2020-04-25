#ifndef ABEL_STRINGS_STRING_VIEW_H_
#define ABEL_STRINGS_STRING_VIEW_H_

#include <algorithm>
#include <abel/base/profile.h>
#include <abel/strings/internal/char_traits.h>
#include <type_traits>
#include <algorithm>
#include <climits>
#include <cstring>
#include <ostream>

#ifdef ABEL_USES_STD_STRING_VIEW

#include <string_view>  // IWYU pragma: export

namespace abel {

using std::string_view;

}  // namespace abel

#else  // ABEL_USES_STD_STRING_VIEW

#if ABEL_COMPILER_HAS_BUILTIN(__builtin_memcmp) || \
    (defined(__GNUC__) && !defined(__clang__))
#define ABEL_INTERNAL_STRING_VIEW_MEMCMP __builtin_memcmp
#else  // ABEL_COMPILER_HAS_BUILTIN(__builtin_memcmp)
#define ABEL_INTERNAL_STRING_VIEW_MEMCMP memcmp
#endif  // ABEL_COMPILER_HAS_BUILTIN(__builtin_memcmp)

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>

#include <abel/base/throw_delegate.h>
#include <abel/base/profile.h>

namespace abel {

// abel::basic_string_view
//
// A `basic_string_view` provides a lightweight view into the string data provided by
// a `std::string`, double-quoted string literal, character array, or even
// another `basic_string_view`. A `basic_string_view` does *not* own the string to which it
// points, and that data cannot be modified through the view.
//
// You can use `basic_string_view` as a function or method parameter anywhere a
// parameter can receive a double-quoted string literal, `const char*`,
// `std::string`, or another `abel::basic_string_view` argument with no need to copy
// the string data. Systematic use of `basic_string_view` within function arguments
// reduces data copies and `strlen()` calls.
//
// Because of its small size, prefer passing `basic_string_view` by value:
//
//   void MyFunction(abel::basic_string_view arg);
//
// If circumstances require, you may also pass one by const reference:
//
//   void MyFunction(const abel::basic_string_view& arg);  // not preferred
//
// Passing by value generates slightly smaller code for many architectures.
//
// In either case, the source data of the `basic_string_view` must outlive the
// `basic_string_view` itself.
//
// A `basic_string_view` is also suitable for local variables if you know that the
// lifetime of the underlying object is longer than the lifetime of your
// `basic_string_view` variable. However, beware of binding a `basic_string_view` to a
// temporary value:
//
//   // BAD use of basic_string_view: lifetime problem
//   abel::basic_string_view sv = obj.ReturnAString();
//
//   // GOOD use of basic_string_view: str outlives sv
//   std::string str = obj.ReturnAString();
//   abel::basic_string_view sv = str;
//
// Due to lifetime issues, a `basic_string_view` is sometimes a poor choice for a
// return value and usually a poor choice for a data member. If you do use a
// `basic_string_view` this way, it is your responsibility to ensure that the object
// pointed to by the `basic_string_view` outlives the `basic_string_view`.
//
// A `basic_string_view` may represent a whole string or just part of a string. For
// example, when splitting a string, `std::vector<abel::basic_string_view>` is a
// natural data type for the output.
//
// When constructed from a source which is NUL-terminated, the `basic_string_view`
// itself will not include the NUL-terminator unless a specific size (including
// the NUL) is passed to the constructor. As a result, common idioms that work
// on NUL-terminated strings do not work on `basic_string_view` objects. If you write
// code that scans a `basic_string_view`, you must check its length rather than test
// for nul, for example. Note, however, that nuls may still be embedded within
// a `basic_string_view` explicitly.
//
// You may create a null `basic_string_view` in two ways:
//
//   abel::basic_string_view sv();
//   abel::basic_string_view sv(nullptr, 0);
//
// For the above, `sv.data() == nullptr`, `sv.length() == 0`, and
// `sv.empty() == true`. Also, if you create a `basic_string_view` with a non-null
// pointer then `sv.data() != nullptr`. Thus, you can use `basic_string_view()` to
// signal an undefined value that is different from other `basic_string_view` values
// in a similar fashion to how `const char* p1 = nullptr;` is different from
// `const char* p2 = "";`. However, in practice, it is not recommended to rely
// on this behavior.
//
// Be careful not to confuse a null `basic_string_view` with an empty one. A null
// `basic_string_view` is an empty `basic_string_view`, but some empty `basic_string_view`s are
// not null. Prefer checking for emptiness over checking for null.
//
// There are many ways to create an empty basic_string_view:
//
//   const char* nullcp = nullptr;
//   // basic_string_view.size() will return 0 in all cases.
//   abel::basic_string_view();
//   abel::basic_string_view(nullcp, 0);
//   abel::basic_string_view("");
//   abel::basic_string_view("", 0);
//   abel::basic_string_view("abcdef", 0);
//   abel::basic_string_view("abcdef" + 6, 0);
//
// All empty `basic_string_view` objects whether null or not, are equal:
//
//   abel::basic_string_view() == abel::basic_string_view("", 0)
//   abel::basic_string_view(nullptr, 0) == abel::basic_string_view("abcdef"+6, 0)

    template<typename T>
    class basic_string_view {
    public:
        using value_type = typename std::remove_pointer<T>::type;
        using traits_type = std::char_traits<value_type>;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using reference = value_type &;
        using const_reference = const value_type &;
        using const_iterator = const value_type *;
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;

        static constexpr size_type npos = static_cast<size_type>(-1);

        // Null `basic_string_view` constructor
        constexpr basic_string_view() noexcept : ptr_(nullptr), length_(0) {}

        // Implicit constructors

        template<typename Allocator>
        basic_string_view(const std::basic_string<T, traits_type, Allocator> &str) noexcept
        // This is implemented in terms of `basic_string_view(p, n)` so `str.size()`
        // doesn't need to be reevaluated after `ptr_` is set.
                : basic_string_view(str.data(), str.size()) {}

        // Implicit constructor of a `basic_string_view` from NUL-terminated `str`. When
        // accepting possibly null strings, use `abel::null_safe_string_view(str)`
        // instead (see below).
        constexpr basic_string_view(const value_type *str)
                : ptr_(str),
                  length_(str ? std::char_traits<value_type>::length(str) : 0) {}

        // Implicit constructor of a `basic_string_view` from a `const value_type*` and length.
        constexpr basic_string_view(const value_type *data, size_type len)
                : ptr_(data), length_(CheckLengthInternal(len)) {}

        // NOTE: Harmlessly omitted to work around gdb bug.
        //   constexpr basic_string_view(const basic_string_view&) noexcept = default;
//    basic_string_view& operator=(const basic_string_view&) noexcept = default;
//
//    basic_string_view& operator=(const value_type *str) noexcept {
//        basic_string_view bsv(str);
//        *this = bsv;
//        return *this;
//    }

        // Iterators

        // basic_string_view::begin()
        //
        // Returns an iterator pointing to the first character at the beginning of the
        // `basic_string_view`, or `end()` if the `basic_string_view` is empty.
        constexpr const_iterator begin() const noexcept { return ptr_; }

        // basic_string_view::end()
        //
        // Returns an iterator pointing just beyond the last character at the end of
        // the `basic_string_view`. This iterator acts as a placeholder; attempting to
        // access it results in undefined behavior.
        constexpr const_iterator end() const noexcept { return ptr_ + length_; }

        // basic_string_view::cbegin()
        //
        // Returns a const iterator pointing to the first character at the beginning
        // of the `basic_string_view`, or `end()` if the `basic_string_view` is empty.
        constexpr const_iterator cbegin() const noexcept { return begin(); }

        // basic_string_view::cend()
        //
        // Returns a const iterator pointing just beyond the last character at the end
        // of the `basic_string_view`. This pointer acts as a placeholder; attempting to
        // access its element results in undefined behavior.
        constexpr const_iterator cend() const noexcept { return end(); }

        // basic_string_view::rbegin()
        //
        // Returns a reverse iterator pointing to the last character at the end of the
        // `basic_string_view`, or `rend()` if the `basic_string_view` is empty.
        const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator(end());
        }

        // basic_string_view::rend()
        //
        // Returns a reverse iterator pointing just before the first character at the
        // beginning of the `basic_string_view`. This pointer acts as a placeholder;
        // attempting to access its element results in undefined behavior.
        const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator(begin());
        }

        // basic_string_view::crbegin()
        //
        // Returns a const reverse iterator pointing to the last character at the end
        // of the `basic_string_view`, or `crend()` if the `basic_string_view` is empty.
        const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        // basic_string_view::crend()
        //
        // Returns a const reverse iterator pointing just before the first character
        // at the beginning of the `basic_string_view`. This pointer acts as a placeholder;
        // attempting to access its element results in undefined behavior.
        const_reverse_iterator crend() const noexcept { return rend(); }

        // Capacity Utilities

        // basic_string_view::size()
        //
        // Returns the number of characters in the `basic_string_view`.
        constexpr size_type size() const noexcept {
            return length_;
        }

        // basic_string_view::length()
        //
        // Returns the number of characters in the `basic_string_view`. Alias for `size()`.
        constexpr size_type length() const noexcept { return size(); }

        // basic_string_view::max_size()
        //
        // Returns the maximum number of characters the `basic_string_view` can hold.
        constexpr size_type max_size() const noexcept { return kMaxSize; }

        // basic_string_view::empty()
        //
        // Checks if the `basic_string_view` is empty (refers to no characters).
        constexpr bool empty() const noexcept { return length_ == 0; }

        // basic_string_view::operator[]
        //
        // Returns the ith element of the `basic_string_view` using the array operator.
        // Note that this operator does not perform any bounds checking.
        constexpr const_reference operator[](size_type i) const { return ptr_[i]; }

        // basic_string_view::at()
        //
        // Returns the ith element of the `basic_string_view`. Bounds checking is performed,
        // and an exception of type `std::out_of_range` will be thrown on invalid
        // access.
        constexpr const_reference at(size_type i) const {
            return ABEL_LIKELY(i < size())
                   ? ptr_[i]
                   : ((void) throw_std_out_of_range(
                            "abel::basic_string_view::at"),
                            ptr_[i]);
        }

        // basic_string_view::front()
        //
        // Returns the first element of a `basic_string_view`.
        constexpr const_reference front() const { return ptr_[0]; }

        // basic_string_view::back()
        //
        // Returns the last element of a `basic_string_view`.
        constexpr const_reference back() const { return ptr_[size() - 1]; }

        // basic_string_view::data()
        //
        // Returns a pointer to the underlying character array (which is of course
        // stored elsewhere). Note that `basic_string_view::data()` may contain embedded nul
        // characters, but the returned buffer may or may not be NUL-terminated;
        // therefore, do not pass `data()` to a routine that expects a NUL-terminated
        // std::string.
        constexpr const_pointer data() const noexcept { return ptr_; }

        // Modifiers

        // basic_string_view::remove_prefix()
        //
        // Removes the first `n` characters from the `basic_string_view`. Note that the
        // underlying std::string is not changed, only the view.
        void remove_prefix(size_type n) {
            assert(n <= length_);
            ptr_ += n;
            length_ -= n;
        }

        // basic_string_view::remove_suffix()
        //
        // Removes the last `n` characters from the `basic_string_view`. Note that the
        // underlying std::string is not changed, only the view.
        void remove_suffix(size_type n) {
            assert(n <= length_);
            length_ -= n;
        }

        // basic_string_view::swap()
        //
        // Swaps this `basic_string_view` with another `basic_string_view`.
        void swap(basic_string_view &s) noexcept {
            auto t = *this;
            *this = s;
            s = t;
        }

        // Explicit conversion operators

        // Converts to `std::basic_string`.
        template<typename A>
        explicit operator std::basic_string<value_type, traits_type, A>() const {
            if (!data())
                return {};
            return std::basic_string<value_type, traits_type, A>(data(), size());
        }

        // basic_string_view::copy()
        //
        // Copies the contents of the `basic_string_view` at offset `pos` and length `n`
        // into `buf`.
        size_type copy(value_type *buf, size_type n, size_type pos = 0) const {
            if (ABEL_UNLIKELY(pos > length_)) {
                throw_std_out_of_range("abel::basic_string_view::copy");
            }
            size_type rlen = (std::min)(length_ - pos, n);
            if (rlen > 0) {
                const value_type *start = ptr_ + pos;
                traits_type::copy(buf, start, rlen);
            }
            return rlen;
        }

        // basic_string_view::substr()
        //
        // Returns a "substring" of the `basic_string_view` (at offset `pos` and length
        // `n`) as another basic_string_view. This function throws `std::out_of_bounds` if
        // `pos > size`.
        basic_string_view substr(size_type pos, size_type n = npos) const {
            if (ABEL_UNLIKELY(pos > length_))
                throw_std_out_of_range("abel::basic_string_view::substr");
            n = (std::min)(n, length_ - pos);
            return basic_string_view(ptr_ + pos, n);
        }

        // basic_string_view::compare()
        //
        // Performs a lexicographical comparison between the `basic_string_view` and
        // another `abel::basic_string_view`, returning -1 if `this` is less than, 0 if
        // `this` is equal to, and 1 if `this` is greater than the passed std::string
        // view. Note that in the case of data equality, a further comparison is made
        // on the respective sizes of the two `basic_string_view`s to determine which is
        // smaller, equal, or greater.
        constexpr int compare(basic_string_view x) const noexcept {
            return CompareImpl(
                    length_, x.length_,
                    length_ == 0 || x.length_ == 0
                    ? 0
                    : ABEL_INTERNAL_STRING_VIEW_MEMCMP(
                            ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_));
        }

        // Overload of `basic_string_view::compare()` for comparing a substring of the
        // 'basic_string_view` and another `abel::basic_string_view`.
        int compare(size_type pos1, size_type count1, basic_string_view v) const {
            return substr(pos1, count1).compare(v);
        }

        // Overload of `basic_string_view::compare()` for comparing a substring of the
        // `basic_string_view` and a substring of another `abel::basic_string_view`.
        int compare(size_type pos1, size_type count1, basic_string_view v, size_type pos2,
                    size_type count2) const {
            return substr(pos1, count1).compare(v.substr(pos2, count2));
        }

        // Overload of `basic_string_view::compare()` for comparing a `basic_string_view` and a
        // a different  C-style std::string `s`.
        int compare(const value_type *s) const { return compare(basic_string_view(s)); }

        // Overload of `basic_string_view::compare()` for comparing a substring of the
        // `basic_string_view` and a different std::string C-style std::string `s`.
        int compare(size_type pos1, size_type count1, const value_type *s) const {
            return substr(pos1, count1).compare(basic_string_view(s));
        }

        // Overload of `basic_string_view::compare()` for comparing a substring of the
        // `basic_string_view` and a substring of a different C-style std::string `s`.
        int compare(size_type pos1, size_type count1, const value_type *s,
                    size_type count2) const {
            return substr(pos1, count1).compare(basic_string_view(s, count2));
        }

        // Find Utilities

        // basic_string_view::find()
        //
        // Finds the first occurrence of the substring `s` within the `basic_string_view`,
        // returning the position of the first character's match, or `npos` if no
        // match was found.
        size_type find(basic_string_view s, size_type pos = 0) const noexcept;

        // Overload of `basic_string_view::find()` for finding the given character `c`
        // within the `basic_string_view`.
        size_type find(value_type c, size_type pos = 0) const noexcept;

        // basic_string_view::rfind()
        //
        // Finds the last occurrence of a substring `s` within the `basic_string_view`,
        // returning the position of the first character's match, or `npos` if no
        // match was found.
        size_type rfind(basic_string_view s, size_type pos = npos) const
        noexcept;

        // Overload of `basic_string_view::rfind()` for finding the last given character `c`
        // within the `basic_string_view`.
        size_type rfind(value_type c, size_type pos = npos) const noexcept;

        // basic_string_view::find_first_of()
        //
        // Finds the first occurrence of any of the characters in `s` within the
        // `basic_string_view`, returning the start position of the match, or `npos` if no
        // match was found.
        size_type find_first_of(basic_string_view s, size_type pos = 0) const
        noexcept;

        // Overload of `basic_string_view::find_first_of()` for finding a character `c`
        // within the `basic_string_view`.
        size_type find_first_of(value_type c, size_type pos = 0) const
        noexcept {
            return find(c, pos);
        }

        // basic_string_view::find_last_of()
        //
        // Finds the last occurrence of any of the characters in `s` within the
        // `basic_string_view`, returning the start position of the match, or `npos` if no
        // match was found.
        size_type find_last_of(basic_string_view s, size_type pos = npos) const
        noexcept;

        // Overload of `basic_string_view::find_last_of()` for finding a character `c`
        // within the `basic_string_view`.
        size_type find_last_of(value_type c, size_type pos = npos) const
        noexcept {
            return rfind(c, pos);
        }

        // basic_string_view::find_first_not_of()
        //
        // Finds the first occurrence of any of the characters not in `s` within the
        // `basic_string_view`, returning the start position of the first non-match, or
        // `npos` if no non-match was found.
        size_type find_first_not_of(basic_string_view s, size_type pos = 0) const noexcept;

        // Overload of `basic_string_view::find_first_not_of()` for finding a character
        // that is not `c` within the `basic_string_view`.
        size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept;

        // basic_string_view::find_last_not_of()
        //
        // Finds the last occurrence of any of the characters not in `s` within the
        // `basic_string_view`, returning the start position of the last non-match, or
        // `npos` if no non-match was found.
        size_type find_last_not_of(basic_string_view s,
                                   size_type pos = npos) const noexcept;

        // Overload of `basic_string_view::find_last_not_of()` for finding a character
        // that is not `c` within the `basic_string_view`.
        size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept;

        // starts_with
        ABEL_CONSTEXPR bool starts_with(basic_string_view x) const noexcept
        {
            return (size() >= x.size()) && (compare(0, x.size(), x) == 0);
        }

        ABEL_CONSTEXPR bool starts_with(T x) const noexcept
        {
            return starts_with(basic_string_view(&x, 1));
        }

        ABEL_CONSTEXPR bool starts_with(const T* s) const
        {
            return starts_with(basic_string_view(s));
        }

        ABEL_CONSTEXPR bool ends_with(basic_string_view x) const noexcept {
            return (size() >= x.size()) && (compare(size() - x.size(), npos, x) == 0);
        }

        ABEL_CONSTEXPR bool ends_with(T x) const noexcept {
            return ends_with(basic_string_view(&x, 1));
        }

        ABEL_CONSTEXPR bool ends_with(const T *s) const noexcept {
            return ends_with(basic_string_view(s));
        }

        void clear() noexcept {
            ptr_ = nullptr;
            length_ = 0;
        }

    private:
        static constexpr size_type kMaxSize =
                (std::numeric_limits<difference_type>::max)();

        static constexpr size_type CheckLengthInternal(size_type len) {
            return (void) ABEL_ASSERT(len <= kMaxSize), len;
        }

        static constexpr int CompareImpl(size_type length_a, size_type length_b,
                                         int compare_result) {
            return compare_result == 0 ? static_cast<int>(length_a > length_b) -
                                         static_cast<int>(length_a < length_b)
                                       : static_cast<int>(compare_result > 0) -
                                         static_cast<int>(compare_result < 0);
        }

        const value_type *ptr_;
        size_type length_;
    };

    using string_view = basic_string_view<char>;
    using wstring_view = basic_string_view<wchar_t>;

    typedef basic_string_view<char> u8string_view;  // Actually not a C++17 type, but added for consistency.
    typedef basic_string_view<char16_t> u16string_view;
    typedef basic_string_view<char32_t> u32string_view;

// This large function is defined ABEL_FORCE_INLINE so that in a fairly common case where
// one of the arguments is a literal, the compiler can elide a lot of the
// following comparisons.
    template<typename T>
    constexpr bool operator==(basic_string_view<T> x, basic_string_view<T> y) noexcept {
        return x.size() == y.size() &&
               (x.empty() ||
                ABEL_INTERNAL_STRING_VIEW_MEMCMP(x.data(), y.data(), x.size()) == 0);
    }

    template<class CharT>
    inline constexpr bool operator==(typename std::decay<basic_string_view<CharT>>::type x,
                                     basic_string_view<CharT> y) noexcept {
        return x.size() == y.size() &&
               (x.empty() ||
                ABEL_INTERNAL_STRING_VIEW_MEMCMP(x.data(), y.data(), x.size()) == 0);
    }

    template<class CharT>
    inline constexpr bool operator==(basic_string_view<CharT> x, typename std::decay<basic_string_view<CharT>>::type y)
    noexcept {
        return x.size() == y.size() &&
               (x.empty() ||
                ABEL_INTERNAL_STRING_VIEW_MEMCMP(x.data(), y.data(), x.size()) == 0);
    }

    template<class CharT>
    inline constexpr bool operator==(typename std::decay<basic_string_view<CharT>>::type x,
                                     typename std::decay<basic_string_view<CharT>>::type y) noexcept {
        return x.size() == y.size() &&
               (x.empty() ||
                ABEL_INTERNAL_STRING_VIEW_MEMCMP(x.data(), y.data(), x.size()) == 0);
    }

    template<typename T>
    constexpr bool operator!=(basic_string_view<T> x, basic_string_view<T> y) noexcept {
        return !(x == y);
    }

    template<typename T>
    constexpr bool operator!=(basic_string_view<T> x, typename std::decay<basic_string_view<T>>::type y) noexcept {
        return !(x == y);
    }

    template<typename T>
    constexpr bool operator!=(typename std::decay<basic_string_view<T>>::type x, basic_string_view<T> y) noexcept {
        return !(x == y);
    }

    template<typename T>
    constexpr bool operator!=(typename std::decay<basic_string_view<T>>::type x,
                              typename std::decay<basic_string_view<T>>::type y) noexcept {
        return !(x == y);
    }

    template<typename T>
    constexpr bool operator<(basic_string_view<T> x, basic_string_view<T> y) noexcept {
        return x.compare(y) < 0;
    }

    template<typename T>
    constexpr bool operator>(basic_string_view<T> x, basic_string_view<T> y) noexcept {
        return y < x;
    }

    template<typename T>
    constexpr bool operator<=(basic_string_view<T> x, basic_string_view<T> y) noexcept {
        return !(y < x);
    }

    template<typename T>
    constexpr bool operator>=(basic_string_view<T> x, basic_string_view<T> y) noexcept {
        return !(x < y);
    }

// IO Insertion Operator
    template<typename T>
    std::ostream &operator<<(std::ostream &o, basic_string_view<T> piece);

}  // namespace abel

#undef ABEL_INTERNAL_STRING_VIEW_MEMCMP

#endif  // ABEL_USES_STD_STRING_VIEW

namespace abel {

// clipped_substr()
//
// Like `s.substr(pos, n)`, but clips `pos` to an upper bound of `s.size()`.
// Provided because std::basic_string_view::substr throws if `pos > size()`
    template<typename T>
    ABEL_FORCE_INLINE basic_string_view<T> clipped_substr(basic_string_view<T> s, size_t pos,
                                                          size_t n = basic_string_view<T>::npos) {
        pos = (std::min)(pos, static_cast<size_t>(s.size()));
        return s.substr(pos, n);
    }

// null_safe_string_view()
//
// Creates an `abel::basic_string_view` from a pointer `p` even if it's null-valued.
// This function should be used where an `abel::basic_string_view` can be created from
// a possibly-null pointer.
    ABEL_FORCE_INLINE string_view null_safe_string_view(const char *p) {
        return p ? string_view(p) : string_view();
    }

}  // namespace abel


namespace abel {

    namespace {
        inline void WritePadding(std::ostream &o, size_t pad) {
            char fill_buf[32];
            memset(fill_buf, o.fill(), sizeof(fill_buf));
            while (pad) {
                size_t n = std::min(pad, sizeof(fill_buf));
                o.write(fill_buf, n);
                pad -= n;
            }
        }

        template<typename T>
        class LookupTable {
        public:
            // For each character in wanted, sets the index corresponding
            // to the ASCII code of that character. This is used by
            // the find_.*_of methods below to tell whether or not a character is in
            // the lookup table in constant time.
            explicit LookupTable(basic_string_view<T> wanted) {
                for (T c : wanted) {
                    table_[Index(c)] = true;
                }
            }

            bool operator[](T c) const { return table_[Index(c)]; }

        private:
            static unsigned char Index(T c) { return static_cast<unsigned char>(c); }

            bool table_[UCHAR_MAX + 1] = {};
        };

    }  // namespace

    inline std::ostream &operator<<(std::ostream &o, string_view piece) {
        std::ostream::sentry sentry(o);
        if (sentry) {
            size_t lpad = 0;
            size_t rpad = 0;
            if (static_cast<size_t>(o.width()) > piece.size()) {
                size_t pad = o.width() - piece.size();
                if ((o.flags() & o.adjustfield) == o.left) {
                    rpad = pad;
                } else {
                    lpad = pad;
                }
            }
            if (lpad)
                WritePadding(o, lpad);
            o.write(piece.data(), piece.size());
            if (rpad)
                WritePadding(o, rpad);
            o.width(0);
        }
        return o;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find(basic_string_view<T> s,
                                                                               basic_string_view<T>::size_type pos) const noexcept {
        if (empty() || pos > length_) {
            if (empty() && pos == 0 && s.empty())
                return 0;
            return npos;
        }
        const value_type *result =
                static_cast<const value_type *>(strings_internal::char_match(ptr_ + pos, length_ - pos, s.ptr_,
                                                                             s.length_));
        return result ? result - ptr_ : npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find(value_type c,
                                                                               size_type pos) const noexcept {
        if (empty() || pos >= length_) {
            return npos;
        }
        const value_type *result =
                static_cast<const value_type *>(memchr(ptr_ + pos, c, length_ - pos));
        return result != nullptr ? result - ptr_ : npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::rfind(basic_string_view<T> s,
                                                                                size_type pos) const
    noexcept {
        if (length_ < s.length_)
            return npos;
        if (s.empty())
            return std::min(length_, pos);
        const value_type *last = ptr_ + std::min(length_ - s.length_, pos) + s.length_;
        const value_type *result = std::find_end(ptr_, last, s.ptr_, s.ptr_ + s.length_);
        return result != last ? result - ptr_ : npos;
    }

// Search range is [0..pos] inclusive.  If pos == npos, search everything.
    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::rfind(value_type c, size_type pos) const
    noexcept {
        // Note: memrchr() is not available on Windows.
        if (empty())
            return npos;
        for (size_type i = std::min(pos, length_ - 1);; --i) {
            if (ptr_[i] == c) {
                return i;
            }
            if (i == 0)
                break;
        }
        return npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find_first_of(basic_string_view<T> s,
                                                                                        size_type pos) const
    noexcept {
        if (empty() || s.empty()) {
            return npos;
        }
        // Avoid the cost of LookupTable() for a single-character search.
        if (s.length_ == 1)
            return find_first_of(s.ptr_[0], pos);
        LookupTable<value_type> tbl(s);
        for (size_type i = pos; i < length_; ++i) {
            if (tbl[ptr_[i]]) {
                return i;
            }
        }
        return npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find_first_not_of(basic_string_view<T> s,
                                                                                            size_type pos) const
    noexcept {
        if (empty())
            return npos;
        // Avoid the cost of LookupTable() for a single-character search.
        if (s.length_ == 1)
            return find_first_not_of(s.ptr_[0], pos);
        LookupTable<value_type> tbl(s);
        for (size_type i = pos; i < length_; ++i) {
            if (!tbl[ptr_[i]]) {
                return i;
            }
        }
        return npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find_first_not_of(value_type c,
                                                                                            size_type pos) const
    noexcept {
        if (empty())
            return npos;
        for (; pos < length_; ++pos) {
            if (ptr_[pos] != c) {
                return pos;
            }
        }
        return npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find_last_of(basic_string_view<T> s,
                                                                                       size_type pos) const noexcept {
        if (empty() || s.empty())
            return npos;
        // Avoid the cost of LookupTable() for a single-character search.
        if (s.length_ == 1)
            return find_last_of(s.ptr_[0], pos);
        LookupTable<value_type> tbl(s);
        for (size_type i = std::min(pos, length_ - 1);; --i) {
            if (tbl[ptr_[i]]) {
                return i;
            }
            if (i == 0)
                break;
        }
        return npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find_last_not_of(basic_string_view<T> s,
                                                                                           size_type pos) const
    noexcept {
        if (empty())
            return npos;
        size_type i = std::min(pos, length_ - 1);
        if (s.empty())
            return i;
        // Avoid the cost of LookupTable() for a single-character search.
        if (s.length_ == 1)
            return find_last_not_of(s.ptr_[0], pos);
        LookupTable<value_type> tbl(s);
        for (;; --i) {
            if (!tbl[ptr_[i]]) {
                return i;
            }
            if (i == 0)
                break;
        }
        return npos;
    }

    template<typename T>
    inline typename basic_string_view<T>::size_type basic_string_view<T>::find_last_not_of(value_type c,
                                                                                           size_type pos) const
    noexcept {
        if (empty())
            return npos;
        size_type i = std::min(pos, length_ - 1);
        for (;; --i) {
            if (ptr_[i] != c) {
                return i;
            }
            if (i == 0)
                break;
        }
        return npos;
    }

// MSVC has non-standard behavior that implicitly creates definitions for static
// const members. These implicit definitions conflict with explicit out-of-class
// member definitions that are required by the C++ standard, resulting in
// LNK1169 "multiply defined" errors at link time. __declspec(selectany) asks
// MSVC to choose only one definition for the symbol it decorates. See details
// at https://msdn.microsoft.com/en-us/library/34h23df8(v=vs.100).aspx
#ifdef _MSC_VER
#define ABEL_STRING_VIEW_SELECTANY __declspec(selectany)
#else
#define ABEL_STRING_VIEW_SELECTANY
#endif

    ABEL_STRING_VIEW_SELECTANY
    template<typename T>
    constexpr typename basic_string_view<T>::size_type basic_string_view<T>::npos;
    ABEL_STRING_VIEW_SELECTANY
    template<typename T>
    constexpr typename basic_string_view<T>::size_type basic_string_view<T>::kMaxSize;

}  // namespace abel

namespace std {

    template<>
    struct hash<abel::string_view> {
        size_t operator()(const abel::string_view &x) const {
            abel::string_view::const_iterator p = x.cbegin();
            abel::string_view::const_iterator end = x.cend();
            uint32_t result = 2166136261U; // We implement an FNV-like string hash.
            while (p != end)
                result = (result * 16777619) ^ (uint8_t) *p++;
            return (size_t) result;
        }
    };

}

#endif  // ABEL_STRINGS_STRING_VIEW_H_
