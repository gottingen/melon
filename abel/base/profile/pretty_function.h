//

#ifndef ABEL_BASE_PROFILE_PRETTY_FUNCTION_H_
#define ABEL_BASE_PROFILE_PRETTY_FUNCTION_H_

// ABEL_PRETTY_FUNCTION
//
// In C++11, __func__ gives the undecorated name of the current function.  That
// is, "main", not "int main()".  Various compilers give extra macros to get the
// decorated function name, including return type and arguments, to
// differentiate between overload sets.  ABEL_PRETTY_FUNCTION is a portable
// version of these macros which forwards to the correct macro on each compiler.
#if defined(_MSC_VER)
    #define ABEL_PRETTY_FUNCTION __FUNCSIG__
#elif defined(__GNUC__)
    #define ABEL_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
    #error "Unsupported compiler"
#endif

#endif  // ABEL_BASE_PROFILE_PRETTY_FUNCTION_H_
