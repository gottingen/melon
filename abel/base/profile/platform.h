//
// Created by liyinbin on 2019/12/11.
//

#ifndef ABEL_BASE_PROFILE_PLATFORM_H_
#define ABEL_BASE_PROFILE_PLATFORM_H_


// Cygwin
// This is a pseudo-platform which will be defined along with ABEL_PLATFORM_LINUX when
// using the Cygwin build environment.
#if defined(__CYGWIN__)
    #define ABEL_PLATFORM_CYGWIN 1
    #define ABEL_PLATFORM_DESKTOP 1
#endif

// MinGW
// This is a pseudo-platform which will be defined along with ABEL_PLATFORM_WINDOWS when
// using the MinGW Windows build environment.
#if defined(__MINGW32__) || defined(__MINGW64__)
    #define ABEL_PLATFORM_MINGW 1
    #define ABEL_PLATFORM_DESKTOP 1
#endif

#if defined(ABEL_PLATFORM_PS4) || defined(__ORBIS__) || defined(ABEL_PLATFORM_KETTLE)
// PlayStation 4
    // Orbis was Sony's code-name for the platform, which is now obsolete.
    // Kettle was an CB-specific code-name for the platform, which is now obsolete.
    #if defined(ABEL_PLATFORM_PS4)
        #undef  ABEL_PLATFORM_PS4
    #endif
    #define ABEL_PLATFORM_PS4 1

    // Backward compatibility:
    #if defined(ABEL_PLATFORM_KETTLE)
        #undef  ABEL_PLATFORM_KETTLE
    #endif
    // End backward compatbility

    #define ABEL_PLATFORM_KETTLE 1
    #define ABEL_PLATFORM_NAME "PS4"
    #define ABEL_SYSTEM_LITTLE_ENDIAN 1
    #define ABEL_PLATFORM_DESCRIPTION "PS4 on x64"
    #define ABEL_PLATFORM_CONSOLE 1
    #define ABEL_PLATFORM_SONY 1
    #define ABEL_PLATFORM_POSIX 1
    // #define ABEL_POSIX_THREADS_AVAILABLE 1  // POSIX threading API is available but discouraged.  Sony indicated use of the scePthreads* API is preferred.
    #define ABEL_PROCESSOR_X86_64 1
    #if defined(__GNUC__) || defined(__clang__)
        #define ABEL_ASM_STYLE_ATT 1
    #endif

#elif defined(ABEL_PLATFORM_XBOXONE) || defined(_DURANGO) || defined(_XBOX_ONE) || defined(ABEL_PLATFORM_CAPILANO) || defined(_GAMING_XBOX)
// XBox One
// Durango was Microsoft's code-name for the platform, which is now obsolete.
// Microsoft uses _DURANGO instead of some variation of _XBOX, though it's not natively defined by the compiler.
// Capilano was an EA-specific code-name for the platform, which is now obsolete.
    #if defined(ABEL_PLATFORM_XBOXONE)
        #undef  ABEL_PLATFORM_XBOXONE
    #endif
    #define ABEL_PLATFORM_XBOXONE 1

// Backward compatibility:
    #if defined(ABEL_PLATFORM_CAPILANO)
        #undef  ABEL_PLATFORM_CAPILANO
    #endif
    #define ABEL_PLATFORM_CAPILANO 1
    #if defined(ABEL_PLATFORM_CAPILANO_XDK) && !defined(ABEL_PLATFORM_XBOXONE_XDK)
        #define ABEL_PLATFORM_XBOXONE_XDK 1
    #endif
    #if defined(ABEL_PLATFORM_CAPILANO_ADK) && !defined(ABEL_PLATFORM_XBOXONE_ADK)
        #define ABEL_PLATFORM_XBOXONE_ADK 1
    #endif
// End backward compatibility

    #if !defined(_DURANGO)
        #define _DURANGO
    #endif
    #define ABEL_PLATFORM_NAME "XBox One"
//#define ABEL_PROCESSOR_X86  Currently our policy is that we don't define this, even though x64 is something of a superset of x86.
    #define ABEL_PROCESSOR_X86_64 1
    #define ABEL_SYSTEM_LITTLE_ENDIAN 1
    #define ABEL_PLATFORM_DESCRIPTION "XBox One on x64"
    #define ABEL_ASM_STYLE_INTEL 1
    #define ABEL_PLATFORM_CONSOLE 1
    #define ABEL_PLATFORM_MICROSOFT 1

