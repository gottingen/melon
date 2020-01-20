//

#ifndef ABEL_BASE_INTERNAL_ENDIAN_H_
#define ABEL_BASE_INTERNAL_ENDIAN_H_

// The following guarantees declaration of the byte swap functions
#ifdef _MSC_VER
#include <stdlib.h>  // NOLINT(build/include)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#elif defined(__GLIBC__)
#include <byteswap.h>  // IWYU pragma: export
#endif

#include <cstdint>
#include <abel/base/profile.h>
#include <abel/base/internal/unaligned_access.h>
#include <abel/base/profile.h>

namespace abel {


// Use compiler byte-swapping intrinsics if they are available.  32-bit
// and 64-bit versions are available in Clang and GCC as of GCC 4.3.0.
// The 16-bit version is available in Clang and GCC only as of GCC 4.8.0.
// For simplicity, we enable them all only for GCC 4.8.0 or later.
#if defined(__clang__) || \
    (defined(__GNUC__) && \
     ((__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ >= 5))
ABEL_FORCE_INLINE uint64_t gbswap_64(uint64_t host_int) {
  return __builtin_bswap64(host_int);
}
ABEL_FORCE_INLINE uint32_t gbswap_32(uint32_t host_int) {
  return __builtin_bswap32(host_int);
}
ABEL_FORCE_INLINE uint16_t gbswap_16(uint16_t host_int) {
  return __builtin_bswap16(host_int);
}

#elif defined(_MSC_VER)
ABEL_FORCE_INLINE uint64_t gbswap_64(uint64_t host_int) {
  return _byteswap_uint64(host_int);
}
ABEL_FORCE_INLINE uint32_t gbswap_32(uint32_t host_int) {
  return _byteswap_ulong(host_int);
}
ABEL_FORCE_INLINE uint16_t gbswap_16(uint16_t host_int) {
  return _byteswap_ushort(host_int);
}

#else
ABEL_FORCE_INLINE uint64_t gbswap_64(uint64_t host_int) {
#if defined(__GNUC__) && defined(__x86_64__) && !defined(__APPLE__)
  // Adapted from /usr/include/byteswap.h.  Not available on Mac.
  if (__builtin_constant_p(host_int)) {
    return __bswap_constant_64(host_int);
  } else {
    uint64_t result;
    __asm__("bswap %0" : "=r"(result) : "0"(host_int));
    return result;
  }
#elif defined(__GLIBC__)
  return bswap_64(host_int);
#else
  return (((host_int & uint64_t{0xFF}) << 56) |
          ((host_int & uint64_t{0xFF00}) << 40) |
          ((host_int & uint64_t{0xFF0000}) << 24) |
          ((host_int & uint64_t{0xFF000000}) << 8) |
          ((host_int & uint64_t{0xFF00000000}) >> 8) |
          ((host_int & uint64_t{0xFF0000000000}) >> 24) |
          ((host_int & uint64_t{0xFF000000000000}) >> 40) |
          ((host_int & uint64_t{0xFF00000000000000}) >> 56));
#endif  // bswap_64
}

ABEL_FORCE_INLINE uint32_t gbswap_32(uint32_t host_int) {
#if defined(__GLIBC__)
  return bswap_32(host_int);
#else
  return (((host_int & uint32_t{0xFF}) << 24) |
          ((host_int & uint32_t{0xFF00}) << 8) |
          ((host_int & uint32_t{0xFF0000}) >> 8) |
          ((host_int & uint32_t{0xFF000000}) >> 24));
#endif
}

ABEL_FORCE_INLINE uint16_t gbswap_16(uint16_t host_int) {
#if defined(__GLIBC__)
  return bswap_16(host_int);
#else
  return (((host_int & uint16_t{0xFF}) << 8) |
          ((host_int & uint16_t{0xFF00}) >> 8));
#endif
}

#endif  // intrinsics available

#ifdef ABEL_SYSTEM_LITTLE_ENDIAN

// Definitions for ntohl etc. that don't require us to include
// netinet/in.h. We wrap gbswap_32 and gbswap_16 in functions rather
// than just #defining them because in debug mode, gcc doesn't
// correctly handle the (rather involved) definitions of bswap_32.
// gcc guarantees that ABEL_FORCE_INLINE functions are as fast as macros, so
// this isn't a performance hit.
ABEL_FORCE_INLINE uint16_t ghtons(uint16_t x) { return gbswap_16(x); }
ABEL_FORCE_INLINE uint32_t ghtonl(uint32_t x) { return gbswap_32(x); }
ABEL_FORCE_INLINE uint64_t ghtonll(uint64_t x) { return gbswap_64(x); }

#elif defined ABEL_SYSTEM_BIG_ENDIAN

// These definitions are simpler on big-endian machines
// These are functions instead of macros to avoid self-assignment warnings
// on calls such as "i = ghtnol(i);".  This also provides type checking.
ABEL_FORCE_INLINE uint16_t ghtons(uint16_t x) { return x; }
ABEL_FORCE_INLINE uint32_t ghtonl(uint32_t x) { return x; }
ABEL_FORCE_INLINE uint64_t ghtonll(uint64_t x) { return x; }

#else
#error \
    "Unsupported byte order: Either ABEL_SYSTEM_BIG_ENDIAN or " \
       "ABEL_SYSTEM_LITTLE_ENDIAN must be defined"
#endif  // byte order

ABEL_FORCE_INLINE uint16_t gntohs(uint16_t x) { return ghtons(x); }
ABEL_FORCE_INLINE uint32_t gntohl(uint32_t x) { return ghtonl(x); }
ABEL_FORCE_INLINE uint64_t gntohll(uint64_t x) { return ghtonll(x); }

// Utilities to convert numbers between the current hosts's native byte
// order and little-endian byte order
//
// Load/Store methods are alignment safe
namespace little_endian {
// Conversion functions.
#ifdef ABEL_SYSTEM_LITTLE_ENDIAN

ABEL_FORCE_INLINE uint16_t FromHost16(uint16_t x) { return x; }
ABEL_FORCE_INLINE uint16_t ToHost16(uint16_t x) { return x; }

ABEL_FORCE_INLINE uint32_t FromHost32(uint32_t x) { return x; }
ABEL_FORCE_INLINE uint32_t ToHost32(uint32_t x) { return x; }

ABEL_FORCE_INLINE uint64_t FromHost64(uint64_t x) { return x; }
ABEL_FORCE_INLINE uint64_t ToHost64(uint64_t x) { return x; }

ABEL_FORCE_INLINE constexpr bool IsLittleEndian() { return true; }

#elif defined ABEL_SYSTEM_BIG_ENDIAN

ABEL_FORCE_INLINE uint16_t FromHost16(uint16_t x) { return gbswap_16(x); }
ABEL_FORCE_INLINE uint16_t ToHost16(uint16_t x) { return gbswap_16(x); }

ABEL_FORCE_INLINE uint32_t FromHost32(uint32_t x) { return gbswap_32(x); }
ABEL_FORCE_INLINE uint32_t ToHost32(uint32_t x) { return gbswap_32(x); }

ABEL_FORCE_INLINE uint64_t FromHost64(uint64_t x) { return gbswap_64(x); }
ABEL_FORCE_INLINE uint64_t ToHost64(uint64_t x) { return gbswap_64(x); }

ABEL_FORCE_INLINE constexpr bool IsLittleEndian() { return false; }

#endif /* ENDIAN */

// Functions to do unaligned loads and stores in little-endian order.
ABEL_FORCE_INLINE uint16_t Load16(const void *p) {
  return ToHost16(ABEL_INTERNAL_UNALIGNED_LOAD16(p));
}

ABEL_FORCE_INLINE void Store16(void *p, uint16_t v) {
  ABEL_INTERNAL_UNALIGNED_STORE16(p, FromHost16(v));
}

ABEL_FORCE_INLINE uint32_t Load32(const void *p) {
  return ToHost32(ABEL_INTERNAL_UNALIGNED_LOAD32(p));
}

ABEL_FORCE_INLINE void Store32(void *p, uint32_t v) {
  ABEL_INTERNAL_UNALIGNED_STORE32(p, FromHost32(v));
}

ABEL_FORCE_INLINE uint64_t Load64(const void *p) {
  return ToHost64(ABEL_INTERNAL_UNALIGNED_LOAD64(p));
}

ABEL_FORCE_INLINE void Store64(void *p, uint64_t v) {
  ABEL_INTERNAL_UNALIGNED_STORE64(p, FromHost64(v));
}

}  // namespace little_endian

// Utilities to convert numbers between the current hosts's native byte
// order and big-endian byte order (same as network byte order)
//
// Load/Store methods are alignment safe
namespace big_endian {
#ifdef ABEL_SYSTEM_LITTLE_ENDIAN

ABEL_FORCE_INLINE uint16_t FromHost16(uint16_t x) { return gbswap_16(x); }
ABEL_FORCE_INLINE uint16_t ToHost16(uint16_t x) { return gbswap_16(x); }

ABEL_FORCE_INLINE uint32_t FromHost32(uint32_t x) { return gbswap_32(x); }
ABEL_FORCE_INLINE uint32_t ToHost32(uint32_t x) { return gbswap_32(x); }

ABEL_FORCE_INLINE uint64_t FromHost64(uint64_t x) { return gbswap_64(x); }
ABEL_FORCE_INLINE uint64_t ToHost64(uint64_t x) { return gbswap_64(x); }

ABEL_FORCE_INLINE constexpr bool IsLittleEndian() { return true; }

#elif defined ABEL_SYSTEM_BIG_ENDIAN

ABEL_FORCE_INLINE uint16_t FromHost16(uint16_t x) { return x; }
ABEL_FORCE_INLINE uint16_t ToHost16(uint16_t x) { return x; }

ABEL_FORCE_INLINE uint32_t FromHost32(uint32_t x) { return x; }
ABEL_FORCE_INLINE uint32_t ToHost32(uint32_t x) { return x; }

ABEL_FORCE_INLINE uint64_t FromHost64(uint64_t x) { return x; }
ABEL_FORCE_INLINE uint64_t ToHost64(uint64_t x) { return x; }

ABEL_FORCE_INLINE constexpr bool IsLittleEndian() { return false; }

#endif /* ENDIAN */

// Functions to do unaligned loads and stores in big-endian order.
ABEL_FORCE_INLINE uint16_t Load16(const void *p) {
  return ToHost16(ABEL_INTERNAL_UNALIGNED_LOAD16(p));
}

ABEL_FORCE_INLINE void Store16(void *p, uint16_t v) {
  ABEL_INTERNAL_UNALIGNED_STORE16(p, FromHost16(v));
}

ABEL_FORCE_INLINE uint32_t Load32(const void *p) {
  return ToHost32(ABEL_INTERNAL_UNALIGNED_LOAD32(p));
}

ABEL_FORCE_INLINE void Store32(void *p, uint32_t v) {
  ABEL_INTERNAL_UNALIGNED_STORE32(p, FromHost32(v));
}

ABEL_FORCE_INLINE uint64_t Load64(const void *p) {
  return ToHost64(ABEL_INTERNAL_UNALIGNED_LOAD64(p));
}

ABEL_FORCE_INLINE void Store64(void *p, uint64_t v) {
  ABEL_INTERNAL_UNALIGNED_STORE64(p, FromHost64(v));
}

}  // namespace big_endian


}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_ENDIAN_H_
