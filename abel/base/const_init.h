//
// -----------------------------------------------------------------------------
// kConstInit
// -----------------------------------------------------------------------------
//
// A constructor tag used to mark an object as safe for use as a global
// variable, avoiding the usual lifetime issues that can affect globals.

#ifndef ABEL_BASE_CONST_INIT_H_
#define ABEL_BASE_CONST_INIT_H_

#include <abel/base/profile.h>

// In general, objects with static storage duration (such as global variables)
// can trigger tricky object lifetime situations.  Attempting to access them
// from the constructors or destructors of other global objects can result in
// undefined behavior, unless their constructors and destructors are designed
// with this issue in mind.
//
// The normal way to deal with this issue in C++11 is to use constant
// initialization and trivial destructors.
//
// Constant initialization is guaranteed to occur before any other code
// executes.  Constructors that are declared 'constexpr' are eligible for
// constant initialization.  You can annotate a variable declaration with the
// ABEL_CONST_INIT macro to express this intent.  For compilers that support
// it, this annotation will cause a compilation error for declarations that
// aren't subject to constant initialization (perhaps because a runtime value
// was passed as a constructor argument).
//
// On program shutdown, lifetime issues can be avoided on global objects by
// ensuring that they contain  trivial destructors.  A class has a trivial
// destructor unless it has a user-defined destructor, a virtual method or base
// class, or a data member or base class with a non-trivial destructor of its
// own.  Objects with static storage duration and a trivial destructor are not
// cleaned up on program shutdown, and are thus safe to access from other code
// running during shutdown.
//
// For a few core abel classes, we make a best effort to allow for safe global
// instances, even though these classes have non-trivial destructors.  These
// objects can be created with the abel::kConstInit tag.  For example:
//   ABEL_CONST_INIT abel::mutex global_mutex(abel::kConstInit);
//
// The line above declares a global variable of type abel::mutex which can be
// accessed at any point during startup or shutdown.  global_mutex's destructor
// will still run, but will not invalidate the object.  Note that C++ specifies
// that accessing an object after its destructor has run results in undefined
// behavior, but this pattern works on the toolchains we support.
//
// The abel::kConstInit tag should only be used to define objects with static
// or thread_local storage duration.

namespace abel {
ABEL_NAMESPACE_BEGIN

enum const_init_type {
  kConstInit,
};

ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_CONST_INIT_H_
