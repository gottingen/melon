//
//

#ifndef ABEL_BASE_INTERNAL_UNALIGNED_ACCESS_H_
#define ABEL_BASE_INTERNAL_UNALIGNED_ACCESS_H_

#include <string.h>
#include <cstdint>
#include <abel/base/profile.h>

// unaligned APIs

// Portable handling of unaligned loads, stores, and copies.

// The unaligned API is C++ only.  The declarations use C++ features
// (namespaces, inline) which are absent or incompatible in C.
#if defined(__cplusplus)

#if defined(ADDRESS_SANITIZER) || defined(THREAD_SANITIZER) || \
    defined(MEMORY_SANITIZER)
// Consider we have an unaligned load/store of 4 bytes from address 0x...05.
// AddressSanitizer will treat it as a 3-byte access to the range 05:07 and
// will miss a bug if 08 is the first unaddressable byte.
// ThreadSanitizer will also treat this as a 3-byte access to 05:07 and will
// miss a race between this access and some other accesses to 08.
// MemorySanitizer will correctly propagate the shadow on unaligned stores
// and correctly report bugs on unaligned loads, but it may not properly
// update and report the origin of the uninitialized memory.
// For all three tools, replacing an unaligned access with a tool-specific
// callback solves the problem.

// Make sure uint16_t/uint32_t/uint64_t are defined.
#include <stdint.h>

extern "C" {
uint16_t __sanitizer_unaligned_load16(const void *p);
uint32_t __sanitizer_unaligned_load32(const void *p);
uint64_t __sanitizer_unaligned_load64(const void *p);
void __sanitizer_unaligned_store16(void *p, uint16_t v);
void __sanitizer_unaligned_store32(void *p, uint32_t v);
void __sanitizer_unaligned_store64(void *p, uint64_t v);
}  // extern "C"

namespace abel {

namespace base_internal {

ABEL_FORCE_INLINE uint16_t UnalignedLoad16(const void *p) {
  return __sanitizer_unaligned_load16(p);
}

ABEL_FORCE_INLINE uint32_t UnalignedLoad32(const void *p) {
  return __sanitizer_unaligned_load32(p);
}

ABEL_FORCE_INLINE uint64_t UnalignedLoad64(const void *p) {
  return __sanitizer_unaligned_load64(p);
}

ABEL_FORCE_INLINE void UnalignedStore16(void *p, uint16_t v) {
  __sanitizer_unaligned_store16(p, v);
}

ABEL_FORCE_INLINE void UnalignedStore32(void *p, uint32_t v) {
  __sanitizer_unaligned_store32(p, v);
}

ABEL_FORCE_INLINE void UnalignedStore64(void *p, uint64_t v) {
  __sanitizer_unaligned_store64(p, v);
}

}  // namespace base_internal

}  // namespace abel

#define ABEL_INTERNAL_UNALIGNED_LOAD16(_p) \
  (abel::base_internal::UnalignedLoad16(_p))
#define ABEL_INTERNAL_UNALIGNED_LOAD32(_p) \
  (abel::base_internal::UnalignedLoad32(_p))
#define ABEL_INTERNAL_UNALIGNED_LOAD64(_p) \
  (abel::base_internal::UnalignedLoad64(_p))

#define ABEL_INTERNAL_UNALIGNED_STORE16(_p, _val) \
  (abel::base_internal::UnalignedStore16(_p, _val))
#define ABEL_INTERNAL_UNALIGNED_STORE32(_p, _val) \
  (abel::base_internal::UnalignedStore32(_p, _val))
#define ABEL_INTERNAL_UNALIGNED_STORE64(_p, _val) \
  (abel::base_internal::UnalignedStore64(_p, _val))

#else

namespace abel {

    namespace base_internal {

        ABEL_FORCE_INLINE uint16_t UnalignedLoad16(const void *p) {
            uint16_t t;
            memcpy(&t, p, sizeof t);
            return t;
        }

        ABEL_FORCE_INLINE uint32_t UnalignedLoad32(const void *p) {
            uint32_t t;
            memcpy(&t, p, sizeof t);
            return t;
        }

        ABEL_FORCE_INLINE uint64_t UnalignedLoad64(const void *p) {
            uint64_t t;
            memcpy(&t, p, sizeof t);
            return t;
        }

        ABEL_FORCE_INLINE void UnalignedStore16(void *p, uint16_t v) { memcpy(p, &v, sizeof v); }

        ABEL_FORCE_INLINE void UnalignedStore32(void *p, uint32_t v) { memcpy(p, &v, sizeof v); }

        ABEL_FORCE_INLINE void UnalignedStore64(void *p, uint64_t v) { memcpy(p, &v, sizeof v); }

    }  // namespace base_internal

}  // namespace abel

#define ABEL_INTERNAL_UNALIGNED_LOAD16(_p) \
  (abel::base_internal::UnalignedLoad16(_p))
#define ABEL_INTERNAL_UNALIGNED_LOAD32(_p) \
  (abel::base_internal::UnalignedLoad32(_p))
#define ABEL_INTERNAL_UNALIGNED_LOAD64(_p) \
  (abel::base_internal::UnalignedLoad64(_p))

#define ABEL_INTERNAL_UNALIGNED_STORE16(_p, _val) \
  (abel::base_internal::UnalignedStore16(_p, _val))
#define ABEL_INTERNAL_UNALIGNED_STORE32(_p, _val) \
  (abel::base_internal::UnalignedStore32(_p, _val))
#define ABEL_INTERNAL_UNALIGNED_STORE64(_p, _val) \
  (abel::base_internal::UnalignedStore64(_p, _val))

#endif

#endif  // defined(__cplusplus), end of unaligned API

#endif  // ABEL_BASE_INTERNAL_UNALIGNED_ACCESS_H_
