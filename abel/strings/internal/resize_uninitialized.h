//
//

#ifndef ABEL_STRINGS_INTERNAL_RESIZE_UNINITIALIZED_H_
#define ABEL_STRINGS_INTERNAL_RESIZE_UNINITIALIZED_H_

#include <string>
#include <type_traits>
#include <utility>

#include <abel/base/profile.h>
#include <abel/meta/type_traits.h>  //  for void_t

namespace abel {

namespace strings_internal {

// Is a subclass of true_type or false_type, depending on whether or not
// T has a __resize_default_init member.
template <typename string_type, typename = void>
struct ResizeUninitializedTraits {
  using HasMember = std::false_type;
  static void Resize(string_type* s, size_t new_size) { s->resize(new_size); }
};

// __resize_default_init is provided by libc++ >= 8.0
template <typename string_type>
struct ResizeUninitializedTraits<
    string_type, abel::void_t<decltype(std::declval<string_type&>()
                                           .__resize_default_init(237))> > {
  using HasMember = std::true_type;
  static void Resize(string_type* s, size_t new_size) {
    s->__resize_default_init(new_size);
  }
};

// Returns true if the std::string implementation supports a resize where
// the new characters added to the std::string are left untouched.
//
// (A better name might be "STLStringSupportsUninitializedResize", alluding to
// the previous function.)
template <typename string_type>
ABEL_FORCE_INLINE constexpr bool STLStringSupportsNontrashingResize(string_type*) {
  return ResizeUninitializedTraits<string_type>::HasMember::value;
}

// Like str->resize(new_size), except any new characters added to "*str" as a
// result of resizing may be left uninitialized, rather than being filled with
// '0' bytes. Typically used when code is then going to overwrite the backing
// store of the std::string with known data.
template <typename string_type, typename = void>
ABEL_FORCE_INLINE void STLStringResizeUninitialized(string_type* s, size_t new_size) {
  ResizeUninitializedTraits<string_type>::Resize(s, new_size);
}

}  // namespace strings_internal

}  // namespace abel

#endif  // ABEL_STRINGS_INTERNAL_RESIZE_UNINITIALIZED_H_
