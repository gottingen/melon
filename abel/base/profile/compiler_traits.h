//
// Created by liyinbin on 2019/12/11.
//

#ifndef ABEL_BASE_PROFILE_COMPILER_TRAITS_H_
#define ABEL_BASE_PROFILE_COMPILER_TRAITS_H_

#include <abel/base/profile/platform.h>
#include <abel/base/profile/compiler.h>


// Metrowerks uses #defines in its core C header files to define
// the kind of information we need below (e.g. C99 compatibility)



// Determine if this compiler is ANSI C compliant and if it is C99 compliant.
#if defined(__STDC__)
#define ABEL_COMPILER_IS_ANSIC 1    // The compiler claims to be ANSI C

// Is the compiler a C99 compiler or equivalent?
// From ISO/IEC 9899:1999:
//    6.10.8 Predefined macro names
//    __STDC_VERSION__ The integer constant 199901L. (150)
//
//    150) This macro was not specified in ISO/IEC 9899:1990 and was
//    specified as 199409L in ISO/IEC 9899/AMD1:1995. The intention
//    is that this will remain an integer constant of type long int
//    that is increased with each revision of this International Standard.
//
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define ABEL_COMPILER_IS_C99 1
#endif

// Is the compiler a C11 compiler?
// From ISO/IEC 9899:2011:
//   Page 176, 6.10.8.1 (Predefined macro names) :
//   __STDC_VERSION__ The integer constant 201112L. (178)
//
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define ABEL_COMPILER_IS_C11 1
#endif
#endif

// Some compilers (e.g. GCC) define __USE_ISOC99 if they are not
// strictly C99 compilers (or are simply C++ compilers) but are set
// to use C99 functionality. Metrowerks defines _MSL_C99 as 1 in
// this case, but 0 otherwise.
#if (defined(__USE_ISOC99) || (defined(_MSL_C99) && (_MSL_C99 == 1))) && !defined(ABEL_COMPILER_IS_C99)
#define ABEL_COMPILER_IS_C99 1
#endif

// Metrowerks defines C99 types (e.g. intptr_t) instrinsically when in C99 mode (-lang C99 on the command line).
#if (defined(_MSL_C99) && (_MSL_C99 == 1))
#define ABEL_COMPILER_HAS_C99_TYPES 1
#endif

#if defined(__GNUC__)
#if (((__GNUC__ * 100) + __GNUC_MINOR__) >= 302) // Also, GCC defines _HAS_C9X.
#define ABEL_COMPILER_HAS_C99_TYPES 1 // The compiler is not necessarily a C99 compiler, but it defines C99 types.

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1    // This tells the GCC compiler that we want it to use its native C99 types.
#endif
#endif
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#define ABEL_COMPILER_HAS_C99_TYPES 1
#endif

#ifdef  __cplusplus
#define ABEL_COMPILER_IS_CPLUSPLUS 1
#endif


// ------------------------------------------------------------------------
// ABEL_PREPROCESSOR_JOIN
//
// This macro joins the two arguments together, even when one of
// the arguments is itself a macro (see 16.3.1 in C++98 standard).
// This is often used to create a unique name with __LINE__.
//
// For example, this declaration:
//    char ABEL_PREPROCESSOR_JOIN(unique_, __LINE__);
// expands to this:
//    char unique_73;
//
// Note that all versions of MSVC++ up to at least version 7.1
// fail to properly compile macros that use __LINE__ in them
// when the "program database for edit and continue" option
// is enabled. The result is that __LINE__ gets converted to
// something like __LINE__(Var+37).
//
#ifndef ABEL_PREPROCESSOR_JOIN
#define ABEL_PREPROCESSOR_JOIN(a, b)  ABEL_PREPROCESSOR_JOIN1(a, b)
#define ABEL_PREPROCESSOR_JOIN1(a, b) ABEL_PREPROCESSOR_JOIN2(a, b)
#define ABEL_PREPROCESSOR_JOIN2(a, b) a##b
#endif


// ------------------------------------------------------------------------
// ABEL_STRINGIFY
//
// Example usage:
//     printf("Line: %s", ABEL_STRINGIFY(__LINE__));
//
#ifndef ABEL_STRINGIFY
#define ABEL_STRINGIFY(x)     ABEL_STRINGIFYIMPL(x)
#define ABEL_STRINGIFYIMPL(x) #x
#endif


// ------------------------------------------------------------------------
// ABEL_IDENTITY
//
#ifndef ABEL_IDENTITY
#define ABEL_IDENTITY(x) x
#endif


// ------------------------------------------------------------------------
// ABEL_COMPILER_MANAGED_CPP
// Defined if this is being compiled with Managed C++ extensions
#ifdef ABEL_COMPILER_MSVC
#if ABEL_COMPILER_VERSION >= 1300
#ifdef _MANAGED
#define ABEL_COMPILER_MANAGED_CPP 1
#endif
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_COMPILER_INTMAX_SIZE
//
// This is related to the concept of intmax_t uintmax_t, but is available
// in preprocessor form as opposed to compile-time form. At compile-time
// you can use intmax_t and uintmax_t to use the actual types.
//
#if defined(__GNUC__) && defined(__x86_64__)
#define ABEL_COMPILER_INTMAX_SIZE 16  // intmax_t is __int128_t (GCC extension) and is 16 bytes.
#else
#define ABEL_COMPILER_INTMAX_SIZE 8   // intmax_t is int64_t and is 8 bytes.
#endif



// ------------------------------------------------------------------------
// ABEL_LPAREN / ABEL_RPAREN / ABEL_COMMA / ABEL_SEMI
//
// These are used for using special characters in macro-using expressions.
// Note that this macro intentionally uses (), as in some cases it can't
// work unless it does.
//
// Example usage:
//     int x = SOME_MACRO(SomeTemplate<int ABEL_COMMA() int CBCOMMA() char>);
//
#ifndef ABEL_LPAREN
#define ABEL_LPAREN() (
#endif
#ifndef ABEL_RPAREN
#define ABEL_RPAREN() )
#endif
#ifndef ABEL_COMMA
#define ABEL_COMMA()  ,
#endif
#ifndef ABEL_SEMI
#define ABEL_SEMI()   ;
#endif




// ------------------------------------------------------------------------
// ABEL_OFFSETOF
// Implements a portable version of the non-standard offsetof macro.
//
// The offsetof macro is guaranteed to only work with POD types. However, we wish to use
// it for non-POD types but where we know that offsetof will still work for the cases
// in which we use it. GCC unilaterally gives a warning when using offsetof with a non-POD,
// even if the given usage happens to work. So we make a workaround version of offsetof
// here for GCC which has the same effect but tricks the compiler into not issuing the warning.
// The 65536 does the compiler fooling; the reinterpret_cast prevents the possibility of
// an overloaded operator& for the class getting in the way.
//
// Example usage:
//     struct A{ int x; int y; };
//     size_t n = ABEL_OFFSETOF(A, y);
//
#if defined(__GNUC__)                       // We can't use GCC 4's __builtin_offsetof because it mistakenly complains about non-PODs that are really PODs.
#define ABEL_OFFSETOF(struct_, member_)  ((size_t)(((uintptr_t)&reinterpret_cast<const volatile char&>((((struct_*)65536)->member_))) - 65536))
#else
#define ABEL_OFFSETOF(struct_, member_)  offsetof(struct_, member_)
#endif

// ------------------------------------------------------------------------
// ABEL_SIZEOF_MEMBER
// Implements a portable way to determine the size of a member.
//
// The ABEL_SIZEOF_MEMBER simply returns the size of a member within a class or struct; member
// access rules still apply. We offer two approaches depending on the compiler's support for non-static member
// initializers although most C++11 compilers support this.
//
// Example usage:
//     struct A{ int x; int y; };
//     size_t n = ABEL_SIZEOF_MEMBER(A, y);
//
#ifndef ABEL_COMPILER_NO_EXTENDED_SIZEOF
#define ABEL_SIZEOF_MEMBER(struct_, member_) (sizeof(struct_::member_))
#else
#define ABEL_SIZEOF_MEMBER(struct_, member_) (sizeof(((struct_*)0)->member_))
#endif

// ------------------------------------------------------------------------
// alignment expressions
//
// Here we define
//    ABEL_ALIGN_OF(type)         // Returns size_t.
//    ABEL_ALIGN_MAX_STATIC       // The max align value that the compiler will respect for ABEL_ALIGN for static data (global and static variables). Some compilers allow high values, some allow no more than 8. ABEL_ALIGN_MIN is assumed to be 1.
//    ABEL_ALIGN_MAX_AUTOMATIC    // The max align value for automatic variables (variables declared as local to a function).
//    ABEL_ALIGN(n)               // Used as a prefix. n is byte alignment, with being a power of two. Most of the time you can use this and avoid using ABEL_PREFIX_ALIGN/ABEL_POSTFIX_ALIGN.
//    ABEL_ALIGNED(t, v, n)       // Type, variable, alignment. Used to align an instance. You should need this only for unusual compilers.
//    ABEL_PACKED                 // Specifies that the given structure be packed (and not have its members aligned).
//
// Also we define the following for rare cases that it's needed.
//    ABEL_PREFIX_ALIGN(n)        // n is byte alignment, with being a power of two. You should need this only for unusual compilers.
//    ABEL_POSTFIX_ALIGN(n)       // Valid values for n are 1, 2, 4, 8, etc. You should need this only for unusual compilers.
//
// Example usage:
//    size_t x = ABEL_ALIGN_OF(int);                                  Non-aligned equivalents.        Meaning
//    ABEL_PREFIX_ALIGN(8) int x = 5;                                 int x = 5;                      Align x on 8 for compilers that require prefix attributes. Can just use ABEL_ALIGN instead.
//    ABEL_ALIGN(8) int x;                                            int x;                          Align x on 8 for compilers that allow prefix attributes.
//    int x ABEL_POSTFIX_ALIGN(8);                                    int x;                          Align x on 8 for compilers that require postfix attributes.
//    int x ABEL_POSTFIX_ALIGN(8) = 5;                                int x = 5;                      Align x on 8 for compilers that require postfix attributes.
//    int x ABEL_POSTFIX_ALIGN(8)(5);                                 int x(5);                       Align x on 8 for compilers that require postfix attributes.
//    struct ABEL_PREFIX_ALIGN(8) X { int x; } ABEL_POSTFIX_ALIGN(8);   struct X { int x; };            Define X as a struct which is aligned on 8 when used.
//    ABEL_ALIGNED(int, x, 8) = 5;                                    int x = 5;                      Align x on 8.
//    ABEL_ALIGNED(int, x, 16)(5);                                    int x(5);                       Align x on 16.
//    ABEL_ALIGNED(int, x[3], 16);                                    int x[3];                       Align x array on 16.
//    ABEL_ALIGNED(int, x[3], 16) = { 1, 2, 3 };                      int x[3] = { 1, 2, 3 };         Align x array on 16.
//    int x[3] ABEL_PACKED;                                           int x[3];                       Pack the 3 ints of the x array. GCC doesn't seem to support packing of int arrays.
//    struct ABEL_ALIGN(32) X { int x; int y; };                      struct X { int x; };            Define A as a struct which is aligned on 32 when used.
//    ABEL_ALIGN(32) struct X { int x; int y; } Z;                    struct X { int x; } Z;          Define A as a struct, and align the instance Z on 32.
//    struct X { int x ABEL_PACKED; int y ABEL_PACKED; };               struct X { int x; int y; };     Pack the x and y members of struct X.
//    struct X { int x; int y; } ABEL_PACKED;                         struct X { int x; int y; };     Pack the members of struct X.
//    typedef ABEL_ALIGNED(int, int16, 16); int16 n16;                typedef int int16; int16 n16;   Define int16 as an int which is aligned on 16.
//    typedef ABEL_ALIGNED(X, X16, 16); X16 x16;                      typedef X X16; X16 x16;         Define X16 as an X which is aligned on 16.

#if !defined(ABEL_ALIGN_MAX)          // If the user hasn't globally set an alternative value...
#if defined(ABEL_PROCESSOR_ARM)                       // ARM compilers in general tend to limit automatic variables to 8 or less.
#define ABEL_ALIGN_MAX_STATIC    1048576
#define ABEL_ALIGN_MAX_AUTOMATIC       1          // Typically they support only built-in natural aligment types (both arm-eabi and apple-abi).
#elif defined(ABEL_PLATFORM_APPLE)
#define ABEL_ALIGN_MAX_STATIC    1048576
#define ABEL_ALIGN_MAX_AUTOMATIC      16
#else
#define ABEL_ALIGN_MAX_STATIC    1048576          // Arbitrarily high value. What is the actual max?
#define ABEL_ALIGN_MAX_AUTOMATIC 1048576
#endif
#endif

// EDG intends to be compatible with GCC but has a bug whereby it
// fails to support calling a constructor in an aligned declaration when
// using postfix alignment attributes. Prefix works for alignment, but does not align
// the size like postfix does.  Prefix also fails on templates.  So gcc style post fix
// is still used, but the user will need to use ABEL_POSTFIX_ALIGN before the constructor parameters.
#if defined(__GNUC__) && (__GNUC__ < 3)
#define ABEL_ALIGN_OF(type) ((size_t)__alignof__(type))
#define ABEL_ALIGN(n)
#define ABEL_PREFIX_ALIGN(n)
#define ABEL_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
#define ABEL_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((aligned(n)))
#define ABEL_PACKED __attribute__((packed))

// GCC 3.x+, IBM, and clang support prefix attributes.
#elif (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__xlC__) || defined(__clang__)
#define ABEL_ALIGN_OF(type) ((size_t)__alignof__(type))
#define ABEL_ALIGN(n) __attribute__((aligned(n)))
#define ABEL_PREFIX_ALIGN(n)
#define ABEL_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
#define ABEL_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((aligned(n)))
#define ABEL_PACKED __attribute__((packed))

