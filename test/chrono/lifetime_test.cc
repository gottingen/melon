//

#include <cstdlib>
#include <thread>  // NOLINT(build/c++11), abel test
#include <type_traits>

#include <abel/base/profile.h>
#include <abel/base/const_init.h>
#include <abel/log/abel_logging.h>
#include <abel/thread/thread_annotations.h>
#include <abel/thread/mutex.h>
#include <abel/thread/notification.h>

namespace {

// A two-threaded test which checks that mutex, cond_var, and notification have
// correct basic functionality.  The intent is to establish that they
// function correctly in various phases of construction and destruction.
//
// Thread one acquires a lock on 'mutex', wakes thread two via 'notification',
// then waits for 'state' to be set, as signalled by 'condvar'.
//
// Thread two waits on 'notification', then sets 'state' inside the 'mutex',
// signalling the change via 'condvar'.
//
// These tests use ABEL_RAW_CHECK to validate invariants, rather than EXPECT or
// ASSERT from gUnit, because we need to invoke them during global destructors,
// when gUnit teardown would have already begun.
    void ThreadOne(abel::mutex *mutex, abel::cond_var *condvar,
                   abel::notification *notification, bool *state) {
        // Test that the notification is in a valid initial state.
        ABEL_RAW_CHECK(!notification->has_been_notified(), "invalid notification");
        ABEL_RAW_CHECK(*state == false, "*state not initialized");

        {
            abel::mutex_lock lock(mutex);

            notification->notify();
            ABEL_RAW_CHECK(notification->has_been_notified(), "invalid notification");

            while (*state == false) {
                condvar->wait(mutex);
            }
        }
    }

    void ThreadTwo(abel::mutex *mutex, abel::cond_var *condvar,
                   abel::notification *notification, bool *state) {
        ABEL_RAW_CHECK(*state == false, "*state not initialized");

        // Wake thread one
        notification->wait_for_notification();
        ABEL_RAW_CHECK(notification->has_been_notified(), "invalid notification");
        {
            abel::mutex_lock lock(mutex);
            *state = true;
            condvar->signal();
        }
    }

// Launch thread 1 and thread 2, and block on their completion.
// If any of 'mutex', 'condvar', or 'notification' is nullptr, use a locally
// constructed instance instead.
    void RunTests(abel::mutex *mutex, abel::cond_var *condvar) {
        abel::mutex default_mutex;
        abel::cond_var default_condvar;
        abel::notification notification;
        if (!mutex) {
            mutex = &default_mutex;
        }
        if (!condvar) {
            condvar = &default_condvar;
        }
        bool state = false;
        std::thread thread_one(ThreadOne, mutex, condvar, &notification, &state);
        std::thread thread_two(ThreadTwo, mutex, condvar, &notification, &state);
        thread_one.join();
        thread_two.join();
    }

    void TestLocals() {
        abel::mutex mutex;
        abel::cond_var condvar;
        RunTests(&mutex, &condvar);
    }

// Normal kConstInit usage
    ABEL_CONST_INIT abel::mutex const_init_mutex(abel::kConstInit);

    void TestConstInitGlobal() { RunTests(&const_init_mutex, nullptr); }

// Global variables during start and termination
//
// In a translation unit, static storage duration variables are initialized in
// the order of their definitions, and destroyed in the reverse order of their
// definitions.  We can use this to arrange for tests to be run on these objects
// before they are created, and after they are destroyed.

    using Function = void (*)();

    class OnConstruction {
    public:
        explicit OnConstruction(Function fn) { fn(); }
    };

    class OnDestruction {
    public:
        explicit OnDestruction(Function fn) : fn_(fn) {}

        ~OnDestruction() { fn_(); }

    private:
        Function fn_;
    };

// These tests require that the compiler correctly supports C++11 constant
// initialization... but MSVC has a known regression since v19.10:
// https://developercommunity.visualstudio.com/content/problem/336946/class-with-constexpr-constructor-not-using-static.html
// TODO(epastor): Limit the affected range once MSVC fixes this bug.
#if defined(__clang__) || !(defined(_MSC_VER) && _MSC_VER > 1900)
// kConstInit
// Test early usage.  (Declaration comes first; definitions must appear after
// the test runner.)
    extern abel::mutex early_const_init_mutex;
// (Normally I'd write this +[], to make the cast-to-function-pointer explicit,
// but in some MSVC setups we support, lambdas provide conversion operators to
// different flavors of function pointers, making this trick ambiguous.)
    OnConstruction test_early_const_init([] {
        RunTests(&early_const_init_mutex, nullptr);
    });
// This definition appears before test_early_const_init, but it should be
// initialized first (due to constant initialization).  Test that the object
// actually works when constructed this way.
    ABEL_CONST_INIT abel::mutex early_const_init_mutex(abel::kConstInit);

// Furthermore, test that the const-init c'tor doesn't stomp over the state of
// a mutex.  Really, this is a test that the platform under test correctly
// supports C++11 constant initialization.  (The constant-initialization
// constructors of globals "happen at link time"; memory is pre-initialized,
// before the constructors of either grab_lock or check_still_locked are run.)
    extern abel::mutex const_init_sanity_mutex;
    OnConstruction grab_lock([]() ABEL_NO_THREAD_SAFETY_ANALYSIS {
        const_init_sanity_mutex.lock();
    });
    ABEL_CONST_INIT abel::mutex const_init_sanity_mutex(abel::kConstInit);
    OnConstruction check_still_locked([]() ABEL_NO_THREAD_SAFETY_ANALYSIS {
        const_init_sanity_mutex.assert_held();
        const_init_sanity_mutex.unlock();
    });
#endif  // defined(__clang__) || !(defined(_MSC_VER) && _MSC_VER > 1900)

// Test shutdown usage.  (Declarations come first; definitions must appear after
// the test runner.)
    extern abel::mutex late_const_init_mutex;
// OnDestruction is being used here as a global variable, even though it has a
// non-trivial destructor.  This is against the style guide.  We're violating
// that rule here to check that the exception we allow for kConstInit is safe.
// NOLINTNEXTLINE
    OnDestruction test_late_const_init([] {
        RunTests(&late_const_init_mutex, nullptr);
    });
    ABEL_CONST_INIT abel::mutex late_const_init_mutex(abel::kConstInit);

}  // namespace

int main() {
    TestLocals();
    TestConstInitGlobal();
    // Explicitly call exit(0) here, to make it clear that we intend for the
    // above global object destructors to run.
    std::exit(0);
}
