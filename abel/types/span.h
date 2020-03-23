//
//
// -----------------------------------------------------------------------------
// span.h
// -----------------------------------------------------------------------------
//
// This header file defines a `span<T>` type for holding a view of an existing
// array of data. The `span` object, much like the `abel::string_view` object,
// does not own such data itself. A span provides a lightweight way to pass
// around view of such data.
//
// Additionally, this header file defines `make_span()` and `make_const_span()`
// factory functions, for clearly creating spans of type `span<T>` or read-only
// `span<const T>` when such types may be difficult to identify due to issues
// with implicit conversion.
//
// The C++ standards committee currently has a proposal for a `std::span` type,
// (http://wg21.link/p0122), which is not yet part of the standard (though may
// become part of C++20). As of August 2017, the differences between
// `abel::span` and this proposal are:
//    * `abel::span` uses `size_t` for `size_type`
//    * `abel::span` has no `operator()`
//    * `abel::span` has no constructors for `std::unique_ptr` or
//      `std::shared_ptr`
//    * `abel::span` has the factory functions `make_span()` and
//      `make_const_span()`
//    * `abel::span` has `front()` and `back()` methods
//    * bounds-checked access to `abel::span` is accomplished with `at()`
//    * `abel::span` has compiler-provided move and copy constructors and
//      assignment. This is due to them being specified as `constexpr`, but that
//      implies const in C++11.
//    * `abel::span` has no `element_type` or `index_type` typedefs
//    * A read-only `abel::span<const T>` can be implicitly constructed from an
//      initializer list.
//    * `abel::span` has no `bytes()`, `size_bytes()`, `as_bytes()`, or
//      `as_mutable_bytes()` methods
//    * `abel::span` has no static extent template parameter, nor constructors
//      which exist only because of the static extent parameter.
//    * `abel::span` has an explicit mutable-reference constructor
//
// For more information, see the class comments below.
#ifndef ABEL_TYPES_SPAN_H_
#define ABEL_TYPES_SPAN_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
#include <abel/base/throw_delegate.h>
#include <abel/base/profile.h>
#include <abel/asl/type_traits.h>
#include <abel/types/internal/span.h>

namespace abel {


//------------------------------------------------------------------------------
// span
//------------------------------------------------------------------------------
//
// A `span` is an "array view" type for holding a view of a contiguous data
// array; the `span` object does not and cannot own such data itself. A span
// provides an easy way to provide overloads for anything operating on
// contiguous sequences without needing to manage pointers and array lengths
// manually.

// A span is conceptually a pointer (ptr) and a length (size) into an already
// existing array of contiguous memory; the array it represents references the
// elements "ptr[0] .. ptr[size-1]". Passing a properly-constructed `span`
// instead of raw pointers avoids many issues related to index out of bounds
// errors.
//
// Spans may also be constructed from containers holding contiguous sequences.
// Such containers must supply `data()` and `size() const` methods (e.g
// `std::vector<T>`, `abel::InlinedVector<T, N>`). All implicit conversions to
// `abel::span` from such containers will create spans of type `const T`;
// spans which can mutate their values (of type `T`) must use explicit
// constructors.
//
// A `span<T>` is somewhat analogous to an `abel::string_view`, but for an array
// of elements of type `T`. A user of `span` must ensure that the data being
// pointed to outlives the `span` itself.
//
// You can construct a `span<T>` in several ways:
//
//   * Explicitly from a reference to a container type
//   * Explicitly from a pointer and size
//   * Implicitly from a container type (but only for spans of type `const T`)
//   * Using the `make_span()` or `make_const_span()` factory functions.
//
// Examples:
//
//   // Construct a span explicitly from a container:
//   std::vector<int> v = {1, 2, 3, 4, 5};
//   auto span = abel::span<const int>(v);
//
//   // Construct a span explicitly from a C-style array:
//   int a[5] =  {1, 2, 3, 4, 5};
//   auto span = abel::span<const int>(a);
//
//   // Construct a span implicitly from a container
//   void MyRoutine(abel::span<const int> a) {
//     ...
//   }
//   std::vector v = {1,2,3,4,5};
//   MyRoutine(v)                     // convert to span<const T>
//
// Note that `span` objects, in addition to requiring that the memory they
// point to remains alive, must also ensure that such memory does not get
// reallocated. Therefore, to avoid undefined behavior, containers with
// associated span views should not invoke operations that may reallocate memory
// (such as resizing) or invalidate iterators into the container.
//
// One common use for a `span` is when passing arguments to a routine that can
// accept a variety of array types (e.g. a `std::vector`, `abel::InlinedVector`,
// a C-style array, etc.). Instead of creating overloads for each case, you
// can simply specify a `span` as the argument to such a routine.
//
// Example:
//
//   void MyRoutine(abel::span<const int> a) {
//     ...
//   }
//
//   std::vector v = {1,2,3,4,5};
//   MyRoutine(v);
//
//   abel::InlinedVector<int, 4> my_inline_vector;
//   MyRoutine(my_inline_vector);
//
//   // Explicit constructor from pointer,size
//   int* my_array = new int[10];
//   MyRoutine(abel::span<const int>(my_array, 10));
    template<typename T>
    class span {
    private:
        // Used to determine whether a span can be constructed from a container of
        // type C.
        template<typename C>
        using EnableIfConvertibleFrom =
        typename std::enable_if<span_internal::HasData<T, C>::value &&
                                span_internal::HasSize<C>::value>::type;