// Metrowerks supports prefix attributes.
// Metrowerks does not support packed alignment attributes.
#elif defined(ABEL_COMPILER_INTEL) || defined(CS_UNDEFINED_STRING) || (defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1300))
#define ABEL_ALIGN_OF(type) ((size_t)__alignof(type))
#define ABEL_ALIGN(n) __declspec(align(n))
#define ABEL_PREFIX_ALIGN(n) ABEL_ALIGN(n)
#define ABEL_POSTFIX_ALIGN(n)
#define ABEL_ALIGNED(variable_type, variable, n) ABEL_ALIGN(n) variable_type variable
#define ABEL_PACKED // See ABEL_PRAGMA_PACK_VC for an alternative.

// Arm brand compiler
#elif defined(ABEL_COMPILER_ARM)
#define ABEL_ALIGN_OF(type) ((size_t)__ALIGNOF__(type))
#define ABEL_ALIGN(n) __align(n)
#define ABEL_PREFIX_ALIGN(n) __align(n)
#define ABEL_POSTFIX_ALIGN(n)
#define ABEL_ALIGNED(variable_type, variable, n) __align(n) variable_type variable
#define ABEL_PACKED __packed

#else // Unusual compilers
// There is nothing we can do about some of these. This is not as bad a problem as it seems.
// If the given platform/compiler doesn't support alignment specifications, then it's somewhat
// likely that alignment doesn't matter for that platform. Otherwise they would have defined
// functionality to manipulate alignment.
#define ABEL_ALIGN(n)
#define ABEL_PREFIX_ALIGN(n)
#define ABEL_POSTFIX_ALIGN(n)
#define ABEL_ALIGNED(variable_type, variable, n) variable_type variable
#define ABEL_PACKED

#ifdef __cplusplus
        template <typename T> struct CBAlignOf1 { enum { s = sizeof (T), value = s ^ (s & (s - 1)) }; };
            template <typename T> struct CBlignOf2;
            template <int size_diff> struct helper { template <typename T> struct Val { enum { value = size_diff }; }; };
            template <> struct helper<0> { template <typename T> struct Val { enum { value = CBlignOf2<T>::value }; }; };
            template <typename T> struct CBAlignOf2 { struct Big { T x; char c; };
            enum { diff = sizeof (Big) - sizeof (T), value = helper<diff>::template Val<Big>::value }; };
            template <typename T> struct CBAlignof3 { enum { x = CBAlignOf2<T>::value, y = CBlignOf1<T>::value, value = x < y ? x : y }; };
            template <typename T> struct CBAlignof3 { enum { x = CBAlignOf2<T>::value, y = CBlignOf1<T>::value, value = x < y ? x : y }; };
#define ABEL_ALIGN_OF(type) ((size_t)CBAlignof3<type>::value)

#else
// C implementation of ABEL_ALIGN_OF
// This implementation works for most cases, but doesn't directly work
// for types such as function pointer declarations. To work with those
// types you need to typedef the type and then use the typedef in ABEL_ALIGN_OF.
#define ABEL_ALIGN_OF(type) ((size_t)offsetof(struct { char c; type m; }, m))
#endif
#endif

// ABEL_PRAGMA_PACK_VC
//
// Wraps #pragma pack in a way that allows for cleaner code.
//
// Example usage:
//    ABEL_PRAGMA_PACK_VC(push, 1)
//    struct X{ char c; int i; };
//    ABEL_PRAGMA_PACK_VC(pop)
//
#if !defined(ABEL_PRAGMA_PACK_VC)
#if defined(ABEL_COMPILER_MSVC)
#define ABEL_PRAGMA_PACK_VC(...) __pragma(pack(__VA_ARGS__))
#elif !defined(ABEL_COMPILER_NO_VARIADIC_MACROS)
#define ABEL_PRAGMA_PACK_VC(...)
#else
// No support. However, all compilers of significance to us support variadic macros.
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_LIKELY / ABEL_UNLIKELY
//
// Defined as a macro which gives a hint to the compiler for branch
// prediction. GCC gives you the ability to manually give a hint to
// the compiler about the result of a comparison, though it's often
// best to compile shipping code with profiling feedback under both
// GCC (-fprofile-arcs) and VC++ (/LTCG:PGO, etc.). However, there
// are times when you feel very sure that a boolean expression will
// usually evaluate to either true or false and can help the compiler
// by using an explicity directive...
//
// Example usage:
//    if(ABEL_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
//       { ... }
//
// Example usage:
//    if(ABEL_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
//       { ... }
//
#ifndef ABEL_LIKELY
#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
#if defined(__cplusplus)
#define ABEL_LIKELY(x)   __builtin_expect(!!(x), true)
#define ABEL_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define ABEL_LIKELY(x)   __builtin_expect(!!(x), 1)
#define ABEL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
#else
#define ABEL_LIKELY(x)   (x)
#define ABEL_UNLIKELY(x) (x)
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_INIT_PRIORITY_AVAILABLE
//
// This value is either not defined, or defined to 1.
// Defines if the GCC attribute init_priority is supported by the compiler.
//
#if !defined(ABEL_INIT_PRIORITY_AVAILABLE)
#if defined(__GNUC__) && !defined(__EDG__) // EDG typically #defines __GNUC__ but doesn't implement init_priority.
#define ABEL_INIT_PRIORITY_AVAILABLE 1
#elif defined(__clang__)
#define ABEL_INIT_PRIORITY_AVAILABLE 1  // Clang implements init_priority
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_INIT_PRIORITY
//
// This is simply a wrapper for the GCC init_priority attribute that allows
// multiplatform code to be easier to read. This attribute doesn't apply
// to VC++ because VC++ uses file-level pragmas to control init ordering.
//
// Example usage:
//     SomeClass gSomeClass ABEL_INIT_PRIORITY(2000);
//
#if !defined(ABEL_INIT_PRIORITY)
#if defined(ABEL_INIT_PRIORITY_AVAILABLE)
#define ABEL_INIT_PRIORITY(x)  __attribute__ ((init_priority (x)))
#else
#define ABEL_INIT_PRIORITY(x)
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_MAY_ALIAS_AVAILABLE
//
// Defined as 0, 1, or 2.
// Defines if the GCC attribute may_alias is supported by the compiler.
// Consists of a value 0 (unsupported, shouldn't be used), 1 (some support),
// or 2 (full proper support).
//
#ifndef ABEL_MAY_ALIAS_AVAILABLE
#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303)
#if   !defined(__EDG__)                 // define it as 1 while defining GCC's support as 2.
#define ABEL_MAY_ALIAS_AVAILABLE 2
#else
#define ABEL_MAY_ALIAS_AVAILABLE 0
#endif
#else
#define ABEL_MAY_ALIAS_AVAILABLE 0
#endif
#endif


// ABEL_MAY_ALIAS
//
// Defined as a macro that wraps the GCC may_alias attribute. This attribute
// has no significance for VC++ because VC++ doesn't support the concept of
// strict aliasing. Users should avoid writing code that breaks strict
// aliasing rules; ABEL_MAY_ALIAS is for cases with no alternative.
//
// Example usage:
//    void* ABEL_MAY_ALIAS gPtr = NULL;
//
// Example usage:
//    typedef void* ABEL_MAY_ALIAS pvoid_may_alias;
//    pvoid_may_alias gPtr = NULL;
//
#if ABEL_MAY_ALIAS_AVAILABLE
#define ABEL_MAY_ALIAS __attribute__((__may_alias__))
#else
#define ABEL_MAY_ALIAS
#endif


// ------------------------------------------------------------------------
// ABEL_ASSUME
//
// This acts the same as the VC++ __assume directive and is implemented
// simply as a wrapper around it to allow portable usage of it and to take
// advantage of it if and when it appears in other compilers.
//
// Example usage:
//    void Function(int a) {
//       switch(a) {
//          case 1:
//             DoSomething(1);
//             break;
//          case 2:
//             DoSomething(-1);
//             break;
//          default:
//             ABEL_ASSUME(0); // This tells the optimizer that the default cannot be reached.
//       }
//    }
//
#ifndef ABEL_ASSUME
#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later
#define ABEL_ASSUME(x) __assume(x)
#else
#define ABEL_ASSUME(x)
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_ANALYSIS_ASSUME
//
// This acts the same as the VC++ __analysis_assume directive and is implemented
// simply as a wrapper around it to allow portable usage of it and to take
// advantage of it if and when it appears in other compilers.
//
// Example usage:
//    char Function(char* p) {
//       ABEL_ANALYSIS_ASSUME(p != NULL);
//       return *p;
//    }
//
#ifndef ABEL_ANALYSIS_ASSUME
#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later
#define ABEL_ANALYSIS_ASSUME(x) __analysis_assume(!!(x)) // !! because that allows for convertible-to-bool in addition to bool.
#else
#define ABEL_ANALYSIS_ASSUME(x)
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_DISABLE_VC_WARNING / ABEL_RESTORE_VC_WARNING
//
// Disable and re-enable warning(s) within code.
// This is simply a wrapper for VC++ #pragma warning(disable: nnnn) for the
// purpose of making code easier to read due to avoiding nested compiler ifdefs
// directly in code.
//
// Example usage:
//     ABEL_DISABLE_VC_WARNING(4127 3244)
//     <code>
//     ABEL_RESTORE_VC_WARNING()
//
#ifndef ABEL_DISABLE_VC_WARNING
#if defined(_MSC_VER)
#define ABEL_DISABLE_VC_WARNING(w)  \
                __pragma(warning(push))       \
                __pragma(warning(disable:w))
#else
#define ABEL_DISABLE_VC_WARNING(w)
#endif
#endif

#ifndef ABEL_RESTORE_VC_WARNING
#if defined(_MSC_VER)
#define ABEL_RESTORE_VC_WARNING()   \
                __pragma(warning(pop))
#else
#define ABEL_RESTORE_VC_WARNING()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_ENABLE_VC_WARNING_AS_ERROR / ABEL_DISABLE_VC_WARNING_AS_ERROR
//
// Disable and re-enable treating a warning as error within code.
// This is simply a wrapper for VC++ #pragma warning(error: nnnn) for the
// purpose of making code easier to read due to avoiding nested compiler ifdefs
// directly in code.
//
// Example usage:
//     ABEL_ENABLE_VC_WARNING_AS_ERROR(4996)
//     <code>
//     ABEL_DISABLE_VC_WARNING_AS_ERROR()
//
#ifndef ABEL_ENABLE_VC_WARNING_AS_ERROR
#if defined(_MSC_VER)
#define ABEL_ENABLE_VC_WARNING_AS_ERROR(w) \
                    __pragma(warning(push)) \
                    __pragma(warning(error:w))
#else
#define ABEL_ENABLE_VC_WARNING_AS_ERROR(w)
#endif
#endif

#ifndef ABEL_DISABLE_VC_WARNING_AS_ERROR
#if defined(_MSC_VER)
#define ABEL_DISABLE_VC_WARNING_AS_ERROR() \
                    __pragma(warning(pop))
#else
#define ABEL_DISABLE_VC_WARNING_AS_ERROR()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_GCC_WARNING / ABEL_RESTORE_GCC_WARNING
//
// Example usage:
//     // Only one warning can be ignored per statement, due to how GCC works.
//     ABEL_DISABLE_GCC_WARNING(-Wuninitialized)
//     ABEL_DISABLE_GCC_WARNING(-Wunused)
//     <code>
//     ABEL_RESTORE_GCC_WARNING()
//     ABEL_RESTORE_GCC_WARNING()
//
#ifndef ABEL_DISABLE_GCC_WARNING
#if defined(ABEL_COMPILER_GNUC)
#define CBGCCWHELP0(x) #x
#define CBGCCWHELP1(x) CBGCCWHELP0(GCC diagnostic ignored x)
#define CBGCCWHELP2(x) CBGCCWHELP1(#x)
#endif

#if defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006) // Can't test directly for __GNUC__ because some compilers lie.
#define ABEL_DISABLE_GCC_WARNING(w)   \
                _Pragma("GCC diagnostic push")  \
                _Pragma(CBGCCWHELP2(w))
#elif defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004)
#define ABEL_DISABLE_GCC_WARNING(w)   \
                _Pragma(CBGCCWHELP2(w))
#else
#define ABEL_DISABLE_GCC_WARNING(w)
#endif
#endif

#ifndef ABEL_RESTORE_GCC_WARNING
#if defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006)
#define ABEL_RESTORE_GCC_WARNING()    \
                _Pragma("GCC diagnostic pop")
#else
#define ABEL_RESTORE_GCC_WARNING()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_ALL_GCC_WARNINGS / ABEL_RESTORE_ALL_GCC_WARNINGS
//
// This isn't possible except via using _Pragma("GCC system_header"), though
// that has some limitations in how it works. Another means is to manually
// disable individual warnings within a GCC diagnostic push statement.
// GCC doesn't have as many warnings as VC++ and EDG and so this may be feasible.
// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// ABEL_ENABLE_GCC_WARNING_AS_ERROR / ABEL_DISABLE_GCC_WARNING_AS_ERROR
//
// Example usage:
//     // Only one warning can be treated as an error per statement, due to how GCC works.
//     ABEL_ENABLE_GCC_WARNING_AS_ERROR(-Wuninitialized)
//     ABEL_ENABLE_GCC_WARNING_AS_ERROR(-Wunused)
//     <code>
//     ABEL_DISABLE_GCC_WARNING_AS_ERROR()
//     ABEL_DISABLE_GCC_WARNING_AS_ERROR()
//
#ifndef ABEL_ENABLE_GCC_WARNING_AS_ERROR
#if defined(ABEL_COMPILER_GNUC)
#define CBGCCWERRORHELP0(x) #x
#define CBGCCWERRORHELP1(x) CBGCCWERRORHELP0(GCC diagnostic error x)
#define CBGCCWERRORHELP2(x) CBGCCWERRORHELP1(#x)
#endif

#if defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006) // Can't test directly for __GNUC__ because some compilers lie.
#define ABEL_ENABLE_GCC_WARNING_AS_ERROR(w)   \
                _Pragma("GCC diagnostic push")  \
                _Pragma(CBGCCWERRORHELP2(w))
#elif defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004)
#define ABEL_DISABLE_GCC_WARNING(w)   \
                _Pragma(CBGCCWERRORHELP2(w))
#else
#define ABEL_DISABLE_GCC_WARNING(w)
#endif
#endif