// WINAPI_FAMILY defines - mirrored from winapifamily.h
    #define ABEL_WINAPI_FAMILY_APP         1000
    #define ABEL_WINAPI_FAMILY_DESKTOP_APP 1001
    #define ABEL_WINAPI_FAMILY_PHONE_APP   1002
    #define ABEL_WINAPI_FAMILY_TV_APP      1003
    #define ABEL_WINAPI_FAMILY_TV_TITLE    1004
    #define ABEL_WINAPI_FAMILY_GAMES       1006

    #if defined(WINAPI_FAMILY)
        #include <winapifamily.h>
        #if defined(WINAPI_FAMILY_TV_TITLE) && WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_TV_TITLE
        #elif defined(WINAPI_FAMILY_DESKTOP_APP) && WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_DESKTOP_APP
        #elif defined(WINAPI_FAMILY_GAMES) && WINAPI_FAMILY == WINAPI_FAMILY_GAMES
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_GAMES
        #else
            #error Unsupported WINAPI_FAMILY
        #endif
    #else
        #error WINAPI_FAMILY should always be defined on Capilano.
    #endif

// Macro to determine if a partition is enabled.
    #define ABEL_WINAPI_FAMILY_PARTITION(Partition)	(Partition)

    #if ABEL_WINAPI_FAMILY == ABEL_WINAPI_FAMILY_DESKTOP_APP
        #define ABEL_WINAPI_PARTITION_CORE     1
        #define ABEL_WINAPI_PARTITION_DESKTOP  1
        #define ABEL_WINAPI_PARTITION_APP      1
        #define ABEL_WINAPI_PARTITION_PC_APP   0
        #define ABEL_WIANPI_PARTITION_PHONE    0
        #define ABEL_WINAPI_PARTITION_TV_APP   0
        #define ABEL_WINAPI_PARTITION_TV_TITLE 0
        #define ABEL_WINAPI_PARTITION_GAMES    0
    #elif ABEL_WINAPI_FAMILY == ABEL_WINAPI_FAMILY_TV_TITLE
        #define ABEL_WINAPI_PARTITION_CORE     1
        #define ABEL_WINAPI_PARTITION_DESKTOP  0
        #define ABEL_WINAPI_PARTITION_APP      0
        #define ABEL_WINAPI_PARTITION_PC_APP   0
        #define ABEL_WIANPI_PARTITION_PHONE    0
        #define ABEL_WINAPI_PARTITION_TV_APP   0
        #define ABEL_WINAPI_PARTITION_TV_TITLE 1
        #define ABEL_WINAPI_PARTITION_GAMES    0
    #elif ABEL_WINAPI_FAMILY == ABEL_WINAPI_FAMILY_GAMES
        #define ABEL_WINAPI_PARTITION_CORE     1
        #define ABEL_WINAPI_PARTITION_DESKTOP  0
        #define ABEL_WINAPI_PARTITION_APP      0
        #define ABEL_WINAPI_PARTITION_PC_APP   0
        #define ABEL_WIANPI_PARTITION_PHONE    0
        #define ABEL_WINAPI_PARTITION_TV_APP   0
        #define ABEL_WINAPI_PARTITION_TV_TITLE 0
        #define ABEL_WINAPI_PARTITION_GAMES    1
    #else
        #error Unsupported WINAPI_FAMILY
    #endif

    #if ABEL_WINAPI_FAMILY_PARTITION(ABEL_WINAPI_PARTITION_GAMES)
        #define CS_UNDEFINED_STRING 			1
        #define CS_UNDEFINED_STRING 		1
    #endif

    #if ABEL_WINAPI_FAMILY_PARTITION(ABEL_WINAPI_PARTITION_TV_TITLE)
        #define ABEL_PLATFORM_XBOXONE_XDK 	1
    #endif

