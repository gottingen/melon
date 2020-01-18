//

#ifndef ABEL_BASE_INTERNAL_HIDE_PTR_H_
#define ABEL_BASE_INTERNAL_HIDE_PTR_H_

#include <cstdint>

#include <abel/base/profile.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {

// Arbitrary value with high bits set. Xor'ing with it is unlikely
// to map one valid pointer to another valid pointer.
constexpr uintptr_t HideMask() {
  return (uintptr_t{0xF03A5F7BU} << (sizeof(uintptr_t) - 4) * 8) | 0xF03A5F7BU;
}

// Hide a pointer from the leak checker. For internal use only.
// Differs from abel::IgnoreLeak(ptr) in that abel::IgnoreLeak(ptr) causes ptr
// and all objects reachable from ptr to be ignored by the leak checker.
template <class T>
ABEL_FORCE_INLINE uintptr_t HidePtr(T* ptr) {
  return reinterpret_cast<uintptr_t>(ptr) ^ HideMask();
}

// Return a pointer that has been hidden from the leak checker.
// For internal use only.
template <class T>
ABEL_FORCE_INLINE T* UnhidePtr(uintptr_t hidden) {
  return reinterpret_cast<T*>(hidden ^ HideMask());
}

}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_HIDE_PTR_H_