        // Used to SFINAE-enable a function when the slice elements are const.
        template<typename U>
        using EnableIfConstView =
        typename std::enable_if<std::is_const<T>::value, U>::type;

        // Used to SFINAE-enable a function when the slice elements are mutable.
        template<typename U>
        using EnableIfMutableView =
        typename std::enable_if<!std::is_const<T>::value, U>::type;

    public:
        using value_type = abel::remove_cv_t<T>;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        static const size_type npos = ~(size_type(0));

        constexpr span() noexcept : span(nullptr, 0) {}

        constexpr span(pointer array, size_type length) noexcept
                : ptr_(array), len_(length) {}

        // Implicit conversion constructors
        template<size_t N>
        constexpr span(T (&a)[N]) noexcept  // NOLINT(runtime/explicit)
                : span(a, N) {}

        // Explicit reference constructor for a mutable `span<T>` type. Can be
        // replaced with make_span() to infer the type parameter.
        template<typename V, typename = EnableIfConvertibleFrom<V>,
                typename = EnableIfMutableView<V>>
        explicit span(V &v) noexcept  // NOLINT(runtime/references)
                : span(span_internal::GetData(v), v.size()) {}

        // Implicit reference constructor for a read-only `span<const T>` type
        template<typename V, typename = EnableIfConvertibleFrom<V>,
                typename = EnableIfConstView<V>>
        constexpr span(const V &v) noexcept  // NOLINT(runtime/explicit)
                : span(span_internal::GetData(v), v.size()) {}

        // Implicit constructor from an initializer list, making it possible to pass a
        // brace-enclosed initializer list to a function expecting a `span`. Such
        // spans constructed from an initializer list must be of type `span<const T>`.
        //
        //   void Process(abel::span<const int> x);
        //   Process({1, 2, 3});
        //
        // Note that as always the array referenced by the span must outlive the span.
        // Since an initializer list constructor acts as if it is fed a temporary
        // array (cf. C++ standard [dcl.init.list]/5), it's safe to use this
        // constructor only when the `std::initializer_list` itself outlives the span.
        // In order to meet this requirement it's sufficient to ensure that neither
        // the span nor a copy of it is used outside of the expression in which it's
        // created:
        //
        //   // Assume that this function uses the array directly, not retaining any
        //   // copy of the span or pointer to any of its elements.
        //   void Process(abel::span<const int> ints);
        //
        //   // Okay: the std::initializer_list<int> will reference a temporary array
        //   // that isn't destroyed until after the call to Process returns.
        //   Process({ 17, 19 });
        //
        //   // Not okay: the storage used by the std::initializer_list<int> is not
        //   // allowed to be referenced after the first line.
        //   abel::span<const int> ints = { 17, 19 };
        //   Process(ints);
        //
        //   // Not okay for the same reason as above: even when the elements of the
        //   // initializer list expression are not temporaries the underlying array
        //   // is, so the initializer list must still outlive the span.
        //   const int foo = 17;
        //   abel::span<const int> ints = { foo };
        //   Process(ints);
        //
        template<typename LazyT = T,
                typename = EnableIfConstView<LazyT>>
        span(
                std::initializer_list<value_type> v) noexcept  // NOLINT(runtime/explicit)
                : span(v.begin(), v.size()) {}

        // Accessors

        // span::data()
        //
        // Returns a pointer to the span's underlying array of data (which is held
        // outside the span).
        constexpr pointer data() const noexcept { return ptr_; }