// Android (Google phone OS)
#elif defined(ABEL_PLATFORM_ANDROID) || defined(__ANDROID__)
    #undef  ABEL_PLATFORM_ANDROID
    #define ABEL_PLATFORM_ANDROID 1
    #define ABEL_PLATFORM_LINUX 1
    #define ABEL_PLATFORM_UNIX 1
    #define ABEL_PLATFORM_POSIX 1
    #define ABEL_PLATFORM_NAME "Android"
    #define ABEL_ASM_STYLE_ATT 1
    #if defined(__arm__)
        #define ABEL_ABI_ARM_LINUX 1  // a.k.a. "ARM eabi"
        #define ABEL_PROCESSOR_ARM32 1
        #define ABEL_PLATFORM_DESCRIPTION "Android on ARM"
    #elif defined(__aarch64__)
        #define ABEL_PROCESSOR_ARM64 1
        #define ABEL_PLATFORM_DESCRIPTION "Android on ARM64"
    #elif defined(__i386__)
        #define ABEL_PROCESSOR_X86 1
        #define ABEL_PLATFORM_DESCRIPTION "Android on x86"
    #elif defined(__x86_64)
        #define ABEL_PROCESSOR_X86_64 1
        #define ABEL_PLATFORM_DESCRIPTION "Android on x64"
    #else
        #error Unknown processor
    #endif
    #if !defined(ABEL_SYSTEM_BIG_ENDIAN) && !defined(ABEL_SYSTEM_LITTLE_ENDIAN)
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
    #endif
    #define ABEL_PLATFORM_MOBILE 1
#elif defined(__APPLE__) && __APPLE__
    #include <TargetConditionals.h>

// Apple family of operating systems.
    #define ABEL_PLATFORM_APPLE
    #define ABEL_PLATFORM_POSIX 1

// iPhone
// TARGET_OS_IPHONE will be undefined on an unknown compiler, and will be defined on gcc.
    #if defined(ABEL_PLATFORM_IPHONE) || defined(__IPHONE__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR)
        #undef  ABEL_PLATFORM_IPHONE
        #define ABEL_PLATFORM_IPHONE 1
        #define ABEL_PLATFORM_NAME "iPhone"
        #define ABEL_ASM_STYLE_ATT 1
        #define ABEL_POSIX_THREADS_AVAILABLE 1
        #if defined(__arm__)
            #define ABEL_ABI_ARM_APPLE 1
            #define ABEL_PROCESSOR_ARM32 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "iPhone on ARM"
        #elif defined(__aarch64__) || defined(__AARCH64)
            #define ABEL_ABI_ARM64_APPLE 1
            #define ABEL_PROCESSOR_ARM64 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "iPhone on ARM64"
        #elif defined(__i386__)
            #define ABEL_PLATFORM_IPHONE_SIMULATOR 1
            #define ABEL_PROCESSOR_X86 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "iPhone simulator on x86"
        #elif defined(__x86_64) || defined(__amd64)
            #define ABEL_PROCESSOR_X86_64 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "iPhone simulator on x64"
        #else
            #error Unknown processor
        #endif
        #define ABEL_PLATFORM_MOBILE 1

// Macintosh OSX
// TARGET_OS_MAC is defined by the Metrowerks and older AppleC compilers.
// Howerver, TARGET_OS_MAC is defined to be 1 in all cases.
// __i386__ and __intel__ are defined by the GCC compiler.
// __dest_os is defined by the Metrowerks compiler.
// __MACH__ is defined by the Metrowerks and GCC compilers.
// powerc and __powerc are defined by the Metrowerks and GCC compilers.
    #elif defined(ABEL_PLATFORM_OSX) || defined(__MACH__) || (defined(__MSL__) && (__dest_os == __mac_os_x))
        #undef  ABEL_PLATFORM_OSX
        #define ABEL_PLATFORM_OSX 1
        #define ABEL_PLATFORM_UNIX 1
        #define ABEL_PLATFORM_POSIX 1
  //#define ABEL_PLATFORM_BSD 1           We don't currently define this. OSX has some BSD history but a lot of the API is different.
        #define ABEL_PLATFORM_NAME "OSX"
        #if defined(__i386__) || defined(__intel__)
            #define ABEL_PROCESSOR_X86 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "OSX on x86"
        #elif defined(__x86_64) || defined(__amd64)
            #define ABEL_PROCESSOR_X86_64 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "OSX on x64"
        #elif defined(__arm__)
            #define ABEL_ABI_ARM_APPLE 1
            #define ABEL_PROCESSOR_ARM32 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "OSX on ARM"
        #elif defined(__aarch64__) || defined(__AARCH64)
            #define ABEL_ABI_ARM64_APPLE 1
            #define ABEL_PROCESSOR_ARM64 1
            #define ABEL_SYSTEM_LITTLE_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "OSX on ARM64"
        #elif defined(__POWERPC64__) || defined(__powerpc64__)
            #define ABEL_PROCESSOR_POWERPC 1
            #define ABEL_PROCESSOR_POWERPC_64 1
            #define ABEL_SYSTEM_BIG_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "OSX on PowerPC 64"
        #elif defined(__POWERPC__) || defined(__powerpc__)
            #define ABEL_PROCESSOR_POWERPC 1
            #define ABEL_PROCESSOR_POWERPC_32 1
            #define ABEL_SYSTEM_BIG_ENDIAN 1
            #define ABEL_PLATFORM_DESCRIPTION "OSX on PowerPC"
        #else
            #error Unknown processor
        #endif
        #if defined(__GNUC__)
            #define ABEL_ASM_STYLE_ATT 1
        #else
            #define ABEL_ASM_STYLE_MOTOROLA 1
        #endif
        #define ABEL_PLATFORM_DESKTOP 1
    #else
        #error Unknown Apple Platform
    #endif