#ifndef ABEL_DISABLE_GCC_WARNING_AS_ERROR
#if defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006)
#define ABEL_DISABLE_GCC_WARNING_AS_ERROR()    \
                _Pragma("GCC diagnostic pop")
#else
#define ABEL_DISABLE_GCC_WARNING_AS_ERROR()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_CLANG_WARNING / ABEL_RESTORE_CLANG_WARNING
//
// Example usage:
//     // Only one warning can be ignored per statement, due to how clang works.
//     ABEL_DISABLE_CLANG_WARNING(-Wuninitialized)
//     ABEL_DISABLE_CLANG_WARNING(-Wunused)
//     <code>
//     ABEL_RESTORE_CLANG_WARNING()
//     ABEL_RESTORE_CLANG_WARNING()
//
#ifndef ABEL_DISABLE_CLANG_WARNING
#if defined(ABEL_COMPILER_CLANG) || defined(ABEL_COMPILER_CLANG_CL)
#define CBCLANGWHELP0(x) #x
#define CBCLANGWHELP1(x) CBCLANGWHELP0(clang diagnostic ignored x)
#define CBCLANGWHELP2(x) CBCLANGWHELP1(#x)

#define ABEL_DISABLE_CLANG_WARNING(w)   \
                _Pragma("clang diagnostic push")  \
                _Pragma(CBCLANGWHELP2(-Wunknown-warning-option))\
                _Pragma(CBCLANGWHELP2(w))
#else
#define ABEL_DISABLE_CLANG_WARNING(w)
#endif
#endif

#ifndef ABEL_RESTORE_CLANG_WARNING
#if defined(ABEL_COMPILER_CLANG) || defined(ABEL_COMPILER_CLANG_CL)
#define ABEL_RESTORE_CLANG_WARNING()    \
                _Pragma("clang diagnostic pop")
#else
#define ABEL_RESTORE_CLANG_WARNING()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_ALL_CLANG_WARNINGS / ABEL_RESTORE_ALL_CLANG_WARNINGS
//
// The situation for clang is the same as for GCC. See above.
// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// ABEL_ENABLE_CLANG_WARNING_AS_ERROR / ABEL_DISABLE_CLANG_WARNING_AS_ERROR
//
// Example usage:
//     // Only one warning can be treated as an error per statement, due to how clang works.
//     ABEL_ENABLE_CLANG_WARNING_AS_ERROR(-Wuninitialized)
//     ABEL_ENABLE_CLANG_WARNING_AS_ERROR(-Wunused)
//     <code>
//     ABEL_DISABLE_CLANG_WARNING_AS_ERROR()
//     ABEL_DISABLE_CLANG_WARNING_AS_ERROR()
//
#ifndef ABEL_ENABLE_CLANG_WARNING_AS_ERROR
#if defined(ABEL_COMPILER_CLANG) || defined(ABEL_COMPILER_CLANG_CL)
#define CBCLANGWERRORHELP0(x) #x
#define CBCLANGWERRORHELP1(x) CBCLANGWERRORHELP0(clang diagnostic error x)
#define CBCLANGWERRORHELP2(x) CBCLANGWERRORHELP1(#x)

#define ABEL_ENABLE_CLANG_WARNING_AS_ERROR(w)   \
                _Pragma("clang diagnostic push")  \
                _Pragma(CBCLANGWERRORHELP2(w))
#else
#define ABEL_DISABLE_CLANG_WARNING(w)
#endif
#endif

#ifndef ABEL_DISABLE_CLANG_WARNING_AS_ERROR
#if defined(ABEL_COMPILER_CLANG) || defined(ABEL_COMPILER_CLANG_CL)
#define ABEL_DISABLE_CLANG_WARNING_AS_ERROR()    \
                _Pragma("clang diagnostic pop")
#else
#define ABEL_DISABLE_CLANG_WARNING_AS_ERROR()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_SN_WARNING / ABEL_RESTORE_SN_WARNING
//
// Note that we define this macro specifically for the SN compiler instead of
// having a generic one for EDG-based compilers. The reason for this is that
// while SN is indeed based on EDG, SN has different warning value mappings
// and thus warning 1234 for SN is not the same as 1234 for all other EDG compilers.
//
// Example usage:
//     // Currently we are limited to one warning per line.
//     ABEL_DISABLE_SN_WARNING(1787)
//     ABEL_DISABLE_SN_WARNING(552)
//     <code>
//     ABEL_RESTORE_SN_WARNING()
//     ABEL_RESTORE_SN_WARNING()
//
#ifndef ABEL_DISABLE_SN_WARNING
#define ABEL_DISABLE_SN_WARNING(w)
#endif

#ifndef ABEL_RESTORE_SN_WARNING
#define ABEL_RESTORE_SN_WARNING()
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_ALL_SN_WARNINGS / ABEL_RESTORE_ALL_SN_WARNINGS
//
// Example usage:
//     ABEL_DISABLE_ALL_SN_WARNINGS()
//     <code>
//     ABEL_RESTORE_ALL_SN_WARNINGS()
//
#ifndef ABEL_DISABLE_ALL_SN_WARNINGS
#define ABEL_DISABLE_ALL_SN_WARNINGS()
#endif

#ifndef ABEL_RESTORE_ALL_SN_WARNINGS
#define ABEL_RESTORE_ALL_SN_WARNINGS()
#endif



// ------------------------------------------------------------------------
// ABEL_DISABLE_GHS_WARNING / ABEL_RESTORE_GHS_WARNING
//
// Disable warnings from the Green Hills compiler.
//
// Example usage:
//     ABEL_DISABLE_GHS_WARNING(193)
//     ABEL_DISABLE_GHS_WARNING(236, 5323)
//     <code>
//     ABEL_RESTORE_GHS_WARNING()
//     ABEL_RESTORE_GHS_WARNING()
//
#ifndef ABEL_DISABLE_GHS_WARNING
#define ABEL_DISABLE_GHS_WARNING(w)
#endif

#ifndef ABEL_RESTORE_GHS_WARNING
#define ABEL_RESTORE_GHS_WARNING()
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_ALL_GHS_WARNINGS / ABEL_RESTORE_ALL_GHS_WARNINGS
//
// #ifndef ABEL_DISABLE_ALL_GHS_WARNINGS
//     #if defined(ABEL_COMPILER_GREEN_HILLS)
//         #define ABEL_DISABLE_ALL_GHS_WARNINGS(w)  \_
//             _Pragma("_________")
//     #else
//         #define ABEL_DISABLE_ALL_GHS_WARNINGS(w)
//     #endif
// #endif
//
// #ifndef ABEL_RESTORE_ALL_GHS_WARNINGS
//     #if defined(ABEL_COMPILER_GREEN_HILLS)
//         #define ABEL_RESTORE_ALL_GHS_WARNINGS()   \_
//             _Pragma("_________")
//     #else
//         #define ABEL_RESTORE_ALL_GHS_WARNINGS()
//     #endif
// #endif



// ------------------------------------------------------------------------
// ABEL_DISABLE_EDG_WARNING / ABEL_RESTORE_EDG_WARNING
//
// Example usage:
//     // Currently we are limited to one warning per line.
//     ABEL_DISABLE_EDG_WARNING(193)
//     ABEL_DISABLE_EDG_WARNING(236)
//     <code>
//     ABEL_RESTORE_EDG_WARNING()
//     ABEL_RESTORE_EDG_WARNING()
//
#ifndef ABEL_DISABLE_EDG_WARNING
// EDG-based compilers are inconsistent in how the implement warning pragmas.
#if defined(ABEL_COMPILER_EDG) && !defined(ABEL_COMPILER_INTEL) && !defined(ABEL_COMPILER_RVCT)
#define CBEDGWHELP0(x) #x
#define CBEDGWHELP1(x) CBEDGWHELP0(diag_suppress x)

#define ABEL_DISABLE_EDG_WARNING(w)   \
                _Pragma("control %push diag")   \
                _Pragma(CBEDGWHELP1(w))
#else
#define ABEL_DISABLE_EDG_WARNING(w)
#endif
#endif

#ifndef ABEL_RESTORE_EDG_WARNING
#if defined(ABEL_COMPILER_EDG) && !defined(ABEL_COMPILER_INTEL) && !defined(ABEL_COMPILER_RVCT)
#define ABEL_RESTORE_EDG_WARNING()   \
                _Pragma("control %pop diag")
#else
#define ABEL_RESTORE_EDG_WARNING()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_ALL_EDG_WARNINGS / ABEL_RESTORE_ALL_EDG_WARNINGS
//
//#ifndef ABEL_DISABLE_ALL_EDG_WARNINGS
//    #if defined(ABEL_COMPILER_EDG) && !defined(ABEL_COMPILER_SN)
//        #define ABEL_DISABLE_ALL_EDG_WARNINGS(w)  \_
//            _Pragma("_________")
//    #else
//        #define ABEL_DISABLE_ALL_EDG_WARNINGS(w)
//    #endif
//#endif
//
//#ifndef ABEL_RESTORE_ALL_EDG_WARNINGS
//    #if defined(ABEL_COMPILER_EDG) && !defined(ABEL_COMPILER_SN)
//        #define ABEL_RESTORE_ALL_EDG_WARNINGS()   \_
//            _Pragma("_________")
//    #else
//        #define ABEL_RESTORE_ALL_EDG_WARNINGS()
//    #endif
//#endif



// ------------------------------------------------------------------------
// ABEL_DISABLE_CW_WARNING / ABEL_RESTORE_CW_WARNING
//
// Note that this macro can only control warnings via numbers and not by
// names. The reason for this is that the compiler's syntax for such
// warnings is not the same as for numbers.
//
// Example usage:
//     // Currently we are limited to one warning per line and must also specify the warning in the restore macro.
//     ABEL_DISABLE_CW_WARNING(10317)
//     ABEL_DISABLE_CW_WARNING(10324)
//     <code>
//     ABEL_RESTORE_CW_WARNING(10317)
//     ABEL_RESTORE_CW_WARNING(10324)
//
#ifndef ABEL_DISABLE_CW_WARNING
#define ABEL_DISABLE_CW_WARNING(w)
#endif

#ifndef ABEL_RESTORE_CW_WARNING

#define ABEL_RESTORE_CW_WARNING(w)

#endif


// ------------------------------------------------------------------------
// ABEL_DISABLE_ALL_CW_WARNINGS / ABEL_RESTORE_ALL_CW_WARNINGS
//
#ifndef ABEL_DISABLE_ALL_CW_WARNINGS
#define ABEL_DISABLE_ALL_CW_WARNINGS()

#endif

#ifndef ABEL_RESTORE_ALL_CW_WARNINGS
#define ABEL_RESTORE_ALL_CW_WARNINGS()
#endif



// ------------------------------------------------------------------------
// ABEL_PURE
//
// This acts the same as the GCC __attribute__ ((pure)) directive and is
// implemented simply as a wrapper around it to allow portable usage of
// it and to take advantage of it if and when it appears in other compilers.
//
// A "pure" function is one that has no effects except its return value and
// its return value is a function of only the function's parameters or
// non-volatile global variables. Any parameter or global variable access
// must be read-only. Loop optimization and subexpression elimination can be
// applied to such functions. A common example is strlen(): Given identical
// inputs, the function's return value (its only effect) is invariant across
// multiple invocations and thus can be pulled out of a loop and called but once.
//
// Example usage:
//    ABEL_PURE void Function();
//
#ifndef ABEL_PURE
#if defined(ABEL_COMPILER_GNUC)
#define ABEL_PURE __attribute__((pure))
#elif defined(ABEL_COMPILER_ARM)  // Arm brand compiler for ARM CPU
#define ABEL_PURE __pure
#else
#define ABEL_PURE
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_WEAK
// ABEL_WEAK_SUPPORTED -- defined as 0 or 1.
//
// GCC
// The weak attribute causes the declaration to be emitted as a weak
// symbol rather than a global. This is primarily useful in defining
// library functions which can be overridden in user code, though it
// can also be used with non-function declarations.
//
// VC++
// At link time, if multiple definitions of a COMDAT are seen, the linker
// picks one and discards the rest. If the linker option /OPT:REF
// is selected, then COMDAT elimination will occur to remove all the
// unreferenced data items in the linker output.
//
// Example usage:
//    ABEL_WEAK void Function();
//
#ifndef ABEL_WEAK
#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later
#define ABEL_WEAK __declspec(selectany)
#define ABEL_WEAK_SUPPORTED 1
#elif defined(_MSC_VER) || (defined(__GNUC__) && defined(__CYGWIN__))
#define ABEL_WEAK
#define ABEL_WEAK_SUPPORTED 0
#elif defined(ABEL_COMPILER_ARM)  // Arm brand compiler for ARM CPU
#define ABEL_WEAK __weak
#define ABEL_WEAK_SUPPORTED 1
#else                           // GCC and IBM compilers, others.
#define ABEL_WEAK __attribute__((weak))
#define ABEL_WEAK_SUPPORTED 1
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_UNUSED
//
// Makes compiler warnings about unused variables go away.
//
// Example usage:
//    void Function(int x)
//    {
//        int y;
//        ABEL_UNUSED(x);
//        ABEL_UNUSED(y);
//    }
//
#ifndef ABEL_UNUSED
// The EDG solution below is pretty weak and needs to be augmented or replaced.
// It can't handle the C language, is limited to places where template declarations
// can be used, and requires the type x to be usable as a functions reference argument.
#if defined(__cplusplus) && defined(__EDG__)
template <typename T>
inline void CBBaseUnused(T const volatile & x) { (void)x; }
#define ABEL_UNUSED(x) CBBaseUnused(x)
#else
#define ABEL_UNUSED(x) (void)x
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_EMPTY
//
// Allows for a null statement, usually for the purpose of avoiding compiler warnings.
//
// Example usage:
//    #ifdef ABEL_DEBUG
//        #define MyDebugPrintf(x, y) printf(x, y)
//    #else
//        #define MyDebugPrintf(x, y)  ABEL_EMPTY
//    #endif
//
#ifndef ABEL_EMPTY
#define ABEL_EMPTY (void)0
#endif