        // span::size()
        //
        // Returns the size of this span.
        constexpr size_type size() const noexcept { return len_; }

        // span::length()
        //
        // Returns the length (size) of this span.
        constexpr size_type length() const noexcept { return size(); }

        // span::empty()
        //
        // Returns a boolean indicating whether or not this span is considered empty.
        constexpr bool empty() const noexcept { return size() == 0; }

        // span::operator[]
        //
        // Returns a reference to the i'th element of this span.
        constexpr reference operator[](size_type i) const noexcept {
            // MSVC 2015 accepts this as constexpr, but not ptr_[i]
            return *(data() + i);
        }

        // span::at()
        //
        // Returns a reference to the i'th element of this span.
        constexpr reference at(size_type i) const {
            return ABEL_LIKELY(i < size())  //
                   ? *(data() + i)
                   : (throw_std_out_of_range(
                            "span::at failed bounds check"),
                            *(data() + i));
        }

        // span::front()
        //
        // Returns a reference to the first element of this span.
        constexpr reference front() const noexcept {
            return ABEL_ASSERT(size() > 0), *data();
        }

        // span::back()
        //
        // Returns a reference to the last element of this span.
        constexpr reference back() const noexcept {
            return ABEL_ASSERT(size() > 0), *(data() + size() - 1);
        }

        // span::begin()
        //
        // Returns an iterator to the first element of this span.
        constexpr iterator begin() const noexcept { return data(); }

        // span::cbegin()
        //
        // Returns a const iterator to the first element of this span.
        constexpr const_iterator cbegin() const noexcept { return begin(); }

        // span::end()
        //
        // Returns an iterator to the last element of this span.
        constexpr iterator end() const noexcept { return data() + size(); }

        // span::cend()
        //
        // Returns a const iterator to the last element of this span.
        constexpr const_iterator cend() const noexcept { return end(); }

        // span::rbegin()
        //
        // Returns a reverse iterator starting at the last element of this span.
        constexpr reverse_iterator rbegin() const noexcept {
            return reverse_iterator(end());
        }

        // span::crbegin()
        //
        // Returns a reverse const iterator starting at the last element of this span.
        constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        // span::rend()
        //
        // Returns a reverse iterator starting at the first element of this span.
        constexpr reverse_iterator rend() const noexcept {
            return reverse_iterator(begin());
        }

        // span::crend()
        //
        // Returns a reverse iterator starting at the first element of this span.
        constexpr const_reverse_iterator crend() const noexcept { return rend(); }

        // span mutations

        // span::remove_prefix()
        //
        // Removes the first `n` elements from the span.
        void remove_prefix(size_type n) noexcept {
            assert(size() >= n);
            ptr_ += n;
            len_ -= n;
        }

        // span::remove_suffix()
        //
        // Removes the last `n` elements from the span.
        void remove_suffix(size_type n) noexcept {
            assert(size() >= n);
            len_ -= n;
        }

        // span::subspan()
        //
        // Returns a `span` starting at element `pos` and of length `len`. Both `pos`
        // and `len` are of type `size_type` and thus non-negative. Parameter `pos`
        // must be <= size(). Any `len` value that points past the end of the span
        // will be trimmed to at most size() - `pos`. A default `len` value of `npos`
        // ensures the returned subspan continues until the end of the span.
        //
        // Examples:
        //
        //   std::vector<int> vec = {10, 11, 12, 13};
        //   abel::make_span(vec).subspan(1, 2);  // {11, 12}
        //   abel::make_span(vec).subspan(2, 8);  // {12, 13}
        //   abel::make_span(vec).subspan(1);     // {11, 12, 13}
        //   abel::make_span(vec).subspan(4);     // {}
        //   abel::make_span(vec).subspan(5);     // throws std::out_of_range
        constexpr span subspan(size_type pos = 0, size_type len = npos) const {
            return (pos <= size())
                   ? span(data() + pos, span_internal::Min(size() - pos, len))
                   : (throw_std_out_of_range("pos > size()"), span());
        }

        // span::first()
        //
        // Returns a `span` containing first `len` elements. Parameter `len` is of
        // type `size_type` and thus non-negative. `len` value must be <= size().
        //
        // Examples:
        //
        //   std::vector<int> vec = {10, 11, 12, 13};
        //   abel::make_span(vec).first(1);  // {10}
        //   abel::make_span(vec).first(3);  // {10, 11, 12}
        //   abel::make_span(vec).first(5);  // throws std::out_of_range
        constexpr span first(size_type len) const {
            return (len <= size())
                   ? span(data(), len)
                   : (throw_std_out_of_range("len > size()"), span());
        }