// Linux
// __linux and __linux__ are defined by the GCC and Borland compiler.
// __i386__ and __intel__ are defined by the GCC compiler.
// __i386__ is defined by the Metrowerks compiler.
// _M_IX86 is defined by the Borland compiler.
// __sparc__ is defined by the GCC compiler.
// __powerpc__ is defined by the GCC compiler.
// __ARM_EABI__ is defined by GCC on an ARM v6l (Raspberry Pi 1)
// __ARM_ARCH_7A__ is defined by GCC on an ARM v7l (Raspberry Pi 2)
#elif defined(ABEL_PLATFORM_LINUX) || (defined(__linux) || defined(__linux__))
    #undef  ABEL_PLATFORM_LINUX
    #define ABEL_PLATFORM_LINUX 1
    #define ABEL_PLATFORM_UNIX 1
    #define ABEL_PLATFORM_POSIX 1
    #define ABEL_PLATFORM_NAME "Linux"
    #if defined(__i386__) || defined(__intel__) || defined(_M_IX86)
        #define ABEL_PROCESSOR_X86 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Linux on x86"
    #elif defined(__ARM_ARCH_7A__) || defined(__ARM_EABI__)
        #define ABEL_ABI_ARM_LINUX 1
        #define ABEL_PROCESSOR_ARM32 1
        #define ABEL_PLATFORM_DESCRIPTION "Linux on ARM 6/7 32-bits"
    #elif defined(__aarch64__) || defined(__AARCH64)
        #define ABEL_PROCESSOR_ARM64 1
        #define ABEL_PLATFORM_DESCRIPTION "Linux on ARM64"
    #elif defined(__x86_64__)
        #define ABEL_PROCESSOR_X86_64 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Linux on x64"
    #elif defined(__powerpc64__)
        #define ABEL_PROCESSOR_POWERPC 1
        #define ABEL_PROCESSOR_POWERPC_64 1
        #define ABEL_SYSTEM_BIG_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Linux on PowerPC 64"
    #elif defined(__powerpc__)
        #define ABEL_PROCESSOR_POWERPC 1
        #define ABEL_PROCESSOR_POWERPC_32 1
        #define ABEL_SYSTEM_BIG_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Linux on PowerPC"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if defined(__GNUC__)
        #define ABEL_ASM_STYLE_ATT 1
    #endif
    #define ABEL_PLATFORM_DESKTOP 1


