
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BOOTSTRAP_BOOTSTRAP_H_
#define MELON_BOOTSTRAP_BOOTSTRAP_H_


#include <cstdint>
#include <utility>
#include <functional>
#include "melon/base/profile.h"

// Usage:
//
// - MELON_BOOTSTRAP(init, fini = nullptr): This overload defaults `priority` to
//   1.
// - MELON_BOOTSTRAP(priority, init, fini = nullptr)
//
// This macro registers a callback that is called in `melon::Start` (after
// `main` is entered`.). The user may also provide a finalizer, which is called
// before leaving `melon::Start`, in opposite order.
//
// `priority` specifies relative order between callbacks. Callbacks with smaller
// `priority` are called earlier. Order between callbacks with same priority is
// unspecified and may not be relied on.
//
// It explicitly allowed to use this macro *without* carring a dependency to
// `//melon:init`.
//
// For UT writers: If, for any reason, you cannot use `MELON_TEST_MAIN` as your
// `main` in UT, you need to call `RunAllInitializers()` / `RunAllFinalizers()`
// yourself when entering / leaving `main`.

#define MELON_BOOTSTRAP(...)                                                 \
  static ::melon::internal::bootstrap_registration MELON_CONCAT(        \
      melon_bootstrap_registration_object_, __LINE__)(__FILE__, __LINE__, \
                                                       __VA_ARGS__);

// Implementation goes below.
namespace melon {

    // Called by `OnInitRegistration`.
    void register_bootstrap_callback(std::int32_t priority, const std::function<void()> &init, const std::function<void()> &fini);

    void register_bootstrap_callback(std::int32_t priority, std::function<void()> &&init, std::function<void()> &&fini);
    // Registers a callback that's called before leaving `melon::Start`.
    //
    // These callbacks are called after all finalizers registered via
    // `MELON_BOOTSTRAP`.
    void set_at_exit_callback(std::function<void()> callback);

    void bootstrap_init(int argc, char**argv);
    // Called `melon::run_bootstrap`.
    void run_bootstrap();

    void run_finalizers();

}  // namespace melon

namespace melon::internal {

    // Helper class for registering initialization callbacks at start-up time.
    class bootstrap_registration {
    public:
        bootstrap_registration(const char *file, std::uint32_t line,
                           std::function<void()> init, std::function<void()> fini = nullptr)
                : bootstrap_registration(file, line, 1, std::move(init), std::move(fini)) {}

        bootstrap_registration(const char *file, std::uint32_t line,
                           std::int32_t priority, std::function<void()> init,
                           std::function<void()> fini = nullptr) {

            melon::register_bootstrap_callback(priority, std::move(init), std::move(fini));
        }
    };

}  // namespace melon::internal


#endif // MELON_BOOTSTRAP_BOOTSTRAP_H_