        // span::last()
        //
        // Returns a `span` containing last `len` elements. Parameter `len` is of
        // type `size_type` and thus non-negative. `len` value must be <= size().
        //
        // Examples:
        //
        //   std::vector<int> vec = {10, 11, 12, 13};
        //   abel::make_span(vec).last(1);  // {13}
        //   abel::make_span(vec).last(3);  // {11, 12, 13}
        //   abel::make_span(vec).last(5);  // throws std::out_of_range
        constexpr span last(size_type len) const {
            return (len <= size())
                   ? span(size() - len + data(), len)
                   : (throw_std_out_of_range("len > size()"), span());
        }

        // Support for abel::hash.
        template<typename H>
        friend H abel_hash_value(H h, span v) {
            return H::combine(H::combine_contiguous(std::move(h), v.data(), v.size()),
                              v.size());
        }

    private:
        pointer ptr_;
        size_type len_;
    };

    template<typename T>
    const typename span<T>::size_type span<T>::npos;

// span relationals

// Equality is compared element-by-element, while ordering is lexicographical.
// We provide three overloads for each operator to cover any combination on the
// left or right hand side of mutable span<T>, read-only span<const T>, and
// convertible-to-read-only span<T>.
// TODO(zhangxy): Due to MSVC overload resolution bug with partial ordering
// template functions, 5 overloads per operator is needed as a workaround. We
// should update them to 3 overloads per operator using non-deduced context like
// string_view, i.e.
// - (span<T>, span<T>)
// - (span<T>, non_deduced<span<const T>>)
// - (non_deduced<span<const T>>, span<T>)

// operator==
    template<typename T>
    bool operator==(span<T> a, span<T> b) {
        return span_internal::EqualImpl<span, const T>(a, b);
    }

    template<typename T>
    bool operator==(span<const T> a, span<T> b) {
        return span_internal::EqualImpl<span, const T>(a, b);
    }