#ifndef ABEL_NULL
#if defined(ABEL_COMPILER_NO_NULLPTR) && ABEL_COMPILER_NO_NULLPTR == 1
#define ABEL_NULL NULL
#else
#define ABEL_NULL nullptr
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_CURRENT_FUNCTION
//
// Provides a consistent way to get the current function name as a macro
// like the __FILE__ and __LINE__ macros work. The C99 standard specifies
// that __func__ be provided by the compiler, but most compilers don't yet
// follow that convention. However, many compilers have an alternative.
//
// We also define ABEL_CURRENT_FUNCTION_SUPPORTED for when it is not possible
// to have ABEL_CURRENT_FUNCTION work as expected.
//
// Defined inside a function because otherwise the macro might not be
// defined and code below might not compile. This happens with some
// compilers.
//
#ifndef ABEL_CURRENT_FUNCTION
#if defined __GNUC__ || (defined __ICC && __ICC >= 600)
#define ABEL_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define ABEL_CURRENT_FUNCTION __FUNCSIG__
#elif (defined __INTEL_COMPILER && __INTEL_COMPILER >= 600) || (defined __IBMCPP__ && __IBMCPP__ >= 500) || (defined CS_UNDEFINED_STRING && CS_UNDEFINED_STRING >= 0x4200)
#define ABEL_CURRENT_FUNCTION __FUNCTION__
#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901
#define ABEL_CURRENT_FUNCTION __func__
#else
#define ABEL_CURRENT_FUNCTION "(unknown function)"
#endif
#endif


// ------------------------------------------------------------------------
// wchar_t
// Here we define:
//    ABEL_WCHAR_T_NON_NATIVE
//    ABEL_WCHAR_SIZE = <sizeof(wchar_t)>
//
#ifndef ABEL_WCHAR_T_NON_NATIVE
// Compilers that always implement wchar_t as native include:
//     COMEAU, new SN, and other EDG-based compilers.
//     GCC
//     Borland
//     SunPro
//     IBM Visual Age
#if defined(ABEL_COMPILER_INTEL)
#if (ABEL_COMPILER_VERSION < 700)
#define ABEL_WCHAR_T_NON_NATIVE 1
#else
#if (!defined(_WCHAR_T_DEFINED) && !defined(_WCHAR_T))
#define ABEL_WCHAR_T_NON_NATIVE 1
#endif
#endif
#elif defined(ABEL_COMPILER_MSVC) || defined(ABEL_COMPILER_BORLAND) || (defined(ABEL_COMPILER_CLANG) && defined(ABEL_PLATFORM_WINDOWS))
#ifndef _NATIVE_WCHAR_T_DEFINED
#define ABEL_WCHAR_T_NON_NATIVE 1
#endif
#elif defined(__EDG_VERSION__) && (!defined(_WCHAR_T) && (__EDG_VERSION__ < 400)) // EDG prior to v4 uses _WCHAR_T to indicate if wchar_t is native. v4+ may define something else, but we're not currently aware of it.
#define ABEL_WCHAR_T_NON_NATIVE 1
#endif
#endif

#ifndef ABEL_WCHAR_SIZE // If the user hasn't specified that it is a given size...
#if defined(__WCHAR_MAX__) // GCC defines this for most platforms.
#if (__WCHAR_MAX__ == 2147483647) || (__WCHAR_MAX__ == 4294967295)
#define ABEL_WCHAR_SIZE 4
#elif (__WCHAR_MAX__ == 32767) || (__WCHAR_MAX__ == 65535)
#define ABEL_WCHAR_SIZE 2
#elif (__WCHAR_MAX__ == 127) || (__WCHAR_MAX__ == 255)
#define ABEL_WCHAR_SIZE 1
#else
#define ABEL_WCHAR_SIZE 4
#endif
#elif defined(WCHAR_MAX) // The SN and Arm compilers define this.
#if (WCHAR_MAX == 2147483647) || (WCHAR_MAX == 4294967295)
#define ABEL_WCHAR_SIZE 4
#elif (WCHAR_MAX == 32767) || (WCHAR_MAX == 65535)
#define ABEL_WCHAR_SIZE 2
#elif (WCHAR_MAX == 127) || (WCHAR_MAX == 255)
#define ABEL_WCHAR_SIZE 1
#else
#define ABEL_WCHAR_SIZE 4
#endif
#elif defined(__WCHAR_BIT) // Green Hills (and other versions of EDG?) uses this.
#if (__WCHAR_BIT == 16)
#define ABEL_WCHAR_SIZE 2
#elif (__WCHAR_BIT == 32)
#define ABEL_WCHAR_SIZE 4
#elif (__WCHAR_BIT == 8)
#define ABEL_WCHAR_SIZE 1
#else
#define ABEL_WCHAR_SIZE 4
#endif
#elif defined(_WCMAX) // The SN and Arm compilers define this.
#if (_WCMAX == 2147483647) || (_WCMAX == 4294967295)
#define ABEL_WCHAR_SIZE 4
#elif (_WCMAX == 32767) || (_WCMAX == 65535)
#define ABEL_WCHAR_SIZE 2
#elif (_WCMAX == 127) || (_WCMAX == 255)
#define ABEL_WCHAR_SIZE 1
#else
#define ABEL_WCHAR_SIZE 4
#endif
#elif defined(ABEL_PLATFORM_UNIX)
// It is standard on Unix to have wchar_t be int32_t or uint32_t.
// All versions of GNUC default to a 32 bit wchar_t, but CB has used
// the -fshort-wchar GCC command line option to force it to 16 bit.
// If you know that the compiler is set to use a wchar_t of other than
// the default, you need to manually define ABEL_WCHAR_SIZE for the build.
#define ABEL_WCHAR_SIZE 4
#else
// It is standard on Windows to have wchar_t be uint16_t.  GCC
// defines wchar_t as int by default.  Electronic Arts has
// standardized on wchar_t being an unsigned 16 bit value on all
// console platforms. Given that there is currently no known way to
// tell at preprocessor time what the size of wchar_t is, we declare
// it to be 2, as this is the Electronic Arts standard. If you have
// ABEL_WCHAR_SIZE != sizeof(wchar_t), then your code might not be
// broken, but it also won't work with wchar libraries and data from
// other parts of CB. Under GCC, you can force wchar_t to two bytes
// with the -fshort-wchar compiler argument.
#define ABEL_WCHAR_SIZE 2
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_RESTRICT
//
// The C99 standard defines a new keyword, restrict, which allows for the
// improvement of code generation regarding memory usage. Compilers can
// generate significantly faster code when you are able to use restrict.
//
// Example usage:
//    void DoSomething(char* ABEL_RESTRICT p1, char* ABEL_RESTRICT p2);
//
#ifndef ABEL_RESTRICT
#if defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
#define ABEL_RESTRICT __restrict
#elif defined(ABEL_COMPILER_CLANG)
#define ABEL_RESTRICT __restrict
#elif defined(ABEL_COMPILER_GNUC)     // Includes GCC and other compilers emulating GCC.
#define ABEL_RESTRICT __restrict  // GCC defines 'restrict' (as opposed to __restrict) in C99 mode only.
#elif defined(ABEL_COMPILER_ARM)
#define ABEL_RESTRICT __restrict
#elif defined(ABEL_COMPILER_IS_C99)
#define ABEL_RESTRICT restrict
#else
// If the compiler didn't support restricted pointers, defining ABEL_RESTRICT
// away would result in compiling and running fine but you just wouldn't
// the same level of optimization. On the other hand, all the major compilers
// support restricted pointers.
#define ABEL_RESTRICT
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_DEPRECATED            // Used as a prefix.
// ABEL_PREFIX_DEPRECATED     // You should need this only for unusual compilers.
// ABEL_POSTFIX_DEPRECATED    // You should need this only for unusual compilers.
// ABEL_DEPRECATED_MESSAGE    // Used as a prefix and provides a deprecation message.
//
// Example usage:
//    ABEL_DEPRECATED void Function();
//    ABEL_DEPRECATED_MESSAGE("Use 1.0v API instead") void Function();
//
// or for maximum portability:
//    ABEL_PREFIX_DEPRECATED void Function() ABEL_POSTFIX_DEPRECATED;
//

#ifndef ABEL_DEPRECATED
#if defined(ABEL_COMPILER_CPP14_ENABLED)
#define ABEL_DEPRECATED [[deprecated]]
#elif defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION > 1300) // If VC7 (VS2003) or later...
#define ABEL_DEPRECATED __declspec(deprecated)
#elif defined(ABEL_COMPILER_MSVC)
#define ABEL_DEPRECATED
#else
#define ABEL_DEPRECATED __attribute__((deprecated))
#endif
#endif

#ifndef ABEL_PREFIX_DEPRECATED
#if defined(ABEL_COMPILER_CPP14_ENABLED)
#define ABEL_PREFIX_DEPRECATED [[deprecated]]
#define ABEL_POSTFIX_DEPRECATED
#elif defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION > 1300) // If VC7 (VS2003) or later...
#define ABEL_PREFIX_DEPRECATED __declspec(deprecated)
#define ABEL_POSTFIX_DEPRECATED
#elif defined(ABEL_COMPILER_MSVC)
#define ABEL_PREFIX_DEPRECATED
#define ABEL_POSTFIX_DEPRECATED
#else
#define ABEL_PREFIX_DEPRECATED
#define ABEL_POSTFIX_DEPRECATED __attribute__((deprecated))
#endif
#endif

#ifndef ABEL_DEPRECATED_MESSAGE
#if defined(ABEL_COMPILER_CPP14_ENABLED)
#define ABEL_DEPRECATED_MESSAGE(msg) [[deprecated(#msg)]]
#else
// Compiler does not support depreaction messages, explicitly drop the msg but still mark the function as deprecated
#define ABEL_DEPRECATED_MESSAGE(msg) ABEL_DEPRECATED
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_FORCE_INLINE              // Used as a prefix.
// ABEL_PREFIX_FORCE_INLINE       // You should need this only for unusual compilers.
// ABEL_POSTFIX_FORCE_INLINE      // You should need this only for unusual compilers.
//
// Example usage:
//     ABEL_FORCE_INLINE void Foo();                                // Implementation elsewhere.
//     ABEL_PREFIX_FORCE_INLINE void Foo() ABEL_POSTFIX_FORCE_INLINE; // Implementation elsewhere.
//
// Note that when the prefix version of this function is used, it replaces
// the regular C++ 'inline' statement. Thus you should not use both the
// C++ inline statement and this macro with the same function declaration.
//
// To force inline usage under GCC 3.1+, you use this:
//    inline void Foo() __attribute__((always_inline));
//       or
//    inline __attribute__((always_inline)) void Foo();
//
// The CodeWarrior compiler doesn't have the concept of forcing inlining per function.
//
#ifndef ABEL_FORCE_INLINE
#if defined(ABEL_COMPILER_MSVC)
#define ABEL_FORCE_INLINE __forceinline
#elif defined(ABEL_COMPILER_GNUC) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 301) || defined(ABEL_COMPILER_CLANG)
#if defined(__cplusplus)
#define ABEL_FORCE_INLINE inline __attribute__((always_inline))
#else
#define ABEL_FORCE_INLINE __inline__ __attribute__((always_inline))
#endif
#else
#if defined(__cplusplus)
#define ABEL_FORCE_INLINE inline
#else
#define ABEL_FORCE_INLINE __inline
#endif
#endif
#endif

#if defined(ABEL_COMPILER_GNUC) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 301) || defined(ABEL_COMPILER_CLANG)
#define ABEL_PREFIX_FORCE_INLINE  inline
#define ABEL_POSTFIX_FORCE_INLINE __attribute__((always_inline))
#else
#define ABEL_PREFIX_FORCE_INLINE  inline
#define ABEL_POSTFIX_FORCE_INLINE
#endif


// ------------------------------------------------------------------------
// ABEL_FORCE_INLINE_LAMBDA
//
// ABEL_FORCE_INLINE_LAMBDA is used to force inline a call to a lambda when possible.
// Force inlining a lambda can be useful to reduce overhead in situations where a lambda may
// may only be called once, or inlining allows the compiler to apply other optimizations that wouldn't
// otherwise be possible.
//
// The ability to force inline a lambda is currently only available on a subset of compilers.
//
// Example usage:
//
//		auto lambdaFunction = []() ABEL_FORCE_INLINE_LAMBDA
//		{
//		};
//
#ifndef ABEL_FORCE_INLINE_LAMBDA
#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)
#define ABEL_FORCE_INLINE_LAMBDA __attribute__((always_inline))
#else
#define ABEL_FORCE_INLINE_LAMBDA
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_NO_INLINE             // Used as a prefix.
// ABEL_PREFIX_NO_INLINE      // You should need this only for unusual compilers.
// ABEL_POSTFIX_NO_INLINE     // You should need this only for unusual compilers.
//
// Example usage:
//     ABEL_NO_INLINE        void Foo();                       // Implementation elsewhere.
//     ABEL_PREFIX_NO_INLINE void Foo() ABEL_POSTFIX_NO_INLINE;  // Implementation elsewhere.
//
// That this declaration is incompatbile with C++ 'inline' and any
// variant of ABEL_FORCE_INLINE.
//
// To disable inline usage under VC++ priof to VS2005, you need to use this:
//    #pragma inline_depth(0) // Disable inlining.
//    void Foo() { ... }
//    #pragma inline_depth()  // Restore to default.
//
// Since there is no easy way to disable inlining on a function-by-function
// basis in VC++ prior to VS2005, the best strategy is to write platform-specific
// #ifdefs in the code or to disable inlining for a given module and enable
// functions individually with ABEL_FORCE_INLINE.
//
#ifndef ABEL_NO_INLINE
#if defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
#define ABEL_NO_INLINE __declspec(noinline)
#elif defined(ABEL_COMPILER_MSVC)
#define ABEL_NO_INLINE
#else
#define ABEL_NO_INLINE __attribute__((noinline))
#endif
#endif

#if defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
#define ABEL_PREFIX_NO_INLINE  __declspec(noinline)
#define ABEL_POSTFIX_NO_INLINE
#elif defined(ABEL_COMPILER_MSVC)
#define ABEL_PREFIX_NO_INLINE
#define ABEL_POSTFIX_NO_INLINE
#else
#define ABEL_PREFIX_NO_INLINE
#define ABEL_POSTFIX_NO_INLINE __attribute__((noinline))
#endif


