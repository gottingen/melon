//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.


#ifndef MUTIL_THIRD_PARTY_MURMURHASH3_MURMURHASH3_H
#define MUTIL_THIRD_PARTY_MURMURHASH3_MURMURHASH3_H

// ======= Platform-specific functions and macros =======
// Microsoft Visual Studio
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else   // defined(_MSC_VER)
#include <stdint.h>
#endif // !defined(_MSC_VER)

#if defined(_MSC_VER)
#define MURMURHASH_FORCE_INLINE    __forceinline
#define BIG_CONSTANT(x) (x)
#else
#define MURMURHASH_FORCE_INLINE inline __attribute__((always_inline))
#define BIG_CONSTANT(x) (x##LLU)
#endif

namespace mutil {

// Finalization mix - force all bits of a hash block to avalanche
MURMURHASH_FORCE_INLINE uint32_t fmix32 (uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}
MURMURHASH_FORCE_INLINE uint64_t fmix64 (uint64_t k) {
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;
  return k;
}

// ========= Original version ===========
void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out);
void MurmurHash3_x86_128(const void* key, int len, uint32_t seed, void* out);
void MurmurHash3_x64_128(const void* key, int len, uint32_t seed, void* out);

// ========= Iterative version ==========
// for computing hashcode for very large inputs, say file contents. The API are
// similar with iterative MD5 API.
// Notice: |ctx| must be non-NULL and valid, otherwise the behavior is undefined.
struct MurmurHash3_x86_32_Context {
    uint32_t h1;
    int total_len;
    int tail_len;
    uint8_t tail[4];
};
void MurmurHash3_x86_32_Init(MurmurHash3_x86_32_Context* ctx, uint32_t seed);
void MurmurHash3_x86_32_Update(MurmurHash3_x86_32_Context* ctx, const void* key, int len);
void MurmurHash3_x86_32_Final(void* out, const MurmurHash3_x86_32_Context* ctx);

struct MurmurHash3_x86_128_Context {
    uint32_t h1;
    uint32_t h2;
    uint32_t h3;
    uint32_t h4;
    // Notice: may overflow, but fine.
    int total_len;
    int tail_len;
    uint8_t tail[16];   
};
void MurmurHash3_x86_128_Init(MurmurHash3_x86_128_Context* ctx, uint32_t seed);
void MurmurHash3_x86_128_Update(MurmurHash3_x86_128_Context* ctx, const void* key, int len);
void MurmurHash3_x86_128_Final(void* out, const MurmurHash3_x86_128_Context* ctx);

struct MurmurHash3_x64_128_Context {
    uint64_t h1;
    uint64_t h2;
    // Notice:
    //   different from MurmurHash3_x86_128_Context where total_len is int.
    //   When total_len >= 2^31, this is also (slightly) different from
    //   MurmurHash3_x64_128() because len in the function is int. 
    uint64_t total_len;
    int tail_len;
    uint8_t tail[16];   
};
void MurmurHash3_x64_128_Init(MurmurHash3_x64_128_Context* ctx, uint32_t seed);
void MurmurHash3_x64_128_Update(MurmurHash3_x64_128_Context* ctx, const void* key, int len);
void MurmurHash3_x64_128_Final(void* out, const MurmurHash3_x64_128_Context* ctx);

} // namespace mutil

#endif // MUTIL_THIRD_PARTY_MURMURHASH3_MURMURHASH3_H
