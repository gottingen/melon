//

#ifndef ABEL_HASH_INTERNAL_SPY_HASH_STATE_H_
#define ABEL_HASH_INTERNAL_SPY_HASH_STATE_H_

#include <ostream>
#include <string>
#include <vector>

#include <abel/hash/hash.h>
#include <abel/strings/ends_with.h>
#include <abel/format/printf.h>
#include <abel/strings/str_join.h>

namespace abel {

    namespace hash_internal {

// SpyHashState is an implementation of the hash_state API that simply
// accumulates all input bytes in an internal buffer. This makes it useful
// for testing abel_hash_value overloads (so long as they are templated on the
// hash_state parameter), since it can report the exact hash representation
// that the abel_hash_value overload produces.
//
// Sample usage:
// EXPECT_EQ(SpyHashState::combine(SpyHashState(), foo),
//           SpyHashState::combine(SpyHashState(), bar));
        template<typename T>
        class spy_hash_state_impl : public hash_state_base<spy_hash_state_impl<T>> {
        public:
            spy_hash_state_impl() : error_(std::make_shared<abel::optional<std::string>>()) {
                static_assert(std::is_void<T>::value, "");
            }

            // Move-only
            spy_hash_state_impl(const spy_hash_state_impl &) = delete;

            spy_hash_state_impl &operator=(const spy_hash_state_impl &) = delete;

            spy_hash_state_impl(spy_hash_state_impl &&other) noexcept {
                *this = std::move(other);
            }

            spy_hash_state_impl &operator=(spy_hash_state_impl &&other) noexcept {
                hash_representation_ = std::move(other.hash_representation_);
                error_ = other.error_;
                moved_from_ = other.moved_from_;
                other.moved_from_ = true;
                return *this;
            }

            template<typename U>
            spy_hash_state_impl(spy_hash_state_impl<U> &&other) {  // NOLINT
                hash_representation_ = std::move(other.hash_representation_);
                error_ = other.error_;
                moved_from_ = other.moved_from_;
                other.moved_from_ = true;
            }

            template<typename A, typename... Args>
            static spy_hash_state_impl combine(spy_hash_state_impl s, const A &a,
                                            const Args &... args) {
                // Pass an instance of spy_hash_state_impl<A> when trying to combine `A`. This
                // allows us to test that the user only uses this instance for combine calls
                // and does not call abel_hash_value directly.
                // See abel_hash_value implementation at the bottom.
                s = spy_hash_state_impl<A>::hash_state_base::combine(std::move(s), a);
                return spy_hash_state_impl::combine(std::move(s), args...);
            }

            static spy_hash_state_impl combine(spy_hash_state_impl s) {
                if (direct_abel_hash_value_error_) {
                    *s.error_ = "abel_hash_value should not be invoked directly.";
                } else if (s.moved_from_) {
                    *s.error_ = "Used moved-from instance of the hash state object.";
                }
                return s;
            }

            static void SetDirectAbelHashValueError() {
                direct_abel_hash_value_error_ = true;
            }

            // Two spy_hash_state_impl objects are equal if they hold equal hash
            // representations.
            friend bool operator==(const spy_hash_state_impl &lhs,
                                   const spy_hash_state_impl &rhs) {
                return lhs.hash_representation_ == rhs.hash_representation_;
            }

            friend bool operator!=(const spy_hash_state_impl &lhs,
                                   const spy_hash_state_impl &rhs) {
                return !(lhs == rhs);
            }

            enum class CompareResult {
                kEqual,
                kASuffixB,
                kBSuffixA,
                kUnequal,
            };

            static CompareResult Compare(const spy_hash_state_impl &a,
                                         const spy_hash_state_impl &b) {
                const std::string a_flat = abel::string_join(a.hash_representation_, "");
                const std::string b_flat = abel::string_join(b.hash_representation_, "");
                if (a_flat == b_flat) return CompareResult::kEqual;
                if (abel::ends_with(a_flat, b_flat)) return CompareResult::kBSuffixA;
                if (abel::ends_with(b_flat, a_flat)) return CompareResult::kASuffixB;
                return CompareResult::kUnequal;
            }