// ------------------------------------------------------------------------
// ABEL_NO_VTABLE
//
// Example usage:
//     class ABEL_NO_VTABLE X {
//        virtual void InterfaceFunction();
//     };
//
//     ABEL_CLASS_NO_VTABLE(X) {
//        virtual void InterfaceFunction();
//     };
//
#ifdef ABEL_COMPILER_MSVC
#define ABEL_NO_VTABLE           __declspec(novtable)
#define ABEL_CLASS_NO_VTABLE(x)  class __declspec(novtable) x
#define ABEL_STRUCT_NO_VTABLE(x) struct __declspec(novtable) x
#else
#define ABEL_NO_VTABLE
#define ABEL_CLASS_NO_VTABLE(x)  class x
#define ABEL_STRUCT_NO_VTABLE(x) struct x
#endif


// ------------------------------------------------------------------------
// ABEL_PASCAL
//
// Also known on PC platforms as stdcall.
// This convention causes the compiler to assume that the called function
// will pop off the stack space used to pass arguments, unless it takes a
// variable number of arguments.
//
// Example usage:
//    this:
//       void DoNothing(int x);
//       void DoNothing(int x){}
//    would be written as this:
//       void ABEL_PASCAL_FUNC(DoNothing(int x));
//       void ABEL_PASCAL_FUNC(DoNothing(int x)){}
//
#ifndef ABEL_PASCAL
#if defined(ABEL_COMPILER_MSVC)
#define ABEL_PASCAL __stdcall
#elif defined(ABEL_COMPILER_GNUC) && defined(ABEL_PROCESSOR_X86)
#define ABEL_PASCAL __attribute__((stdcall))
#else
// Some compilers simply don't support pascal calling convention.
// As a result, there isn't an issue here, since the specification of
// pascal calling convention is for the purpose of disambiguating the
// calling convention that is applied.
#define ABEL_PASCAL
#endif
#endif

#ifndef ABEL_PASCAL_FUNC
#if defined(ABEL_COMPILER_MSVC)
#define ABEL_PASCAL_FUNC(funcname_and_paramlist)    __stdcall funcname_and_paramlist
#elif defined(ABEL_COMPILER_GNUC) && defined(ABEL_PROCESSOR_X86)
#define ABEL_PASCAL_FUNC(funcname_and_paramlist)    __attribute__((stdcall)) funcname_and_paramlist
#else
#define ABEL_PASCAL_FUNC(funcname_and_paramlist)    funcname_and_paramlist
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_SSE
// Visual C Processor Packs define _MSC_FULL_VER and are needed for SSE
// Intel C also has SSE support.
// ABEL_SSE is used to select FPU or SSE versions in hw_select.inl
//
// ABEL_SSE defines the level of SSE support:
//  0 indicates no SSE support
//  1 indicates SSE1 is supported
//  2 indicates SSE2 is supported
//  3 indicates SSE3 (or greater) is supported
//
// Note: SSE support beyond SSE3 can't be properly represented as a single
// version number.  Instead users should use specific SSE defines (e.g.
// ABEL_SSE4_2) to detect what specific support is available.  ABEL_SSE being
// equal to 3 really only indicates that SSE3 or greater is supported.
#ifndef ABEL_SSE
#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)
#if defined(__SSE3__)
#define ABEL_SSE 3
#elif defined(__SSE2__)
#define ABEL_SSE 2
#elif defined(__SSE__) && __SSE__
#define ABEL_SSE 1
#else
#define ABEL_SSE 0
#endif
#elif (defined(ABEL_SSE3) && ABEL_SSE3) || defined ABEL_PLATFORM_XBOXONE
#define ABEL_SSE 3
#elif defined(ABEL_SSE2) && ABEL_SSE2
#define ABEL_SSE 2
#elif defined(ABEL_PROCESSOR_X86) && defined(_MSC_FULL_VER) && !defined(__NOSSE__) && defined(_M_IX86_FP)
#define ABEL_SSE _M_IX86_FP
#elif defined(ABEL_PROCESSOR_X86) && defined(ABEL_COMPILER_INTEL) && !defined(__NOSSE__)
#define ABEL_SSE 1
#elif defined(ABEL_PROCESSOR_X86_64)
// All x64 processors support SSE2 or higher
#define ABEL_SSE 2
#else
#define ABEL_SSE 0
#endif
#endif

// ------------------------------------------------------------------------
// We define separate defines for SSE support beyond SSE1.  These defines
// are particularly useful for detecting SSE4.x features since there isn't
// a single concept of SSE4.
//
// The following SSE defines are always defined.  0 indicates the
// feature/level of SSE is not supported, and 1 indicates support is
// available.
#ifndef ABEL_SSE2
#if ABEL_SSE >= 2
#define ABEL_SSE2 1
#else
#define ABEL_SSE2 0
#endif
#endif
#ifndef ABEL_SSE3
#if ABEL_SSE >= 3
#define ABEL_SSE3 1
#else
#define ABEL_SSE3 0
#endif
#endif
#ifndef ABEL_SSSE3
#if defined __SSSE3__ || defined ABEL_PLATFORM_XBOXONE
#define ABEL_SSSE3 1
#else
#define ABEL_SSSE3 0
#endif
#endif
#ifndef ABEL_SSE4_1
#if defined __SSE4_1__ || defined ABEL_PLATFORM_XBOXONE
#define ABEL_SSE4_1 1
#else
#define ABEL_SSE4_1 0
#endif
#endif
#ifndef ABEL_SSE4_2
#if defined __SSE4_2__ || defined ABEL_PLATFORM_XBOXONE
#define ABEL_SSE4_2 1
#else
#define ABEL_SSE4_2 0
#endif
#endif
#ifndef ABEL_SSE4A
#if defined __SSE4A__ || defined ABEL_PLATFORM_XBOXONE
#define ABEL_SSE4A 1
#else
#define ABEL_SSE4A 0
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_AVX
// ABEL_AVX may be used to determine if Advanced Vector Extensions are available for the target architecture
//
// ABEL_AVX defines the level of AVX support:
//  0 indicates no AVX support
//  1 indicates AVX1 is supported
//  2 indicates AVX2 is supported
#ifndef ABEL_AVX
#if defined __AVX2__
#define ABEL_AVX 2
#elif defined __AVX__ || defined ABEL_PLATFORM_XBOXONE
#define ABEL_AVX 1
#else
#define ABEL_AVX 0
#endif
#endif
#ifndef ABEL_AVX2
#if ABEL_AVX >= 2
#define ABEL_AVX2 1
#else
#define ABEL_AVX2 0
#endif
#endif

// ABEL_FP16C may be used to determine the existence of float <-> half conversion operations on an x86 CPU.
// (For example to determine if _mm_cvtph_ps or _mm_cvtps_ph could be used.)
#ifndef ABEL_FP16C
#if defined __F16C__ || defined ABEL_PLATFORM_XBOXONE
#define ABEL_FP16C 1
#else
#define ABEL_FP16C 0
#endif
#endif

// ABEL_FP128 may be used to determine if __float128 is a supported type for use. This type is enabled by a GCC extension (_GLIBCXX_USE_FLOAT128)
// but has support by some implementations of clang (__FLOAT128__)
// PS4 does not support __float128 as of SDK 5.500 https://ps4.siedev.net/resources/documents/SDK/5.500/CPU_Compiler_ABI-Overview/0003.html
#ifndef ABEL_FP128
#if (defined __FLOAT128__ || defined _GLIBCXX_USE_FLOAT128) && !defined(ABEL_PLATFORM_SONY)
#define ABEL_FP128 1
#else
#define ABEL_FP128 0
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_ABM
// ABEL_ABM may be used to determine if Advanced Bit Manipulation sets are available for the target architecture (POPCNT, LZCNT)
//
#ifndef ABEL_ABM
#if defined(__ABM__) || defined(ABEL_PLATFORM_XBOXONE) || defined(ABEL_PLATFORM_SONY)
#define ABEL_ABM 1
#else
#define ABEL_ABM 0
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_NEON
// ABEL_NEON may be used to determine if NEON is supported.
#ifndef ABEL_NEON
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#define ABEL_NEON 1
#else
#define ABEL_NEON 0
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_BMI
// ABEL_BMI may be used to determine if Bit Manipulation Instruction sets are available for the target architecture
//
// ABEL_BMI defines the level of BMI support:
//  0 indicates no BMI support
//  1 indicates BMI1 is supported
//  2 indicates BMI2 is supported
#ifndef ABEL_BMI
#if defined(__BMI2__)
#define ABEL_BMI 2
#elif defined(__BMI__) || defined(ABEL_PLATFORM_XBOXONE)
#define ABEL_BMI 1
#else
#define ABEL_BMI 0
#endif
#endif
#ifndef ABEL_BMI2
#if ABEL_BMI >= 2
#define ABEL_BMI2 1
#else
#define ABEL_BMI2 0
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_FMA3
// ABEL_FMA3 may be used to determine if Fused Multiply Add operations are available for the target architecture
// __FMA__ is defined only by GCC, Clang, and ICC; MSVC only defines __AVX__ and __AVX2__
// FMA3 was introduced alongside AVX2 on Intel Haswell
// All AMD processors support FMA3 if AVX2 is also supported
//
// ABEL_FMA3 defines the level of FMA3 support:
//  0 indicates no FMA3 support
//  1 indicates FMA3 is supported
#ifndef ABEL_FMA3
#if defined(__FMA__) || ABEL_AVX2 >= 1
#define ABEL_FMA3 1
#else
#define ABEL_FMA3 0
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_TBM
// ABEL_TBM may be used to determine if Trailing Bit Manipulation instructions are available for the target architecture
#ifndef ABEL_TBM
#if defined(__TBM__)
#define ABEL_TBM 1
#else
#define ABEL_TBM 0
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_IMPORT
// import declaration specification
// specifies that the declared symbol is imported from another dynamic library.
#ifndef ABEL_IMPORT
#if defined(ABEL_COMPILER_MSVC)
#define ABEL_IMPORT __declspec(dllimport)
#else
#define ABEL_IMPORT
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_EXPORT
// export declaration specification
// specifies that the declared symbol is exported from the current dynamic library.
// this is not the same as the C++ export keyword. The C++ export keyword has been
// removed from the language as of C++11.
#ifndef ABEL_EXPORT
#if defined(ABEL_COMPILER_MSVC)
#define ABEL_EXPORT __declspec(dllexport)
#else
#define ABEL_EXPORT
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_OVERRIDE
//
// C++11 override
// See http://msdn.microsoft.com/en-us/library/jj678987.aspx for more information.
// You can use ABEL_FINAL_OVERRIDE to combine usage of ABEL_OVERRIDE and ABEL_INHERITANCE_FINAL in a single statement.
//
// Example usage:
//        struct B     { virtual void f(int); };
//        struct D : B { void f(int) ABEL_OVERRIDE; };
//
#ifndef ABEL_OVERRIDE
#if defined(ABEL_COMPILER_NO_OVERRIDE)
#define ABEL_OVERRIDE
#else
#define ABEL_OVERRIDE override
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_INHERITANCE_FINAL
//
// Portably wraps the C++11 final specifier.
// See http://msdn.microsoft.com/en-us/library/jj678985.aspx for more information.
// You can use ABEL_FINAL_OVERRIDE to combine usage of ABEL_OVERRIDE and ABEL_INHERITANCE_FINAL in a single statement.
// This is not called ABEL_FINAL because that term is used within CB to denote debug/release/final builds.
//
// Example usage:
//     struct B { virtual void f() ABEL_INHERITANCE_FINAL; };
//
#ifndef ABEL_INHERITANCE_FINAL
#if defined(ABEL_COMPILER_NO_INHERITANCE_FINAL)
#define ABEL_INHERITANCE_FINAL
#elif (defined(_MSC_VER) && (ABEL_COMPILER_VERSION < 1700))  // Pre-VS2012
#define ABEL_INHERITANCE_FINAL sealed
#else
#define ABEL_INHERITANCE_FINAL final
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_FINAL_OVERRIDE
//
// Portably wraps the C++11 override final specifiers combined.
//
// Example usage:
//     struct A            { virtual void f(); };
//     struct B : public A { virtual void f() ABEL_FINAL_OVERRIDE; };
//
#ifndef ABEL_FINAL_OVERRIDE
#define ABEL_FINAL_OVERRIDE ABEL_OVERRIDE ABEL_INHERITANCE_FINAL
#endif


// ------------------------------------------------------------------------
// ABEL_SEALED
//
// This is deprecated, as the C++11 Standard has final (ABEL_INHERITANCE_FINAL) instead.
// See http://msdn.microsoft.com/en-us/library/0w2w91tf.aspx for more information.
// Example usage:
//     struct B { virtual void f() ABEL_SEALED; };
//
#ifndef ABEL_SEALED
#if defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1400) // VS2005 (VC8) and later
#define ABEL_SEALED sealed
#else
#define ABEL_SEALED
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_ABSTRACT
//
// This is a Microsoft language extension.
// See http://msdn.microsoft.com/en-us/library/b0z6b513.aspx for more information.
// Example usage:
//     struct X ABEL_ABSTRACT { virtual void f(){} };
//
#ifndef ABEL_ABSTRACT
#if defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1400) // VS2005 (VC8) and later
#define ABEL_ABSTRACT abstract
#else
#define ABEL_ABSTRACT
#endif
#endif

#ifndef ABEL_EXPLICIT
#if defined(ABEL_COMPILER_NO_EXPLICIT_CONVERSION_OPERATORS) && ABEL_COMPILER_NO_EXPLICIT_CONVERSION_OPERATORS == 1
#define ABEL_EXPLICIT
#else
#define ABEL_EXPLICIT explicit
#endif
#endif

// ------------------------------------------------------------------------
// ABEL_CONSTEXPR
// ABEL_CONSTEXPR_OR_CONST
//
// Portable wrapper for C++11's 'constexpr' support.
//
// See http://www.cprogramming.com/c++11/c++11-compile-time-processing-with-constexpr.html for more information.
// Example usage:
//     ABEL_CONSTEXPR int GetValue() { return 37; }
//     ABEL_CONSTEXPR_OR_CONST double gValue = std::sin(kTwoPi);
//