#elif defined(ABEL_PLATFORM_BSD) || (defined(__BSD__) || defined(__FreeBSD__))
    #undef  ABEL_PLATFORM_BSD
    #define ABEL_PLATFORM_BSD 1
    #define ABEL_PLATFORM_UNIX 1
    #define ABEL_PLATFORM_POSIX 1     // BSD's posix complaince is not identical to Linux's
    #define ABEL_PLATFORM_NAME "BSD Unix"
    #if defined(__i386__) || defined(__intel__)
        #define ABEL_PROCESSOR_X86 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "BSD on x86"
    #elif defined(__x86_64__)
        #define ABEL_PROCESSOR_X86_64 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "BSD on x64"
    #elif defined(__powerpc64__)
        #define ABEL_PROCESSOR_POWERPC 1
        #define ABEL_PROCESSOR_POWERPC_64 1
        #define ABEL_SYSTEM_BIG_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "BSD on PowerPC 64"
    #elif defined(__powerpc__)
        #define ABEL_PROCESSOR_POWERPC 1
        #define ABEL_PROCESSOR_POWERPC_32 1
        #define ABEL_SYSTEM_BIG_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "BSD on PowerPC"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if !defined(ABEL_PLATFORM_FREEBSD) && defined(__FreeBSD__)
        #define ABEL_PLATFORM_FREEBSD 1 // This is a variation of BSD.
    #endif
    #if defined(__GNUC__)
        #define ABEL_ASM_STYLE_ATT 1
    #endif
    #define ABEL_PLATFORM_DESKTOP 1


#elif defined(ABEL_PLATFORM_WINDOWS_PHONE)
    #undef ABEL_PLATFORM_WINDOWS_PHONE
    #define ABEL_PLATFORM_WINDOWS_PHONE 1
    #define ABEL_PLATFORM_NAME "Windows Phone"
    #if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
        #define ABEL_PROCESSOR_X86_64 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows Phone on x64"
    #elif defined(_M_IX86) || defined(_X86_)
        #define ABEL_PROCESSOR_X86 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows Phone on X86"
    #elif defined(_M_ARM)
        #define ABEL_ABI_ARM_WINCE 1
        #define ABEL_PROCESSOR_ARM32 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows Phone on ARM"
    #else //Possibly other Windows Phone variants
        #error Unknown processor
        #error Unknown endianness
    #endif
    #define ABEL_PLATFORM_MICROSOFT 1

// WINAPI_FAMILY defines - mirrored from winapifamily.h
    #define ABEL_WINAPI_FAMILY_APP         1
    #define ABEL_WINAPI_FAMILY_DESKTOP_APP 2
    #define ABEL_WINAPI_FAMILY_PHONE_APP   3

    #if defined(WINAPI_FAMILY)
        #include <winapifamily.h>
        #if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_PHONE_APP
        #else
            #error Unsupported WINAPI_FAMILY for Windows Phone
        #endif
    #else
        #error WINAPI_FAMILY should always be defined on Windows Phone.
    #endif

// Macro to determine if a partition is enabled.
    #define ABEL_WINAPI_FAMILY_PARTITION(Partition)   (Partition)

// Enable the appropriate partitions for the current family
    #if ABEL_WINAPI_FAMILY == ABEL_WINAPI_FAMILY_PHONE_APP
        #   define ABEL_WINAPI_PARTITION_CORE    1
        #   define ABEL_WINAPI_PARTITION_PHONE   1
        #   define ABEL_WINAPI_PARTITION_APP     1
    #else
        #   error Unsupported WINAPI_FAMILY for Windows Phone
    #endif


// Windows
// _WIN32 is defined by the VC++, Intel and GCC compilers.
// _WIN64 is defined by the VC++, Intel and GCC compilers.
// __WIN32__ is defined by the Borland compiler.
// __INTEL__ is defined by the Metrowerks compiler.
// _M_IX86, _M_AMD64 and _M_IA64 are defined by the VC++, Intel, and Borland compilers.
// _X86_, _AMD64_, and _IA64_ are defined by the Metrowerks compiler.
// _M_ARM is defined by the VC++ compiler.
#elif (defined(ABEL_PLATFORM_WINDOWS) || (defined(_WIN32) || defined(__WIN32__) || defined(_WIN64))) && !defined(CS_UNDEFINED_STRING)
    #undef  ABEL_PLATFORM_WINDOWS
    #define ABEL_PLATFORM_WINDOWS 1
    #define ABEL_PLATFORM_NAME "Windows"
    #ifdef _WIN64 // VC++ defines both _WIN32 and _WIN64 when compiling for Win64.
        #define ABEL_PLATFORM_WIN64 1
    #else
        #define ABEL_PLATFORM_WIN32 1
    #endif
    #if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
        #define ABEL_PROCESSOR_X86_64 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows on x64"
    #elif defined(_M_IX86) || defined(_X86_)
        #define ABEL_PROCESSOR_X86 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows on X86"
    #elif defined(_M_IA64) || defined(_IA64_)
        #define ABEL_PROCESSOR_IA64 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows on IA-64"
    #elif defined(_M_ARM)
        #define ABEL_ABI_ARM_WINCE 1
        #define ABEL_PROCESSOR_ARM32 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows on ARM"
    #elif defined(_M_ARM64)
        #define ABEL_PROCESSOR_ARM64 1
        #define ABEL_SYSTEM_LITTLE_ENDIAN 1
        #define ABEL_PLATFORM_DESCRIPTION "Windows on ARM64"
    #else //Possibly other Windows CE variants
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if defined(__GNUC__)
        #define ABEL_ASM_STYLE_ATT 1
    #elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(__ICL)
        #define ABEL_ASM_STYLE_INTEL 1
    #endif
    #define ABEL_PLATFORM_DESKTOP 1
    #define ABEL_PLATFORM_MICROSOFT 1

