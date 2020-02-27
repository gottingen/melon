
#ifndef ABEL_SYSTEM_ENDIAN_H_
#define ABEL_SYSTEM_ENDIAN_H_

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
#include <abel/atomic/unaligned_access.h>
#include <abel/base/profile.h>
#include <abel/base/math.h>

namespace abel {

#ifdef ABEL_SYSTEM_LITTLE_ENDIAN

// Definitions for ntohl etc. that don't require us to include
// netinet/in.h. We wrap gbswap_32 and gbswap_16 in functions rather
// than just #defining them because in debug mode, gcc doesn't
// correctly handle the (rather involved) definitions of bswap_32.
// gcc guarantees that ABEL_FORCE_INLINE functions are as fast as macros, so
// this isn't a performance hit.
    ABEL_FORCE_INLINE uint16_t abel_htons(uint16_t x) { return abel::bit_swap16(x); }

    ABEL_FORCE_INLINE uint32_t abel_htonl(uint32_t x) { return abel::bit_swap32(x); }

    ABEL_FORCE_INLINE uint64_t abel_htonll(uint64_t x) { return abel::bit_swap64(x); }

#elif defined ABEL_SYSTEM_BIG_ENDIAN

    // These definitions are simpler on big-endian machines
    // These are functions instead of macros to avoid self-assignment warnings
    // on calls such as "i = ghtnol(i);".  This also provides type checking.
    ABEL_FORCE_INLINE uint16_t abel_htons(uint16_t x) { return x; }
    ABEL_FORCE_INLINE uint32_t abel_htonl(uint32_t x) { return x; }
    ABEL_FORCE_INLINE uint64_t abel_htonll(uint64_t x) { return x; }

#else
#error \
    "Unsupported byte order: Either ABEL_SYSTEM_BIG_ENDIAN or " \
       "ABEL_SYSTEM_LITTLE_ENDIAN must be defined"
#endif  // byte order

    ABEL_FORCE_INLINE uint16_t abel_ntohs(uint16_t x) { return abel_htons(x); }

    ABEL_FORCE_INLINE uint32_t abel_ntohl(uint32_t x) { return abel_htonl(x); }

    ABEL_FORCE_INLINE uint64_t abel_ntohll(uint64_t x) { return abel_htonll(x); }

// Utilities to convert numbers between the current hosts's native byte
// order and little-endian byte order
//
// Load/Store methods are alignment safe
    namespace little_endian {
// Conversion functions.
#ifdef ABEL_SYSTEM_LITTLE_ENDIAN

        ABEL_FORCE_INLINE uint16_t from_host16(uint16_t x) { return x; }

        ABEL_FORCE_INLINE uint16_t to_host16(uint16_t x) { return x; }

        ABEL_FORCE_INLINE uint32_t from_host32(uint32_t x) { return x; }

        ABEL_FORCE_INLINE uint32_t to_host32(uint32_t x) { return x; }

        ABEL_FORCE_INLINE uint64_t from_host64(uint64_t x) { return x; }

        ABEL_FORCE_INLINE uint64_t to_host64(uint64_t x) { return x; }

        ABEL_FORCE_INLINE constexpr bool isLittle_endian() { return true; }

#elif defined ABEL_SYSTEM_BIG_ENDIAN

        ABEL_FORCE_INLINE uint16_t from_host16(uint16_t x) { return abel::bit_swap16(x); }
        ABEL_FORCE_INLINE uint16_t to_host16(uint16_t x) { return abel::bit_swap16(x); }

        ABEL_FORCE_INLINE uint32_t from_host32(uint32_t x) { return abel::bit_swap32(x); }
        ABEL_FORCE_INLINE uint32_t to_host32(uint32_t x) { return abel::bit_swap32(x); }

        ABEL_FORCE_INLINE uint64_t from_host64(uint64_t x) { return abel::bit_swap64(x); }
        ABEL_FORCE_INLINE uint64_t to_host64(uint64_t x) { return abel::bit_swap64(x); }