#ifndef ABEL_CONSTEXPR
#if ABEL_COMPILER_CPP11_ENABLED
#define ABEL_CONSTEXPR constexpr
#else
#define ABEL_CONSTEXPR
#endif
#endif


#ifndef ABEL_CONSTEXPR_MEMBER
#if defined(ABEL_COMPILER_CPP14_ENABLED)
#define ABEL_CONSTEXPR_MEMBER constexpr
#else
#define ABEL_CONSTEXPR_MEMBER
#endif
#endif

#ifndef ABEL_CONSTEXPR_VARIABLE
#if defined(ABEL_COMPILER_CPP14_ENABLED)
#define ABEL_CONSTEXPR_VARIABLE constexpr
#else
#define ABEL_CONSTEXPR_VARIABLE const
#endif
#endif

#ifndef ABEL_CONSTEXPR_FUNCTION
#if defined(ABEL_COMPILER_CPP14_ENABLED)
#define ABEL_CONSTEXPR_FUNCTION constexpr
#else
#define ABEL_CONSTEXPR_FUNCTION inline
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_CONSTEXPR_IF
//
// Portable wrapper for C++17's 'constexpr if' support.
//
// https://en.cppreference.com/w/cpp/language/if
//
// Example usage:
//
// ABEL_CONSTEXPR_IF(eastl::is_copy_constructible_v<T>)
// 	{ ... }
//
#if !defined(ABEL_CONSTEXPR_IF)
#if defined(ABEL_COMPILER_NO_CONSTEXPR_IF)
#define ABEL_CONSTEXPR_IF(predicate) if ((predicate))
#else
#define ABEL_CONSTEXPR_IF(predicate) if constexpr ((predicate))
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_EXTERN_TEMPLATE
//
// Portable wrapper for C++11's 'extern template' support.
//
// Example usage:
//     ABEL_EXTERN_TEMPLATE(class basic_string<char>);
//
#if !defined(ABEL_EXTERN_TEMPLATE)
#if defined(ABEL_COMPILER_NO_EXTERN_TEMPLATE)
#define ABEL_EXTERN_TEMPLATE(declaration)
#else
#define ABEL_EXTERN_TEMPLATE(declaration) extern template declaration
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_NOEXCEPT
// ABEL_NOEXCEPT_IF(predicate)
// ABEL_NOEXCEPT_EXPR(expression)
//
// Portable wrapper for C++11 noexcept
// http://en.cppreference.com/w/cpp/language/noexcept
// http://en.cppreference.com/w/cpp/language/noexcept_spec
//
// Example usage:
//     ABEL_NOEXCEPT
//     ABEL_NOEXCEPT_IF(predicate)
//     ABEL_NOEXCEPT_EXPR(expression)
//
//     This function never throws an exception.
//     void DoNothing() ABEL_NOEXCEPT
//         { }
//
//     This function throws an exception of T::T() throws an exception.
//     template <class T>
//     void DoNothing() ABEL_NOEXCEPT_IF(ABEL_NOEXCEPT_EXPR(T()))
//         { T t; }
//
#if !defined(ABEL_NOEXCEPT)
#if defined(ABEL_COMPILER_NO_NOEXCEPT)
#define ABEL_NOEXCEPT
#define ABEL_NOEXCEPT_IF(predicate)
#define ABEL_NOEXCEPT_EXPR(expression) false
#else
#define ABEL_NOEXCEPT noexcept
#define ABEL_NOEXCEPT_IF(predicate) noexcept((predicate))
#define ABEL_NOEXCEPT_EXPR(expression) noexcept((expression))
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_NORETURN
//
// Wraps the C++11 noreturn attribute. See ABEL_COMPILER_NO_NORETURN
// http://en.cppreference.com/w/cpp/language/attributes
// http://msdn.microsoft.com/en-us/library/k6ktzx3s%28v=vs.80%29.aspx
// http://blog.aaronballman.com/2011/09/understanding-attributes/
//
// Example usage:
//     ABEL_NORETURN void SomeFunction()
//         { throw "error"; }
//
#if !defined(ABEL_NORETURN)
#if defined(ABEL_COMPILER_MSVC) && (ABEL_COMPILER_VERSION >= 1300) // VS2003 (VC7) and later
#define ABEL_NORETURN __declspec(noreturn)
#elif defined(ABEL_COMPILER_NO_NORETURN)
#define ABEL_NORETURN
#else
#define ABEL_NORETURN [[noreturn]]
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_CARRIES_DEPENDENCY
//
// Wraps the C++11 carries_dependency attribute
// http://en.cppreference.com/w/cpp/language/attributes
// http://blog.aaronballman.com/2011/09/understanding-attributes/
//
// Example usage:
//     ABEL_CARRIES_DEPENDENCY int* SomeFunction()
//         { return &mX; }
//
//
#if !defined(ABEL_CARRIES_DEPENDENCY)
#if defined(ABEL_COMPILER_NO_CARRIES_DEPENDENCY)
#define ABEL_CARRIES_DEPENDENCY
#else
#define ABEL_CARRIES_DEPENDENCY [[carries_dependency]]
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_FALLTHROUGH
//
// [[fallthrough] is a C++17 standard attribute that appears in switch
// statements to indicate that the fallthrough from the previous case in the
// switch statement is intentially and not a bug.
//
// http://en.cppreference.com/w/cpp/language/attributes
//
// Example usage:
// 		void f(int n)
// 		{
// 			switch(n)
// 			{
// 				case 1:
// 				DoCase1();
// 				// Compiler may generate a warning for fallthrough behaviour
//
// 				case 2:
// 				DoCase2();
//
// 				ABEL_FALLTHROUGH;
// 				case 3:
// 				DoCase3();
// 			}
// 		}
//
#if !defined(ABEL_FALLTHROUGH)
#if defined(ABEL_COMPILER_NO_FALLTHROUGH)
#define ABEL_FALLTHROUGH
#else
#define ABEL_FALLTHROUGH [[fallthrough]]
#endif
#endif



// ------------------------------------------------------------------------
// ABEL_NODISCARD
//
// [[nodiscard]] is a C++17 standard attribute that can be applied to a
// function declaration, enum, or class declaration.  If a any of the list
// previously are returned from a function (without the user explicitly
// casting to void) the addition of the [[nodiscard]] attribute encourages
// the compiler to generate a warning about the user discarding the return
// value. This is a useful practice to encourage client code to check API
// error codes.
//
// http://en.cppreference.com/w/cpp/language/attributes
//
// Example usage:
//
//     ABEL_NODISCARD int baz() { return 42; }
//
//     void foo()
//     {
//         baz(); // warning: ignoring return value of function declared with 'nodiscard' attribute
//     }
//
#if !defined(ABEL_NODISCARD)
#if defined(ABEL_COMPILER_NO_NODISCARD)
#define ABEL_NODISCARD
#else
#define ABEL_NODISCARD [[nodiscard]]
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_MAYBE_UNUSED
//
// [[maybe_unused]] is a C++17 standard attribute that suppresses warnings
// on unused entities that are declared as maybe_unused.
//
// http://en.cppreference.com/w/cpp/language/attributes
//
// Example usage:
//    void foo(ABEL_MAYBE_UNUSED int i)
//    {
//        assert(i == 42);  // warning suppressed when asserts disabled.
//    }
//
#if !defined(ABEL_MAYBE_UNUSED)
#if defined(ABEL_COMPILER_NO_MAYBE_UNUSED)
#define ABEL_MAYBE_UNUSED
#else
#define ABEL_MAYBE_UNUSED [[maybe_unused]]
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_NO_UBSAN
//
// The LLVM/Clang undefined behaviour sanitizer will not analyse a function tagged with the following attribute.
//
// https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#disabling-instrumentation-with-attribute-no-sanitize-undefined
//
// Example usage:
//     ABEL_NO_UBSAN int SomeFunction() { ... }
//
#ifndef ABEL_NO_UBSAN
#if defined(ABEL_COMPILER_CLANG)
#define ABEL_NO_UBSAN __attribute__((no_sanitize("undefined")))
#else
#define ABEL_NO_UBSAN
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_NO_ASAN
//
// The LLVM/Clang address sanitizer will not analyse a function tagged with the following attribute.
//
// https://clang.llvm.org/docs/AddressSanitizer.html#disabling-instrumentation-with-attribute-no-sanitize-address
//
// Example usage:
//     ABEL_NO_ASAN int SomeFunction() { ... }
//
#ifndef ABEL_NO_ASAN
#if defined(ABEL_COMPILER_CLANG)
#define ABEL_NO_ASAN __attribute__((no_sanitize("address")))
#else
#define ABEL_NO_ASAN
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_ASAN_ENABLED
//
// Defined as 0 or 1. It's value depends on the compile environment.
// Specifies whether the code is being built with Clang's Address Sanitizer.
//
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ABEL_ASAN_ENABLED 1
#else
#define ABEL_ASAN_ENABLED 0
#endif
#else
#define ABEL_ASAN_ENABLED 0
#endif


// ------------------------------------------------------------------------
// ABEL_NON_COPYABLE
//
// This macro defines as a class as not being copy-constructable
// or assignable. This is useful for preventing class instances
// from being passed to functions by value, is useful for preventing
// compiler warnings by some compilers about the inability to
// auto-generate a copy constructor and assignment, and is useful
// for simply declaring in the interface that copy semantics are
// not supported by the class. Your class needs to have at least a
// default constructor when using this macro.
//
// Beware that this class works by declaring a private: section of
// the class in the case of compilers that don't support C++11 deleted
// functions.
//
// Note: With some pre-C++11 compilers (e.g. Green Hills), you may need
//       to manually define an instances of the hidden functions, even
//       though they are not used.
//
// Example usage:
//    class Widget {
//       Widget();
//       . . .
//       ABEL_NON_COPYABLE(Widget)
//    };
//
#if !defined(ABEL_NON_COPYABLE)
#if defined(ABEL_COMPILER_NO_DELETED_FUNCTIONS)
#define ABEL_NON_COPYABLE(CBClass_)               \
              private:                                      \
                ABEL_DISABLE_VC_WARNING(4822);	/* local class member function does not have a body	*/		\
                CBClass_(const CBClass_&);                  \
                void operator=(const CBClass_&);			\
                ABEL_RESTORE_VC_WARNING()
#else
#define ABEL_NON_COPYABLE(CBClass_)               \
                ABEL_DISABLE_VC_WARNING(4822);    /* local class member function does not have a body	*/        \
                CBClass_(const CBClass_&) = delete;         \
                void operator=(const CBClass_&) = delete;    \
                ABEL_RESTORE_VC_WARNING()
#endif
#endif


// ------------------------------------------------------------------------
// ABEL_FUNCTION_DELETE
//
// Semi-portable way of specifying a deleted function which allows for
// cleaner code in class declarations.
//
// Example usage:
//
//  class Example
//  {
//  private: // For portability with pre-C++11 compilers, make the function private.
//      void foo() ABEL_FUNCTION_DELETE;
//  };
//
// Note: ABEL_FUNCTION_DELETE'd functions should be private to prevent the
// functions from being called even when the compiler does not support
// deleted functions. Some compilers (e.g. Green Hills) that don't support
// C++11 deleted functions can require that you define the function,
// which you can do in the associated source file for the class.
//
#if defined(ABEL_COMPILER_NO_DELETED_FUNCTIONS)
#define ABEL_FUNCTION_DELETE
#else
#define ABEL_FUNCTION_DELETE = delete
#endif

// ------------------------------------------------------------------------
// ABEL_DISABLE_DEFAULT_CTOR
//
// Disables the compiler generated default constructor. This macro is
// provided to improve portability and clarify intent of code.
//
// Example usage:
//
//  class Example
//  {
//  private:
//      ABEL_DISABLE_DEFAULT_CTOR(Example);
//  };
//
#define ABEL_DISABLE_DEFAULT_CTOR(ClassName) ClassName() ABEL_FUNCTION_DELETE

// ------------------------------------------------------------------------
// ABEL_DISABLE_COPY_CTOR
//
// Disables the compiler generated copy constructor. This macro is
// provided to improve portability and clarify intent of code.
//
// Example usage:
//
//  class Example
//  {
//  private:
//      ABEL_DISABLE_COPY_CTOR(Example);
//  };
//
#define ABEL_DISABLE_COPY_CTOR(ClassName) ClassName(const ClassName &) ABEL_FUNCTION_DELETE

// ------------------------------------------------------------------------
// ABEL_DISABLE_MOVE_CTOR
//
// Disables the compiler generated move constructor. This macro is
// provided to improve portability and clarify intent of code.
//
// Example usage:
//
//  class Example
//  {
//  private:
//      ABEL_DISABLE_MOVE_CTOR(Example);
//  };
//
#define ABEL_DISABLE_MOVE_CTOR(ClassName) ClassName(ClassName&&) ABEL_FUNCTION_DELETE

// ------------------------------------------------------------------------
// ABEL_DISABLE_ASSIGNMENT_OPERATOR
//
// Disables the compiler generated assignment operator. This macro is
// provided to improve portability and clarify intent of code.
//
// Example usage:
//
//  class Example
//  {
//  private:
//      ABEL_DISABLE_ASSIGNMENT_OPERATOR(Example);
//  };
//
#define ABEL_DISABLE_ASSIGNMENT_OPERATOR(ClassName) ClassName & operator=(const ClassName &) ABEL_FUNCTION_DELETE

// ------------------------------------------------------------------------
// ABEL_DISABLE_MOVE_OPERATOR
//
// Disables the compiler generated move operator. This macro is
// provided to improve portability and clarify intent of code.
//
// Example usage:
//
//  class Example
//  {
//  private:
//      ABEL_DISABLE_MOVE_OPERATOR(Example);
//  };
//
#define ABEL_DISABLE_MOVE_OPERATOR(ClassName) ClassName & operator=(ClassName&&) ABEL_FUNCTION_DELETE

#define ABEL_DISABLE_IMPLICIT_CTOR(ClassName) \
    ABEL_NON_COPYABLE(ClassName); \
    ABEL_DISABLE_DEFAULT_CTOR(ClassName)