// WINAPI_FAMILY defines to support Windows 8 Metro Apps - mirroring winapifamily.h in the Windows 8 SDK
    #define ABEL_WINAPI_FAMILY_APP         1000
    #define ABEL_WINAPI_FAMILY_DESKTOP_APP 1001
    #define ABEL_WINAPI_FAMILY_GAMES       1006

    #if defined(WINAPI_FAMILY)
        #if defined(_MSC_VER)
            #pragma warning(push, 0)
        #endif
        #include <winapifamily.h>
        #if defined(_MSC_VER)
            #pragma warning(pop)
        #endif
        #if defined(WINAPI_FAMILY_DESKTOP_APP) && WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_DESKTOP_APP
        #elif defined(WINAPI_FAMILY_APP) && WINAPI_FAMILY == WINAPI_FAMILY_APP
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_APP
        #elif defined(WINAPI_FAMILY_GAMES) && WINAPI_FAMILY == WINAPI_FAMILY_GAMES
            #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_GAMES
        #else
            #error Unsupported WINAPI_FAMILY
        #endif
    #else
        #define ABEL_WINAPI_FAMILY ABEL_WINAPI_FAMILY_DESKTOP_APP
    #endif

    #define ABEL_WINAPI_PARTITION_DESKTOP   1
    #define ABEL_WINAPI_PARTITION_APP       1
    #define ABEL_WINAPI_PARTITION_GAMES    (ABEL_WINAPI_FAMILY == ABEL_WINAPI_FAMILY_GAMES)

    #define ABEL_WINAPI_FAMILY_PARTITION(Partition)   (Partition)

// ABEL_PLATFORM_WINRT
// This is a subset of Windows which is used for tablets and the "Metro" (restricted) Windows user interface.
// WinRT doesn't doesn't have access to the Windows "desktop" API, but WinRT can nevertheless run on
// desktop computers in addition to tablets. The Windows Phone API is a subset of WinRT and is not included
// in it due to it being only a part of the API.
    #if defined(__cplusplus_winrt)
        #define ABEL_PLATFORM_WINRT 1
    #endif

// Sun (Solaris)
// __SUNPRO_CC is defined by the Sun compiler.
// __sun is defined by the GCC compiler.
// __i386 is defined by the Sun and GCC compilers.
// __sparc is defined by the Sun and GCC compilers.
#else
    #error Unknown platform
    #error Unknown processor
    #error Unknown endianness
#endif

#ifndef ABEL_PROCESSOR_ARM
    #if defined(ABEL_PROCESSOR_ARM32) || defined(ABEL_PROCESSOR_ARM64) || defined(ABEL_PROCESSOR_ARM7)
        #define ABEL_PROCESSOR_ARM
    #endif
#endif

