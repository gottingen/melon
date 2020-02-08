#ifndef ABEL_BASE_OPTIONS_H_
#define ABEL_BASE_OPTIONS_H_

//
// -----------------------------------------------------------------------------
// File: options.h
// -----------------------------------------------------------------------------
//
// This file contains abel configuration options for setting specific
// implementations instead of letting abel determine which implementation to
// use at compile-time. Setting these options may be useful for package or build
// managers who wish to guarantee ABI stability within binary builds (which are
// otherwise difficult to enforce).
//
// *** IMPORTANT NOTICE FOR PACKAGE MANAGERS:  It is important that
// maintainers of package managers who wish to package abel read and
// understand this file! ***
//
// abel contains a number of possible configuration endpoints, based on
// parameters such as the detected platform, language version, or command-line
// flags used to invoke the underlying binary. As is the case with all
// libraries, binaries which contain abel code must ensure that separate
// packages use the same compiled copy of abel to avoid a diamond dependency
// problem, which can occur if two packages built with different abel
// configuration settings are linked together. Diamond dependency problems in
// C++ may manifest as violations to the One Definition Rule (ODR) (resulting in
// linker errors), or undefined behavior (resulting in crashes).
//
// Diamond dependency problems can be avoided if all packages utilize the same
// exact version of abel. Building from source code with the same compilation
// parameters is the easiest way to avoid such dependency problems. However, for
// package managers who cannot control such compilation parameters, we are
// providing the file to allow you to inject ABI (Application Binary Interface)
// stability across builds. Settings options in this file will neither change
// API nor ABI, providing a stable copy of abel between packages.
//
// Care must be taken to keep options within these configurations isolated
// from any other dynamic settings, such as command-line flags which could alter
// these options. This file is provided specifically to help build and package
// managers provide a stable copy of abel within their libraries and binaries;
// other developers should not have need to alter the contents of this file.
//
// -----------------------------------------------------------------------------
// Usage
// -----------------------------------------------------------------------------
//
// For any particular package release, set the appropriate definitions within
// this file to whatever value makes the most sense for your package(s). Note
// that, by default, most of these options, at the moment, affect the
// implementation of types; future options may affect other implementation
// details.
//
// NOTE: the defaults within this file all assume that abel can select the
// proper abel implementation at compile-time, which will not be sufficient
// to guarantee ABI stability to package managers.
//
// -----------------------------------------------------------------------------
// Type Compatibility Options
// -----------------------------------------------------------------------------
//
// ABEL_OPTION_USE_STD_ANY
//
// This option controls whether abel::any is implemented as an alias to
// std::any, or as an independent implementation.
//
// A value of 0 means to use abel's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::any.  This requires that all code
// using abel is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile abel,
// and use an alias only if a working std::any is available.  This option is
// useful when you are building your entire program, including all of its
// dependencies, from source.  It should not be used otherwise -- for example,
// if you are distributing abel in a binary package manager -- since in
// mode 2, abel::any will name a different type, with a different mangled name
// and binary layout, depending on the compiler flags passed by the end user.
//
// User code should not inspect this macro.  To check in the preprocessor if
// abel::any is a typedef of std::any, use the feature macro ABEL_USES_STD_ANY.

#define ABEL_OPTION_USE_STD_ANY 2


// ABEL_OPTION_USE_STD_OPTIONAL
//
// This option controls whether abel::optional is implemented as an alias to
// std::optional, or as an independent implementation.
//
// A value of 0 means to use abel's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::optional.  This requires that all
// code using abel is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile abel,
// and use an alias only if a working std::optional is available.  This option
// is useful when you are building your program from source.  It should not be
// used otherwise -- for example, if you are distributing abel in a binary
// package manager -- since in mode 2, abel::optional will name a different
// type, with a different mangled name and binary layout, depending on the
// compiler flags passed by the end user.  

// A value of 2 means to detect the C++ version being used to compile abel,
// and use an alias only if a working std::optional is available.  This option
// should not be used when your program is not built from source -- for example,
// if you are distributing abel in a binary package manager -- since in mode
// 2, abel::optional will name a different template class, with a different
// mangled name and binary layout, depending on the compiler flags passed by the
// end user.
//
// User code should not inspect this macro.  To check in the preprocessor if
// abel::optional is a typedef of std::optional, use the feature macro
// ABEL_USES_STD_OPTIONAL.

#define ABEL_OPTION_USE_STD_OPTIONAL 2


// ABEL_OPTION_USE_STD_STRING_VIEW
//
// This option controls whether abel::string_view is implemented as an alias to
// std::string_view, or as an independent implementation.
//
// A value of 0 means to use abel's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::string_view.  This requires that
// all code using abel is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile abel,
// and use an alias only if a working std::string_view is available.  This
// option is useful when you are building your program from source.  It should
// not be used otherwise -- for example, if you are distributing abel in a
// binary package manager -- since in mode 2, abel::string_view will name a
// different type, with a different mangled name and binary layout, depending on
// the compiler flags passed by the end user.  
// User code should not inspect this macro.  To check in the preprocessor if
// abel::string_view is a typedef of std::string_view, use the feature macro
// ABEL_USES_STD_STRING_VIEW.

#define ABEL_OPTION_USE_STD_STRING_VIEW 2


// ABEL_OPTION_USE_STD_VARIANT
//
// This option controls whether abel::variant is implemented as an alias to
// std::variant, or as an independent implementation.
//
// A value of 0 means to use abel's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::variant.  This requires that all
// code using abel is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile abel,
// and use an alias only if a working std::variant is available.  This option
// is useful when you are building your program from source.  It should not be
// used otherwise -- for example, if you are distributing abel in a binary
// package manager -- since in mode 2, abel::variant will name a different
// type, with a different mangled name and binary layout, depending on the
// compiler flags passed by the end user.
//
// User code should not inspect this macro.  To check in the preprocessor if
// abel::variant is a typedef of std::variant, use the feature macro
// ABEL_USES_STD_VARIANT.

#define ABEL_OPTION_USE_STD_VARIANT 2

#endif  // ABEL_BASE_OPTIONS_H_