            // operator<< prints the hash representation as a hex and ASCII dump, to
            // facilitate debugging.
            friend std::ostream &operator<<(std::ostream &out,
                                            const spy_hash_state_impl &hash_state) {
                out << "[\n";
                for (auto &s : hash_state.hash_representation_) {
                    size_t offset = 0;
                    for (char c : s) {
                        if (offset % 16 == 0) {
                            out << fmt::sprintf("\n0x%04x: ", offset);
                        }
                        if (offset % 2 == 0) {
                            out << " ";
                        }
                        out << fmt::sprintf("%02x", c);
                        ++offset;
                    }
                    out << "\n";
                }
                return out << "]";
            }

            // The base case of the combine recursion, which writes raw bytes into the
            // internal buffer.
            static spy_hash_state_impl combine_contiguous(spy_hash_state_impl hash_state,
                                                       const unsigned char *begin,
                                                       size_t size) {
                const size_t large_chunk_stride = PiecewiseChunkSize();
                if (size > large_chunk_stride) {
                    // Combining a large contiguous buffer must have the same effect as
                    // doing it piecewise by the stride length, followed by the (possibly
                    // empty) remainder.
                    while (size >= large_chunk_stride) {
                        hash_state = spy_hash_state_impl::combine_contiguous(
                                std::move(hash_state), begin, large_chunk_stride);
                        begin += large_chunk_stride;
                        size -= large_chunk_stride;
                    }
                }

                hash_state.hash_representation_.emplace_back(
                        reinterpret_cast<const char *>(begin), size);
                return hash_state;
            }

            using spy_hash_state_impl::hash_state_base::combine_contiguous;

            abel::optional<std::string> error() const {
                if (moved_from_) {
                    return "Returned a moved-from instance of the hash state object.";
                }
                return *error_;
            }

        private:
            template<typename U>
            friend
            class spy_hash_state_impl;

            // This is true if spy_hash_state_impl<T> has been passed to a call of
            // abel_hash_value with the wrong type. This detects that the user called
            // abel_hash_value directly (because the hash state type does not match).
            static bool direct_abel_hash_value_error_;

            std::vector<std::string> hash_representation_;
            // This is a shared_ptr because we want all instances of the particular
            // SpyHashState run to share the field. This way we can set the error for
            // use-after-move and all the copies will see it.
            std::shared_ptr<abel::optional<std::string>> error_;
            bool moved_from_ = false;
        };

        template<typename T>
        bool spy_hash_state_impl<T>::direct_abel_hash_value_error_;

        template<bool &B>
        struct OdrUse {
            constexpr OdrUse() {}

            bool &b = B;
        };

        template<void (*)()>
        struct RunOnStartup {
            static bool run;
            static constexpr OdrUse<run> kOdrUse{};
        };

        template<void (*f)()>
        bool RunOnStartup<f>::run = (f(), true);

        template<
                typename T, typename U,
                // Only trigger for when (T != U),
                typename = abel::enable_if_t<!std::is_same<T, U>::value>,
                // This statement works in two ways:
                //  - First, it instantiates RunOnStartup and forces the initialization of
                //    `run`, which set the global variable.
                //  - Second, it triggers a SFINAE error disabling the overload to prevent
                //    compile time errors. If we didn't disable the overload we would get
                //    ambiguous overload errors, which we don't want.
                int = RunOnStartup<spy_hash_state_impl<T>::SetDirectAbelHashValueError>::run>
        void abel_hash_value(spy_hash_state_impl<T>, const U &);

        using SpyHashState = spy_hash_state_impl<void>;

    }  // namespace hash_internal

}  // namespace abel

#endif  // ABEL_HASH_INTERNAL_SPY_HASH_STATE_H_