// ABEL_PLATFORM_PTR_SIZE
// Platform pointer size; same as sizeof(void*).
// This is not the same as sizeof(int), as int is usually 32 bits on
// even 64 bit platforms.
//
// _WIN64 is defined by Win64 compilers, such as VC++.
// _M_IA64 is defined by VC++ and Intel compilers for IA64 processors.
// __LP64__ is defined by HP compilers for the LP64 standard.
// _LP64 is defined by the GCC and Sun compilers for the LP64 standard.
// __ia64__ is defined by the GCC compiler for IA64 processors.
// __arch64__ is defined by the Sparc compiler for 64 bit processors.
// __mips64__ is defined by the GCC compiler for MIPS processors.
// __powerpc64__ is defined by the GCC compiler for PowerPC processors.
// __64BIT__ is defined by the AIX compiler for 64 bit processors.
// __sizeof_ptr is defined by the ARM compiler (armcc, armcpp).
//
#ifndef ABEL_PLATFORM_PTR_SIZE
    #if defined(__WORDSIZE) // Defined by some variations of GCC.
        #define ABEL_PLATFORM_PTR_SIZE ((__WORDSIZE) / 8)
    #elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__aarch64__) || defined(__mips64__) || defined(__64BIT__) || defined(__Ptr_Is_64)
        #define ABEL_PLATFORM_PTR_SIZE 8
    #elif defined(__CC_ARM) && (__sizeof_ptr == 8)
        #define ABEL_PLATFORM_PTR_SIZE 8
    #else
        #define ABEL_PLATFORM_PTR_SIZE 4
    #endif
#endif



// ABEL_PLATFORM_WORD_SIZE
// This defines the size of a machine word. This will be the same as
// the size of registers on the machine but not necessarily the same
// as the size of pointers on the machine. A number of 64 bit platforms
// have 64 bit registers but 32 bit pointers.
//
#ifndef ABEL_PLATFORM_WORD_SIZE
    #define ABEL_PLATFORM_WORD_SIZE ABEL_PLATFORM_PTR_SIZE
#endif

// ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT
// This defines the minimal alignment that the platform's malloc
// implementation will return. This should be used when writing custom
// allocators to ensure that the alignment matches that of malloc
#ifndef ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT
    #if defined(ABEL_PLATFORM_APPLE)
        #define ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT 16
    #elif defined(ABEL_PLATFORM_ANDROID) && defined(ABEL_PROCESSOR_ARM)
        #define ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT 8
    #elif defined(ABEL_PLATFORM_ANDROID) && defined(ABEL_PROCESSOR_X86_64)
        #define ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT 8
    #else
        #define ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT (ABEL_PLATFORM_PTR_SIZE * 2)
    #endif
#endif


// ABEL_MISALIGNED_SUPPORT_LEVEL
// Specifies if the processor can read and write built-in types that aren't
// naturally aligned.
//    0 - not supported. Likely causes an exception.
//    1 - supported but slow.
//    2 - supported and fast.
//
#ifndef ABEL_MISALIGNED_SUPPORT_LEVEL
    #if defined(ABEL_PROCESSOR_X86_64)
        #define ABEL_MISALIGNED_SUPPORT_LEVEL 2
    #else
        #define ABEL_MISALIGNED_SUPPORT_LEVEL 0
    #endif
#endif

// Macro to determine if a Windows API partition is enabled. Always false on non Microsoft platforms.
#if !defined(ABEL_WINAPI_FAMILY_PARTITION)
    #define ABEL_WINAPI_FAMILY_PARTITION(Partition) (0)
#endif


// ABEL_CACHE_LINE_SIZE
// Specifies the cache line size broken down by compile target.
// This the expected best guess values for the targets that we can make at compilation time.

#ifndef ABEL_CACHE_LINE_SIZE
    #if   defined(ABEL_PROCESSOR_X86)
        #define ABEL_CACHE_LINE_SIZE 32    // This is the minimum possible value.
    #elif defined(ABEL_PROCESSOR_X86_64)
        #define ABEL_CACHE_LINE_SIZE 64    // This is the minimum possible value
    #elif defined(ABEL_PROCESSOR_ARM32)
        #define ABEL_CACHE_LINE_SIZE 32    // This varies between implementations and is usually 32 or 64.
    #elif defined(ABEL_PROCESSOR_ARM64)
        #define ABEL_CACHE_LINE_SIZE 64    // Cache line Cortex-A8  (64 bytes) http://shervinemami.info/armAssembly.html however this remains to be mostly an assumption at this stage
    #elif (ABEL_PLATFORM_WORD_SIZE == 4)
        #define ABEL_CACHE_LINE_SIZE 32    // This is the minimum possible value
    #else
        #define ABEL_CACHE_LINE_SIZE 64    // This is the minimum possible value
    #endif
#endif

#endif //ABEL_BASE_PROFILE_PLATFORM_H_
