//
// Created by liyinbin on 2020/3/4.
//

#ifndef ABEL_BASE_AES_H_
#define ABEL_BASE_AES_H_

#if defined(__APPLE__)

#include <TargetConditionals.h>

#endif

// -----------------------------------------------------------------------------
// Architecture Checks
// -----------------------------------------------------------------------------

// These preprocessor directives are trying to determine CPU architecture,
// including necessary headers to support hardware AES.
//
// ABEL_ARCH_{X86/PPC/ARM} macros determine the platform.
#if defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64) || \
    defined(_M_X64)
#define ABEL_ARCH_X86_64
#elif defined(__i386) || defined(_M_IX86)
#define ABEL_ARCH_X86_32
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define ABEL_ARCH_AARCH64
#elif defined(__arm__) || defined(__ARMEL__) || defined(_M_ARM)
#define ABEL_ARCH_ARM
#elif defined(__powerpc64__) || defined(__PPC64__) || defined(__powerpc__) || \
    defined(__ppc__) || defined(__PPC__)
#define ABEL_ARCH_PPC
#else
// Unsupported architecture.
//  * https://sourceforge.net/p/predef/wiki/Architectures/
//  * https://msdn.microsoft.com/en-us/library/b0084kay.aspx
//  * for gcc, clang: "echo | gcc -E -dM -"
#endif


// ABEL_HAVE_ACCELERATED_AES indicates whether the currently active compiler
// flags (e.g. -maes) allow using hardware accelerated AES instructions, which
// implies us assuming that the target platform supports them.
#define ABEL_HAVE_ACCELERATED_AES 0

#if defined(ABEL_ARCH_X86_64)

#if defined(__AES__) || defined(__AVX__)
#undef ABEL_HAVE_ACCELERATED_AES
#define ABEL_HAVE_ACCELERATED_AES 1
#endif

#elif defined(ABEL_ARCH_PPC)

// Rely on VSX and CRYPTO extensions for vcipher on PowerPC.
#if (defined(__VEC__) || defined(__ALTIVEC__)) && defined(__VSX__) && \
    defined(__CRYPTO__)
#undef ABEL_HAVE_ACCELERATED_AES
#define ABEL_HAVE_ACCELERATED_AES 1
#endif

#elif defined(ABEL_ARCH_ARM) || defined(ABEL_ARCH_AARCH64)

// http://infocenter.arm.com/help/topic/com.arm.doc.ihi0053c/IHI0053C_acle_2_0.pdf
// Rely on NEON+CRYPTO extensions for ARM.
#if defined(__ARM_NEON) && defined(__ARM_FEATURE_CRYPTO)
#undef ABEL_HAVE_ACCELERATED_AES
#define ABEL_HAVE_ACCELERATED_AES 1
#endif

#endif

// NaCl does not allow AES.
#if defined(__native_client__)
#undef ABEL_HAVE_ACCELERATED_AES
#define ABEL_HAVE_ACCELERATED_AES 0
#endif

// ABEL_AES_DISPATCH indicates whether the currently active
// platform has, or should use run-time dispatch for selecting the
// acclerated randen implementation.
#define ABEL_AES_DISPATCH 0

#if defined(ABEL_ARCH_X86_64)
// Dispatch is available on x86_64
#undef ABEL_AES_DISPATCH
#define ABEL_AES_DISPATCH 1
#elif defined(__linux__) && defined(ABEL_ARCH_PPC)
// Or when running linux PPC
#undef ABEL_AES_DISPATCH
#define ABEL_AES_DISPATCH 1
#elif defined(__linux__) && defined(ABEL_ARCH_AARCH64)
// Or when running linux AArch64
#undef ABEL_AES_DISPATCH
#define ABEL_AES_DISPATCH 1
#elif defined(__linux__) && defined(ABEL_ARCH_ARM) && (__ARM_ARCH >= 8)
// Or when running linux ARM v8 or higher.
// (This captures a lot of Android configurations.)
#undef ABEL_AES_DISPATCH
#define ABEL_AES_DISPATCH 1
#endif

// NaCl does not allow dispatch.
#if defined(__native_client__)
#undef ABEL_AES_DISPATCH
#define ABEL_AES_DISPATCH 0
#endif

// iOS does not support dispatch, even on x86, since applications
// should be bundled as fat binaries, with a different build tailored for
// each specific supported platform/architecture.
#if (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || \
    (defined(TARGET_OS_IPHONE_SIMULATOR) && TARGET_OS_IPHONE_SIMULATOR)
#undef ABEL_AES_DISPATCH
#define ABEL_AES_DISPATCH 0
#endif


#endif //ABEL_BASE_AES_H_