// ------------------------------------------------------------------------
// CBNonCopyable
//
// Declares a class as not supporting copy construction or assignment.
// May be more reliable with some situations that ABEL_NON_COPYABLE alone,
// though it may result in more code generation.
//
// Note that VC++ will generate warning C4625 and C4626 if you use CBNonCopyable
// and you are compiling with /W4 and /Wall. There is no resolution but
// to redelare ABEL_NON_COPYABLE in your subclass or disable the warnings with
// code like this:
//     ABEL_DISABLE_VC_WARNING(4625 4626)
//     ...
//     ABEL_RESTORE_VC_WARNING()
//
// Example usage:
//     struct Widget : CBNonCopyable {
//        . . .
//     };
//
#ifdef __cplusplus

struct CBNonCopyable {
#if defined(ABEL_COMPILER_NO_DEFAULTED_FUNCTIONS) || defined(__EDG__) // EDG doesn't appear to behave properly for the case of defaulted constructors; it generates a mistaken warning about missing default constructors.
    CBNonCopyable(){} // Putting {} here has the downside that it allows a class to create itself,
   ~CBNonCopyable(){} // but avoids linker errors that can occur with some compilers (e.g. Green Hills).
#else

    CBNonCopyable() = default;

    ~CBNonCopyable() = default;

#endif
    ABEL_NON_COPYABLE(CBNonCopyable)
};

#endif


// ------------------------------------------------------------------------
// ABEL_OPTIMIZE_OFF / ABEL_OPTIMIZE_ON
//
// Implements portable inline optimization enabling/disabling.
// Usage of these macros must be in order OFF then ON. This is
// because the OFF macro pushes a set of settings and the ON
// macro pops them. The nesting of OFF/ON sets (e.g. OFF, OFF, ON, ON)
// is not guaranteed to work on all platforms.
//
// This is often used to allow debugging of some code that's
// otherwise compiled with undebuggable optimizations. It's also
// useful for working around compiler code generation problems
// that occur in optimized builds.
//
// Some compilers (e.g. VC++) don't allow doing this within a function and
// so the usage must be outside a function, as with the example below.
// GCC on x86 appears to have some problem with argument passing when
// using ABEL_OPTIMIZE_OFF in optimized builds.
//
// Example usage:
//     // Disable optimizations for SomeFunction.
//     ABEL_OPTIMIZE_OFF()
//     void SomeFunction()
//     {
//         ...
//     }
//     ABEL_OPTIMIZE_ON()
//
#if !defined(ABEL_OPTIMIZE_OFF)
#if   defined(ABEL_COMPILER_MSVC)
#define ABEL_OPTIMIZE_OFF() __pragma(optimize("", off))
#elif defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION > 4004) && (defined(__i386__) || defined(__x86_64__)) // GCC 4.4+ - Seems to work only on x86/Linux so far. However, GCC 4.4 itself appears broken and screws up parameter passing conventions.
#define ABEL_OPTIMIZE_OFF()            \
                _Pragma("GCC push_options")      \
                _Pragma("GCC optimize 0")
#elif defined(ABEL_COMPILER_CLANG) && (!defined(ABEL_PLATFORM_ANDROID) || (ABEL_COMPILER_VERSION >= 380))
#define ABEL_OPTIMIZE_OFF() \
                ABEL_DISABLE_CLANG_WARNING(-Wunknown-pragmas) \
                _Pragma("clang optimize off") \
                ABEL_RESTORE_CLANG_WARNING()
#else
#define ABEL_OPTIMIZE_OFF()
#endif
#endif

#if !defined(ABEL_OPTIMIZE_ON)
#if   defined(ABEL_COMPILER_MSVC)
#define ABEL_OPTIMIZE_ON() __pragma(optimize("", on))
#elif defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION > 4004) && (defined(__i386__) || defined(__x86_64__)) // GCC 4.4+ - Seems to work only on x86/Linux so far. However, GCC 4.4 itself appears broken and screws up parameter passing conventions.
#define ABEL_OPTIMIZE_ON() _Pragma("GCC pop_options")
#elif defined(ABEL_COMPILER_CLANG) && (!defined(ABEL_PLATFORM_ANDROID) || (ABEL_COMPILER_VERSION >= 380))
#define ABEL_OPTIMIZE_ON() \
                ABEL_DISABLE_CLANG_WARNING(-Wunknown-pragmas) \
                _Pragma("clang optimize on") \
                ABEL_RESTORE_CLANG_WARNING()
#else
#define ABEL_OPTIMIZE_ON()
#endif
#endif



// ABEL_BLOCK_TAIL_CALL_OPTIMIZATION
//
// Instructs the compiler to avoid optimizing tail-call recursion. Use of this
// macro is useful when you wish to preserve the existing function order within
// a stack trace for logging, debugging, or profiling purposes.
//
// Example:
//
//   int f() {
//     int result = g();
//     ABEL_BLOCK_TAIL_CALL_OPTIMIZATION();
//     return result;
//   }
#if defined(__pnacl__)
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#elif defined(__clang__)
// Clang will not tail call given inline volatile assembly.
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(__GNUC__)
// GCC will not tail call given inline volatile assembly.
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(_MSC_VER)
#include <intrin.h>
// The __nop() intrinsic blocks the optimisation.
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() __nop()
#else
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#endif

#ifndef ABEL_WARN_UNUSED_RESULT
#if defined(ABEL_COMPILER_GNUC) && ABEL_COMPILER_VERSION >= 4007 && ABEL_COMPILER_CPP11_ENABLED
#define ABEL_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define ABEL_WARN_UNUSED_RESULT
#endif
#endif //WARN_UNUSED_RESULT

#ifndef ABEL_PRINTF_FORMAT
#if defined(ABEL_COMPILER_GNUC)
#define ABEL_PRINTF_FORMAT(format_param, dots_param) __attribute__((format(printf, format_param, dots_param)))
#else
#define ABEL_PRINTF_FORMAT(format_param, dots_param)
#endif
#endif //ABEL_PRINTF_FORMAT

#define ABEL_WPRINTF_FORMAT(format_param, dots_param)

#ifndef ABEL_CDECL
#if defined(ABEL_PLATFORM_WINDOWS)
#define ABEL_CDECL __cdecl
#else
#define ABEL_CDECL
#endif
#endif //ABEL_CDECL

#if defined(COMPILER_GCC)
#define ABEL_ALLOW_UNUSED __attribute__((unused))
#else
#define ABEL_ALLOW_UNUSED
#endif

#ifndef ABEL_ALLOW_UNUSED
#if defined(ABEL_COMPILER_GNUC)
#define ABEL_ALLOW_UNUSED __attribute__((unused))
#else
#define ABEL_ALLOW_UNUSED
#endif
#endif //ABEL_ALLOW_UNUSED

#ifndef ABEL_CACHE_LINE_ALIGNED
#define ABEL_CACHE_LINE_ALIGNED ABEL_ALIGN(ABEL_CACHE_LINE_SIZE)
#endif //ABEL_CACHE_LINE_ALIGNED

// ABEL_PRINTF
// ABEL_SCANF
//
// Tells the compiler to perform `printf` format string checking if the
// compiler supports it; see the 'format' attribute in
// <https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Function-Attributes.html>.
//
// Note: As the GCC manual states, "[s]ince non-static C++ methods
// have an implicit 'this' argument, the arguments of such methods
// should be counted from two, not one."
#if ABEL_COMPILER_HAS_ATTRIBUTE(format) || (defined(__GNUC__) && !defined(__clang__))
#define ABEL_PRINTF(string_index, first_to_check) \
  __attribute__((__format__(__printf__, string_index, first_to_check)))
#define ABEL_SCANF(string_index, first_to_check) \
  __attribute__((__format__(__scanf__, string_index, first_to_check)))
#else
#define ABEL_PRINTF(string_index, first_to_check)
#define ABEL_SCANF(string_index, first_to_check)
#endif


// ABEL_ATTRIBUTE_NONNULL
//
// Tells the compiler either (a) that a particular function parameter
// should be a non-null pointer, or (b) that all pointer arguments should
// be non-null.
//
// Note: As the GCC manual states, "[s]ince non-static C++ methods
// have an implicit 'this' argument, the arguments of such methods
// should be counted from two, not one."
//
// Args are indexed starting at 1.
//
// For non-static class member functions, the implicit `this` argument
// is arg 1, and the first explicit argument is arg 2. For static class member
// functions, there is no implicit `this`, and the first explicit argument is
// arg 1.
//
// Example:
//
//   /* arg_a cannot be null, but arg_b can */
//   void Function(void* arg_a, void* arg_b) ABEL_ATTRIBUTE_NONNULL(1);
//
//   class C {
//     /* arg_a cannot be null, but arg_b can */
//     void Method(void* arg_a, void* arg_b) ABEL_ATTRIBUTE_NONNULL(2);
//
//     /* arg_a cannot be null, but arg_b can */
//     static void StaticMethod(void* arg_a, void* arg_b)
//     ABEL_ATTRIBUTE_NONNULL(1);
//   };
//
// If no arguments are provided, then all pointer arguments should be non-null.
//
//  /* No pointer arguments may be null. */
//  void Function(void* arg_a, void* arg_b, int arg_c) ABEL_ATTRIBUTE_NONNULL();
//
// NOTE: The GCC nonnull attribute actually accepts a list of arguments, but
// ABEL_ATTRIBUTE_NONNULL does not.
#if ABEL_COMPILER_HAS_ATTRIBUTE(nonnull) || (defined(__GNUC__) && !defined(__clang__))
#define ABEL_NONNULL(arg_index) __attribute__((nonnull(arg_index)))
#else
#define ABEL_NONNULL(...)
#endif

// ABEL_NO_SANITIZE_ADDRESS
//
// Tells the AddressSanitizer (or other memory testing tools) to ignore a given
// function. Useful for cases when a function reads random locations on stack,
// calls _exit from a cloned subprocess, deliberately accesses buffer
// out of bounds or does other scary things with memory.
// NOTE: GCC supports AddressSanitizer(asan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define ABEL_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define ABEL_NO_SANITIZE_ADDRESS
#endif

// ABEL_NO_SANITIZE_MEMORY
//
// Tells the  MemorySanitizer to relax the handling of a given function. All
// "Use of uninitialized value" warnings from such functions will be suppressed,
// and all values loaded from memory will be considered fully initialized.
// This attribute is similar to the ADDRESS_SANITIZER attribute above, but deals
// with initialized-ness rather than addressability issues.
// NOTE: MemorySanitizer(msan) is supported by Clang but not GCC.
#if defined(__clang__)
#define ABEL_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define ABEL_NO_SANITIZE_MEMORY
#endif

// ABEL_NO_SANITIZE_THREAD
//
// Tells the ThreadSanitizer to not instrument a given function.
// NOTE: GCC supports ThreadSanitizer(tsan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define ABEL_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define ABEL_NO_SANITIZE_THREAD
#endif

// ABEL_NO_SANITIZE_UNDEFINED
//
// Tells the UndefinedSanitizer to ignore a given function. Useful for cases
// where certain behavior (eg. division by zero) is being used intentionally.
// NOTE: GCC supports UndefinedBehaviorSanitizer(ubsan) since 4.9.
// https://gcc.gnu.org/gcc-4.9/changes.html
#if defined(__GNUC__) && \
    (defined(UNDEFINED_BEHAVIOR_SANITIZER) || defined(ADDRESS_SANITIZER))
#define ABEL_NO_SANITIZE_UNDEFINED \
  __attribute__((no_sanitize("undefined")))
#else
#define ABEL_NO_SANITIZE_UNDEFINED
#endif

// ABEL_NO_SANITIZE_CFI
//
// Tells the ControlFlowIntegrity sanitizer to not instrument a given function.
// See https://clang.llvm.org/docs/ControlFlowIntegrity.html for details.
#if defined(__GNUC__) && defined(CONTROL_FLOW_INTEGRITY)
#define ABEL_NO_SANITIZE_CFI __attribute__((no_sanitize("cfi")))
#else
#define ABEL_NO_SANITIZE_CFI
#endif


// ABEL_NO_SANITIZE_SAFESTACK
//
// Tells the SafeStack to not instrument a given function.
// See https://clang.llvm.org/docs/SafeStack.html for details.
#if defined(__GNUC__) && defined(SAFESTACK_SANITIZER)
#define ABEL_NO_SANITIZE_SAFESTACK \
  __attribute__((no_sanitize("safe-stack")))
#else
#define ABEL_NO_SANITIZE_SAFESTACK
#endif

// ABEL_RETURNS_NONNULL
//
// Tells the compiler that a particular function never returns a null pointer.
#if ABEL_COMPILER_HAS_ATTRIBUTE(returns_nonnull) || \
    (defined(__GNUC__) && \
     (__GNUC__ > 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)) && \
     !defined(__clang__))
#define ABEL_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define ABEL_RETURNS_NONNULL
#endif

// -----------------------------------------------------------------------------
// Variable Attributes
// -----------------------------------------------------------------------------

// ABEL_ATTRIBUTE_UNUSED
//
// Prevents the compiler from complaining about variables that appear unused.
#if ABEL_COMPILER_HAS_ATTRIBUTE(unused) || (defined(__GNUC__) && !defined(__clang__))
#undef ABEL_ATTRIBUTE_UNUSED
#define ABEL_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define ABEL_ATTRIBUTE_UNUSED
#endif


// ABEL_CONST_INIT
//
// A variable declaration annotated with the `ABEL_CONST_INIT` attribute will
// not compile (on supported platforms) unless the variable has a constant
// initializer. This is useful for variables with static and thread storage
// duration, because it guarantees that they will not suffer from the so-called
// "static init order fiasco".  Prefer to put this attribute on the most visible
// declaration of the variable, if there's more than one, because code that
// accesses the variable can then use the attribute for optimization.
//
// Example:
//
//   class MyClass {
//    public:
//     ABEL_CONST_INIT static MyType my_var;
//   };
//
//   MyType MyClass::my_var = MakeMyType(...);
//
// Note that this attribute is redundant if the variable is declared constexpr.
#if ABEL_COMPILER_HAS_CPP_ATTRIBUTE(clang::require_constant_initialization)
#define ABEL_CONST_INIT [[clang::require_constant_initialization]]
#else
#define ABEL_CONST_INIT
#endif  // ABEL_COMPILER_HAS_CPP_ATTRIBUTE(clang::require_constant_initialization)