        ABEL_FORCE_INLINE constexpr bool is_little_endian() { return false; }

#endif /* ENDIAN */

// Functions to do unaligned loads and stores in little-endian order.
        ABEL_FORCE_INLINE uint16_t load16(const void *p) {
            return to_host16(ABEL_INTERNAL_UNALIGNED_LOAD16(p));
        }

        ABEL_FORCE_INLINE void store16(void *p, uint16_t v) {
            ABEL_INTERNAL_UNALIGNED_STORE16(p, from_host16(v));
        }

        ABEL_FORCE_INLINE uint32_t load32(const void *p) {
            return to_host32(ABEL_INTERNAL_UNALIGNED_LOAD32(p));
        }

        ABEL_FORCE_INLINE void store32(void *p, uint32_t v) {
            ABEL_INTERNAL_UNALIGNED_STORE32(p, from_host32(v));
        }

        ABEL_FORCE_INLINE uint64_t load64(const void *p) {
            return to_host64(ABEL_INTERNAL_UNALIGNED_LOAD64(p));
        }

        ABEL_FORCE_INLINE void store64(void *p, uint64_t v) {
            ABEL_INTERNAL_UNALIGNED_STORE64(p, from_host64(v));
        }

    }  // namespace little_endian

// Utilities to convert numbers between the current hosts's native byte
// order and big-endian byte order (same as network byte order)
//
// Load/Store methods are alignment safe
    namespace big_endian {
#ifdef ABEL_SYSTEM_LITTLE_ENDIAN

        ABEL_FORCE_INLINE uint16_t from_host16(uint16_t x) { return abel::bit_swap16(x); }

        ABEL_FORCE_INLINE uint16_t to_host16(uint16_t x) { return abel::bit_swap16(x); }

        ABEL_FORCE_INLINE uint32_t from_host32(uint32_t x) { return abel::bit_swap32(x); }

        ABEL_FORCE_INLINE uint32_t to_host32(uint32_t x) { return abel::bit_swap32(x); }

        ABEL_FORCE_INLINE uint64_t from_host64(uint64_t x) { return abel::bit_swap64(x); }

        ABEL_FORCE_INLINE uint64_t to_host64(uint64_t x) { return abel::bit_swap64(x); }

        ABEL_FORCE_INLINE constexpr bool is_little_endian() { return true; }

#elif defined ABEL_SYSTEM_BIG_ENDIAN

        ABEL_FORCE_INLINE uint16_t from_host16(uint16_t x) { return x; }
        ABEL_FORCE_INLINE uint16_t to_host16(uint16_t x) { return x; }

        ABEL_FORCE_INLINE uint32_t from_host32(uint32_t x) { return x; }
        ABEL_FORCE_INLINE uint32_t to_host32(uint32_t x) { return x; }

        ABEL_FORCE_INLINE uint64_t from_host64(uint64_t x) { return x; }
        ABEL_FORCE_INLINE uint64_t to_host1664(uint64_t x) { return x; }

        ABEL_FORCE_INLINE constexpr bool is_little_endian() { return false; }

#endif /* ENDIAN */

// Functions to do unaligned loads and stores in big-endian order.
        ABEL_FORCE_INLINE uint16_t load16(const void *p) {
            return to_host16(ABEL_INTERNAL_UNALIGNED_LOAD16(p));
        }

        ABEL_FORCE_INLINE void store16(void *p, uint16_t v) {
            ABEL_INTERNAL_UNALIGNED_STORE16(p, from_host16(v));
        }

        ABEL_FORCE_INLINE uint32_t load32(const void *p) {
            return to_host32(ABEL_INTERNAL_UNALIGNED_LOAD32(p));
        }

        ABEL_FORCE_INLINE void store32(void *p, uint32_t v) {
            ABEL_INTERNAL_UNALIGNED_STORE32(p, from_host32(v));
        }

        ABEL_FORCE_INLINE uint64_t load64(const void *p) {
            return to_host64(ABEL_INTERNAL_UNALIGNED_LOAD64(p));
        }

        ABEL_FORCE_INLINE void store64(void *p, uint64_t v) {
            ABEL_INTERNAL_UNALIGNED_STORE64(p, from_host64(v));
        }

    }  // namespace big_endian

}  // namespace abel

#endif  // ABEL_SYSTEM_ENDIAN_H_