    template<typename T>
    bool operator==(span<T> a, span<const T> b) {
        return span_internal::EqualImpl<span, const T>(a, b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator==(const U &a, span<T> b) {
        return span_internal::EqualImpl<span, const T>(a, b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator==(span<T> a, const U &b) {
        return span_internal::EqualImpl<span, const T>(a, b);
    }

// operator!=
    template<typename T>
    bool operator!=(span<T> a, span<T> b) {
        return !(a == b);
    }

    template<typename T>
    bool operator!=(span<const T> a, span<T> b) {
        return !(a == b);
    }

    template<typename T>
    bool operator!=(span<T> a, span<const T> b) {
        return !(a == b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator!=(const U &a, span<T> b) {
        return !(a == b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator!=(span<T> a, const U &b) {
        return !(a == b);
    }

// operator<
    template<typename T>
    bool operator<(span<T> a, span<T> b) {
        return span_internal::LessThanImpl<span, const T>(a, b);
    }

    template<typename T>
    bool operator<(span<const T> a, span<T> b) {
        return span_internal::LessThanImpl<span, const T>(a, b);
    }

    template<typename T>
    bool operator<(span<T> a, span<const T> b) {
        return span_internal::LessThanImpl<span, const T>(a, b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator<(const U &a, span<T> b) {
        return span_internal::LessThanImpl<span, const T>(a, b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator<(span<T> a, const U &b) {
        return span_internal::LessThanImpl<span, const T>(a, b);
    }

// operator>
    template<typename T>
    bool operator>(span<T> a, span<T> b) {
        return b < a;
    }

    template<typename T>
    bool operator>(span<const T> a, span<T> b) {
        return b < a;
    }

    template<typename T>
    bool operator>(span<T> a, span<const T> b) {
        return b < a;
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator>(const U &a, span<T> b) {
        return b < a;
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator>(span<T> a, const U &b) {
        return b < a;
    }

// operator<=
    template<typename T>
    bool operator<=(span<T> a, span<T> b) {
        return !(b < a);
    }

    template<typename T>
    bool operator<=(span<const T> a, span<T> b) {
        return !(b < a);
    }

    template<typename T>
    bool operator<=(span<T> a, span<const T> b) {
        return !(b < a);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator<=(const U &a, span<T> b) {
        return !(b < a);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator<=(span<T> a, const U &b) {
        return !(b < a);
    }

// operator>=
    template<typename T>
    bool operator>=(span<T> a, span<T> b) {
        return !(a < b);
    }

    template<typename T>
    bool operator>=(span<const T> a, span<T> b) {
        return !(a < b);
    }

    template<typename T>
    bool operator>=(span<T> a, span<const T> b) {
        return !(a < b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator>=(const U &a, span<T> b) {
        return !(a < b);
    }

    template<
            typename T, typename U,
            typename = span_internal::EnableIfConvertibleTo<U, abel::span<const T>>>
    bool operator>=(span<T> a, const U &b) {
        return !(a < b);
    }

// make_span()
//
// Constructs a mutable `span<T>`, deducing `T` automatically from either a
// container or pointer+size.
//
// Because a read-only `span<const T>` is implicitly constructed from container
// types regardless of whether the container itself is a const container,
// constructing mutable spans of type `span<T>` from containers requires
// explicit constructors. The container-accepting version of `make_span()`
// deduces the type of `T` by the constness of the pointer received from the
// container's `data()` member. Similarly, the pointer-accepting version returns
// a `span<const T>` if `T` is `const`, and a `span<T>` otherwise.
//
// Examples:
//
//   void MyRoutine(abel::span<MyComplicatedType> a) {
//     ...
//   };
//   // my_vector is a container of non-const types
//   std::vector<MyComplicatedType> my_vector;
//
//   // Constructing a span implicitly attempts to create a span of type
//   // `span<const T>`
//   MyRoutine(my_vector);                // error, type mismatch
//
//   // Explicitly constructing the span is verbose
//   MyRoutine(abel::span<MyComplicatedType>(my_vector));
//
//   // Use make_span() to make an abel::span<T>
//   MyRoutine(abel::make_span(my_vector));
//
//   // Construct a span from an array ptr+size
//   abel::span<T> my_span() {
//     return abel::make_span(&array[0], num_elements_);
//   }
//
    template<int &... ExplicitArgumentBarrier, typename T>
    constexpr span<T> make_span(T *ptr, size_t size) noexcept {
        return span<T>(ptr, size);
    }

    template<int &... ExplicitArgumentBarrier, typename T>
    span<T> make_span(T *begin, T *end) noexcept {
        return ABEL_ASSERT(begin <= end), span<T>(begin, end - begin);
    }

    template<int &... ExplicitArgumentBarrier, typename C>
    constexpr auto make_span(C &c) noexcept  // NOLINT(runtime/references)
    -> decltype(abel::make_span(span_internal::GetData(c), c.size())) {
        return make_span(span_internal::GetData(c), c.size());
    }

    template<int &... ExplicitArgumentBarrier, typename T, size_t N>
    constexpr span<T> make_span(T (&array)[N]) noexcept {
        return span<T>(array, N);
    }

// make_const_span()
//
// Constructs a `span<const T>` as with `make_span`, deducing `T` automatically,
// but always returning a `span<const T>`.
//
// Examples:
//
//   void ProcessInts(abel::span<const int> some_ints);
//
//   // Call with a pointer and size.
//   int array[3] = { 0, 0, 0 };
//   ProcessInts(abel::make_const_span(&array[0], 3));
//
//   // Call with a [begin, end) pair.
//   ProcessInts(abel::make_const_span(&array[0], &array[3]));
//
//   // Call directly with an array.
//   ProcessInts(abel::make_const_span(array));
//
//   // Call with a contiguous container.
//   std::vector<int> some_ints = ...;
//   ProcessInts(abel::make_const_span(some_ints));
//   ProcessInts(abel::make_const_span(std::vector<int>{ 0, 0, 0 }));
//
    template<int &... ExplicitArgumentBarrier, typename T>
    constexpr span<const T> make_const_span(T *ptr, size_t size) noexcept {
        return span<const T>(ptr, size);
    }

    template<int &... ExplicitArgumentBarrier, typename T>
    span<const T> make_const_span(T *begin, T *end) noexcept {
        return ABEL_ASSERT(begin <= end), span<const T>(begin, end - begin);
    }

    template<int &... ExplicitArgumentBarrier, typename C>
    constexpr auto make_const_span(const C &c) noexcept -> decltype(make_span(c)) {
        return make_span(c);
    }

    template<int &... ExplicitArgumentBarrier, typename T, size_t N>
    constexpr span<const T> make_const_span(const T (&array)[N]) noexcept {
        return span<const T>(array, N);
    }

}  // namespace abel
#endif  // ABEL_TYPES_SPAN_H_