// ABEL_FUNC_ALIGN
//
// Tells the compiler to align the function start at least to certain
// alignment boundary
#if ABEL_COMPILER_HAS_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__))
#define ABEL_FUNC_ALIGN(bytes) __attribute__((aligned(bytes)))
#else
#define ABEL_FUNC_ALIGN(bytes)
#endif


// ABEL_FALLTHROUGH_INTENDED
//
// Annotates implicit fall-through between switch labels, allowing a case to
// indicate intentional fallthrough and turn off warnings about any lack of a
// `break` statement. The ABEL_FALLTHROUGH_INTENDED macro should be followed by
// a semicolon and can be used in most places where `break` can, provided that
// no statements exist between it and the next switch label.
//
// Example:
//
//  switch (x) {
//    case 40:
//    case 41:
//      if (truth_is_out_there) {
//        ++x;
//        ABEL_FALLTHROUGH_INTENDED;  // Use instead of/along with annotations
//                                    // in comments
//      } else {
//        return x;
//      }
//    case 42:
//      ...
//
// Notes: when compiled with clang in C++11 mode, the ABEL_FALLTHROUGH_INTENDED
// macro is expanded to the [[clang::fallthrough]] attribute, which is analysed
// when  performing switch labels fall-through diagnostic
// (`-Wimplicit-fallthrough`). See clang documentation on language extensions
// for details:
// http://clang.llvm.org/docs/AttributeReference.html#fallthrough-clang-fallthrough
//
// When used with unsupported compilers, the ABEL_FALLTHROUGH_INTENDED macro
// has no effect on diagnostics. In any case this macro has no effect on runtime
// behavior and performance of code.
#ifdef ABEL_FALLTHROUGH_INTENDED
#error "ABEL_FALLTHROUGH_INTENDED should not be defined."
#endif

// TODO(zhangxy): Use c++17 standard [[fallthrough]] macro, when supported.
#if defined(__clang__) && defined(__has_warning)
#if __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#define ABEL_FALLTHROUGH_INTENDED [[clang::fallthrough]]
#endif
#elif defined(__GNUC__) && __GNUC__ >= 7
#define ABEL_FALLTHROUGH_INTENDED [[gnu::fallthrough]]
#endif

#ifndef ABEL_FALLTHROUGH_INTENDED
#define ABEL_FALLTHROUGH_INTENDED \
  do {                            \
  } while (0)
#endif

// ABEL_BAD_CALL_IF()
//
// Used on a function overload to trap bad calls: any call that matches the
// overload will cause a compile-time error. This macro uses a clang-specific
// "enable_if" attribute, as described at
// http://clang.llvm.org/docs/AttributeReference.html#enable-if
//
// Overloads which use this macro should be bracketed by
// `#ifdef ABEL_BAD_CALL_IF`.
//
// Example:
//
//   int isdigit(int c);
//   #ifdef ABEL_BAD_CALL_IF
//   int isdigit(int c)
//     ABEL_BAD_CALL_IF(c <= -1 || c > 255,
//                       "'c' must have the value of an unsigned char or EOF");
//   #endif // ABEL_BAD_CALL_IF
#if ABEL_COMPILER_HAS_ATTRIBUTE(enable_if)
#define ABEL_BAD_CALL_IF(expr, msg) \
  __attribute__((enable_if(expr, "Bad call trap"), unavailable(msg)))
#endif


// ABEL_REINITIALIZES
//
// Indicates that a member function reinitializes the entire object to a known
// state, independent of the previous state of the object.
//
// The clang-tidy check bugprone-use-after-move allows member functions marked
// with this attribute to be called on objects that have been moved from;
// without the attribute, this would result in a use-after-move warning.
#if ABEL_COMPILER_HAS_CPP_ATTRIBUTE(clang::reinitializes)
#define ABEL_REINITIALIZES [[clang::reinitializes]]
#else
#define ABEL_REINITIALIZES
#endif


// ABEL_HAVE_ATTRIBUTE_SECTION
//
// Indicates whether labeled sections are supported. Weak symbol support is
// a prerequisite. Labeled sections are not supported on Darwin/iOS.
#ifdef ABEL_HAVE_ATTRIBUTE_SECTION
#error ABEL_HAVE_ATTRIBUTE_SECTION cannot be directly set
#elif (ABEL_COMPILER_HAS_ATTRIBUTE(section) || \
       (defined(__GNUC__) && !defined(__clang__))) && \
    !defined(__APPLE__) && ABEL_WEAK_SUPPORTED
#define ABEL_HAVE_ATTRIBUTE_SECTION 1

// ABEL_ATTRIBUTE_SECTION
//
// Tells the compiler/linker to put a given function into a section and define
// `__start_ ## name` and `__stop_ ## name` symbols to bracket the section.
// This functionality is supported by GNU linker.  Any function annotated with
// `ABEL_ATTRIBUTE_SECTION` must not be inlined, or it will be placed into
// whatever section its caller is placed into.
//
#ifndef ABEL_ATTRIBUTE_SECTION
#define ABEL_ATTRIBUTE_SECTION(name) \
  __attribute__((section(#name))) __attribute__((noinline))
#endif


// ABEL_ATTRIBUTE_SECTION_VARIABLE
//
// Tells the compiler/linker to put a given variable into a section and define
// `__start_ ## name` and `__stop_ ## name` symbols to bracket the section.
// This functionality is supported by GNU linker.
#ifndef ABEL_ATTRIBUTE_SECTION_VARIABLE
#define ABEL_ATTRIBUTE_SECTION_VARIABLE(name) __attribute__((section(#name)))
#endif

// ABEL_DECLARE_ATTRIBUTE_SECTION_VARS
//
// A weak section declaration to be used as a global declaration
// for ABEL_ATTRIBUTE_SECTION_START|STOP(name) to compile and link
// even without functions with ABEL_ATTRIBUTE_SECTION(name).
// ABEL_DEFINE_ATTRIBUTE_SECTION should be in the exactly one file; it's
// a no-op on ELF but not on Mach-O.
//
#ifndef ABEL_DECLARE_ATTRIBUTE_SECTION_VARS
#define ABEL_DECLARE_ATTRIBUTE_SECTION_VARS(name) \
  extern char __start_##name[] ABEL_WEAK;    \
  extern char __stop_##name[] ABEL_WEAK
#endif
#ifndef ABEL_DEFINE_ATTRIBUTE_SECTION_VARS
#define ABEL_INIT_ATTRIBUTE_SECTION_VARS(name)
#define ABEL_DEFINE_ATTRIBUTE_SECTION_VARS(name)
#endif

// ABEL_ATTRIBUTE_SECTION_START
//
// Returns `void*` pointers to start/end of a section of code with
// functions having ABEL_ATTRIBUTE_SECTION(name).
// Returns 0 if no such functions exist.
// One must ABEL_DECLARE_ATTRIBUTE_SECTION_VARS(name) for this to compile and
// link.
//
#define ABEL_ATTRIBUTE_SECTION_START(name) \
  (reinterpret_cast<void *>(__start_##name))
#define ABEL_ATTRIBUTE_SECTION_STOP(name) \
  (reinterpret_cast<void *>(__stop_##name))

#else  // !ABEL_HAVE_ATTRIBUTE_SECTION

#define ABEL_HAVE_ATTRIBUTE_SECTION 0

// provide dummy definitions
#define ABEL_ATTRIBUTE_SECTION(name)
#define ABEL_ATTRIBUTE_SECTION_VARIABLE(name)
#define ABEL_INIT_ATTRIBUTE_SECTION_VARS(name)
#define ABEL_DEFINE_ATTRIBUTE_SECTION_VARS(name)
#define ABEL_DECLARE_ATTRIBUTE_SECTION_VARS(name)
#define ABEL_ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void *>(0))
#define ABEL_ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void *>(0))

#endif  // ABEL_ATTRIBUTE_SECTION

// ABEL_ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
//
// Support for aligning the stack on 32-bit x86.
#if ABEL_COMPILER_HAS_ATTRIBUTE(force_align_arg_pointer) || \
    (defined(__GNUC__) && !defined(__clang__))
#if defined(__i386__)
#define ABEL_ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC \
  __attribute__((force_align_arg_pointer))
#define ABEL_REQUIRE_STACK_ALIGN_TRAMPOLINE (0)
#elif defined(__x86_64__)
#define ABEL_REQUIRE_STACK_ALIGN_TRAMPOLINE (1)
#define ABEL_ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#else  // !__i386__ && !__x86_64
#define ABEL_REQUIRE_STACK_ALIGN_TRAMPOLINE (0)
#define ABEL_ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#endif  // __i386__
#else
#define ABEL_ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#define ABEL_REQUIRE_STACK_ALIGN_TRAMPOLINE (0)
#endif

// ABEL_MUST_USE_RESULT
//
// Tells the compiler to warn about unused results.
//
// When annotating a function, it must appear as the first part of the
// declaration or definition. The compiler will warn if the return value from
// such a function is unused:
//
//   ABEL_MUST_USE_RESULT Sprocket* AllocateSprocket();
//   AllocateSprocket();  // Triggers a warning.
//
// When annotating a class, it is equivalent to annotating every function which
// returns an instance.
//
//   class ABEL_MUST_USE_RESULT Sprocket {};
//   Sprocket();  // Triggers a warning.
//
//   Sprocket MakeSprocket();
//   MakeSprocket();  // Triggers a warning.
//
// Note that references and pointers are not instances:
//
//   Sprocket* SprocketPointer();
//   SprocketPointer();  // Does *not* trigger a warning.
//
// ABEL_MUST_USE_RESULT allows using cast-to-void to suppress the unused result
// warning. For that, warn_unused_result is used only for clang but not for gcc.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425
//
// Note: past advice was to place the macro after the argument list.
#if ABEL_COMPILER_HAS_ATTRIBUTE(nodiscard)
#define ABEL_MUST_USE_RESULT [[nodiscard]]
#elif defined(__clang__) && ABEL_COMPILER_HAS_ATTRIBUTE(warn_unused_result)
#define ABEL_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define ABEL_MUST_USE_RESULT
#endif

// ABEL_HOT, ABEL_COLD
//
// Tells GCC that a function is hot or cold. GCC can use this information to
// improve static analysis, i.e. a conditional branch to a cold function
// is likely to be not-taken.
// This annotation is used for function declarations.
//
// Example:
//
//   int foo() ABEL_HOT;
#if ABEL_COMPILER_HAS_ATTRIBUTE(hot) || (defined(__GNUC__) && !defined(__clang__))
#define ABEL_HOT __attribute__((hot))
#else
#define ABEL_HOT
#endif

#if ABEL_COMPILER_HAS_ATTRIBUTE(cold) || (defined(__GNUC__) && !defined(__clang__))
#define ABEL_COLD __attribute__((cold))
#else
#define ABEL_COLD
#endif

// ABEL_XRAY_ALWAYS_INSTRUMENT, ABEL_XRAY_NEVER_INSTRUMENT, ABEL_XRAY_LOG_ARGS
//
// We define the ABEL_XRAY_ALWAYS_INSTRUMENT and ABEL_XRAY_NEVER_INSTRUMENT
// macro used as an attribute to mark functions that must always or never be
// instrumented by XRay. Currently, this is only supported in Clang/LLVM.
//
// For reference on the LLVM XRay instrumentation, see
// http://llvm.org/docs/XRay.html.
//
// A function with the XRAY_ALWAYS_INSTRUMENT macro attribute in its declaration
// will always get the XRay instrumentation sleds. These sleds may introduce
// some binary size and runtime overhead and must be used sparingly.
//
// These attributes only take effect when the following conditions are met:
//
//   * The file/target is built in at least C++11 mode, with a Clang compiler
//     that supports XRay attributes.
//   * The file/target is built with the -fxray-instrument flag set for the
//     Clang/LLVM compiler.
//   * The function is defined in the translation unit (the compiler honors the
//     attribute in either the definition or the declaration, and must match).
//
// There are cases when, even when building with XRay instrumentation, users
// might want to control specifically which functions are instrumented for a
// particular build using special-case lists provided to the compiler. These
// special case lists are provided to Clang via the
// -fxray-always-instrument=... and -fxray-never-instrument=... flags. The
// attributes in source take precedence over these special-case lists.
//
// To disable the XRay attributes at build-time, users may define
// ABEL_NO_XRAY_ATTRIBUTES. Do NOT define ABEL_NO_XRAY_ATTRIBUTES on specific
// packages/targets, as this may lead to conflicting definitions of functions at
// link-time.
//
#if ABEL_COMPILER_HAS_CPP_ATTRIBUTE(clang::xray_always_instrument) && \
    !defined(ABEL_NO_XRAY_ATTRIBUTES)
#define ABEL_XRAY_ALWAYS_INSTRUMENT [[clang::xray_always_instrument]]
#define ABEL_XRAY_NEVER_INSTRUMENT [[clang::xray_never_instrument]]
#if ABEL_COMPILER_HAS_CPP_ATTRIBUTE(clang::xray_log_args)
#define ABEL_XRAY_LOG_ARGS(N) \
    [[clang::xray_always_instrument, clang::xray_log_args(N)]]
#else
#define ABEL_XRAY_LOG_ARGS(N) [[clang::xray_always_instrument]]
#endif
#else
#define ABEL_XRAY_ALWAYS_INSTRUMENT
#define ABEL_XRAY_NEVER_INSTRUMENT
#define ABEL_XRAY_LOG_ARGS(N)
#endif


#ifndef ABEL_THREAD_LOCAL

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_INTEL) || defined(ABEL_COMPILER_CLANG)
#define ABEL_THREAD_LOCAL __thread
#elif defined(ABEL_COMPILER_MSVC)
#define ABEL_THREAD_LOCAL __declspec(thread)
#else
#define ABEL_THREAD_LOCAL thread_local
#endif

#define ABEL_THREAD_STACK_LOCAL thread_local

#endif //ABEL_THREAD_LOCAL

#endif //ABEL_BASE_PROFILE_COMPILER_TRAITS_H_
