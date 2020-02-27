//

#include <abel/synchronization/mutex.h>

#ifdef _WIN32
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#else

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>

#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <thread>  // NOLINT(build/c++11)
#include <abel/base/profile.h>
#include <abel/threading/dynamic_annotations.h>
#include <abel/atomic/atomic_hook.h>
#include <abel/chrono/internal/cycle_clock.h>
#include <abel/memory/hide_ptr.h>
#include <abel/memory/internal/low_level_alloc.h>
#include <abel/log/raw_logging.h>
#include <abel/threading/internal/spinlock.h>
#include <abel/system/sysinfo.h>
#include <abel/threading/internal/thread_identity.h>
#include <abel/base/profile.h>
#include <abel/debugging/stacktrace.h>
#include <abel/debugging/symbolize.h>
#include <abel/synchronization/internal/graphcycles.h>
#include <abel/synchronization/internal/per_thread_sem.h>
#include <abel/chrono/time.h>

using abel::threading_internal::CurrentThreadIdentityIfPresent;
using abel::threading_internal::PerThreadSynch;
using abel::threading_internal::ThreadIdentity;
using abel::synchronization_internal::GetOrCreateCurrentThreadIdentity;
using abel::synchronization_internal::GraphCycles;
using abel::synchronization_internal::GraphId;
using abel::synchronization_internal::InvalidGraphId;
using abel::synchronization_internal::KernelTimeout;
using abel::synchronization_internal::PerThreadSem;

extern "C" {
ABEL_WEAK void abel_internal_mutex_yield() { std::this_thread::yield(); }
}  // extern "C"

namespace abel {


    namespace {

#if defined(THREAD_SANITIZER)
        constexpr on_deadlock_cycle kDeadlockDetectionDefault = on_deadlock_cycle::kIgnore;
#else
        constexpr on_deadlock_cycle kDeadlockDetectionDefault = on_deadlock_cycle::kAbort;
#endif

        ABEL_CONST_INIT std::atomic<on_deadlock_cycle> synch_deadlock_detection(
                kDeadlockDetectionDefault);
        ABEL_CONST_INIT std::atomic<bool> synch_check_invariants(false);

// ------------------------------------------ spinlock support

// Make sure read-only globals used in the mutex code are contained on the
// same cacheline and cacheline aligned to eliminate any false sharing with
// other globals from this and other modules.
        static struct MutexGlobals {
            MutexGlobals() {
                // Find machine-specific data needed for Delay() and
                // TryAcquireWithSpinning(). This runs in the global constructor
                // sequence, and before that zeros are safe values.
                num_cpus = abel::num_cpus();
                spinloop_iterations = num_cpus > 1 ? 1500 : 0;
            }

            int num_cpus;
            int spinloop_iterations;
            // Pad this struct to a full cacheline to prevent false sharing.
            char padding[ABEL_CACHE_LINE_SIZE - 2 * sizeof(int)];
        } ABEL_CACHE_LINE_ALIGNED mutex_globals;

        static_assert(
                sizeof(MutexGlobals) == ABEL_CACHE_LINE_SIZE,
                "MutexGlobals must occupy an entire cacheline to prevent false sharing");

        ABEL_CONST_INIT abel::atomic_hook<void (*)(int64_t wait_cycles)>
                submit_profile_data;
        ABEL_CONST_INIT abel::atomic_hook<
                void (*)(const char *msg, const void *obj, int64_t wait_cycles)> mutex_tracer;
        ABEL_CONST_INIT abel::atomic_hook<
                void (*)(const char *msg, const void *cv)> cond_var_tracer;
        ABEL_CONST_INIT abel::atomic_hook<
                bool (*)(const void *pc, char *out, int out_size)>
                symbolizer(abel::Symbolize);

    }  // namespace

    static ABEL_FORCE_INLINE bool EvalConditionAnnotated(const condition *cond, mutex *mu,
                                                         bool locking, bool trylock,
                                                         bool read_lock);

    void register_mutex_profiler(void (*fn)(int64_t wait_timestamp)) {
        submit_profile_data.Store(fn);
    }

    void register_mutex_tracer(void (*fn)(const char *msg, const void *obj,
                                          int64_t wait_cycles)) {
        mutex_tracer.Store(fn);
    }

    void register_cond_var_tracer(void (*fn)(const char *msg, const void *cv)) {
        cond_var_tracer.Store(fn);
    }

    void register_symbolizer(bool (*fn)(const void *pc, char *out, int out_size)) {
        symbolizer.Store(fn);
    }

// spinlock delay on iteration c.  Returns new c.
    namespace {
        enum DelayMode {
            AGGRESSIVE, GENTLE
        };
    };

    static int Delay(int32_t c, DelayMode mode) {
        // If this a uniprocessor, only yield/sleep.  Otherwise, if the mode is
        // aggressive then spin many times before yielding.  If the mode is
        // gentle then spin only a few times before yielding.  Aggressive spinning is
        // used to ensure that an unlock() call, which  must get the spin lock for
        // any thread to make progress gets it without undue delay.
        int32_t limit = (mutex_globals.num_cpus > 1) ?
                        ((mode == AGGRESSIVE) ? 5000 : 250) : 0;
        if (c < limit) {
            c++;               // spin
        } else {
            ABEL_TSAN_MUTEX_PRE_DIVERT(nullptr, 0);
            if (c == limit) {  // yield once
                abel_internal_mutex_yield();
                c++;
            } else {           // then wait
                abel::sleep_for(abel::microseconds(10));
                c = 0;
            }
            ABEL_TSAN_MUTEX_POST_DIVERT(nullptr, 0);
        }
        return (c);
    }

// --------------------------Generic atomic ops
// Ensure that "(*pv & bits) == bits" by doing an atomic update of "*pv" to
// "*pv | bits" if necessary.  wait until (*pv & wait_until_clear)==0
// before making any change.
// This is used to set flags in mutex and condition variable words.
    static void AtomicSetBits(std::atomic<intptr_t> *pv, intptr_t bits,
                              intptr_t wait_until_clear) {
        intptr_t v;
        do {
            v = pv->load(std::memory_order_relaxed);
        } while ((v & bits) != bits &&
                 ((v & wait_until_clear) != 0 ||
                  !pv->compare_exchange_weak(v, v | bits,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)));
    }

// Ensure that "(*pv & bits) == 0" by doing an atomic update of "*pv" to
// "*pv & ~bits" if necessary.  wait until (*pv & wait_until_clear)==0
// before making any change.
// This is used to unset flags in mutex and condition variable words.
    static void AtomicClearBits(std::atomic<intptr_t> *pv, intptr_t bits,
                                intptr_t wait_until_clear) {
        intptr_t v;
        do {
            v = pv->load(std::memory_order_relaxed);
        } while ((v & bits) != 0 &&
                 ((v & wait_until_clear) != 0 ||
                  !pv->compare_exchange_weak(v, v & ~bits,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)));
    }

//------------------------------------------------------------------

// Data for doing deadlock detection.
    static abel::threading_internal::SpinLock deadlock_graph_mu(
            abel::base_internal::kLinkerInitialized);

// graph used to detect deadlocks.
    static GraphCycles *deadlock_graph ABEL_GUARDED_BY(deadlock_graph_mu)
            ABEL_PT_GUARDED_BY(deadlock_graph_mu);

//------------------------------------------------------------------
// An event mechanism for debugging mutex use.
// It also allows mutexes to be given names for those who can't handle
// addresses, and instead like to give their data structures names like
// "Henry", "Fido", or "Rupert IV, King of Yondavia".

    namespace {  // to prevent name pollution
        enum {       // mutex and cond_var events passed as "ev" to PostSynchEvent
            // mutex events
            SYNCH_EV_TRYLOCK_SUCCESS,
            SYNCH_EV_TRYLOCK_FAILED,
            SYNCH_EV_READERTRYLOCK_SUCCESS,
            SYNCH_EV_READERTRYLOCK_FAILED,
            SYNCH_EV_LOCK,
            SYNCH_EV_LOCK_RETURNING,
            SYNCH_EV_READERLOCK,
            SYNCH_EV_READERLOCK_RETURNING,
            SYNCH_EV_UNLOCK,
            SYNCH_EV_READERUNLOCK,

            // cond_var events
            SYNCH_EV_WAIT,
            SYNCH_EV_WAIT_RETURNING,
            SYNCH_EV_SIGNAL,
            SYNCH_EV_SIGNALALL,
        };

        enum {                    // Event flags
            SYNCH_F_R = 0x01,       // reader event
            SYNCH_F_LCK = 0x02,     // PostSynchEvent called with mutex held
            SYNCH_F_TRY = 0x04,     // try_lock or reader_try_lock
            SYNCH_F_UNLOCK = 0x08,  // unlock or reader_unlock

            SYNCH_F_LCK_W = SYNCH_F_LCK,
            SYNCH_F_LCK_R = SYNCH_F_LCK | SYNCH_F_R,
        };
    }  // anonymous namespace

// Properties of the events.
    static const struct {
        int flags;
        const char *msg;
    } event_properties[] = {
            {SYNCH_F_LCK_W | SYNCH_F_TRY,    "try_lock succeeded "},
            {0,                              "try_lock failed "},
            {SYNCH_F_LCK_R | SYNCH_F_TRY,    "reader_try_lock succeeded "},
            {0,                              "reader_try_lock failed "},
            {0,                              "lock blocking "},
            {SYNCH_F_LCK_W,                  "lock returning "},
            {0,                              "reader_lock blocking "},
            {SYNCH_F_LCK_R,                  "reader_lock returning "},
            {SYNCH_F_LCK_W | SYNCH_F_UNLOCK, "unlock "},
            {SYNCH_F_LCK_R | SYNCH_F_UNLOCK, "reader_unlock "},
            {0,                              "wait on "},
            {0,                              "wait unblocked "},
            {0,                              "signal on "},
            {0,                              "signal_all on "},
    };

    static abel::threading_internal::SpinLock synch_event_mu(
            abel::base_internal::kLinkerInitialized);
// protects synch_event

// Hash table size; should be prime > 2.
// Can't be too small, as it's used for deadlock detection information.
    static const uint32_t kNSynchEvent = 1031;

    static struct SynchEvent {     // this is a trivial hash table for the events
        // struct is freed when refcount reaches 0
        int refcount ABEL_GUARDED_BY(synch_event_mu);

        // buckets have linear, 0-terminated  chains
        SynchEvent *next ABEL_GUARDED_BY(synch_event_mu);

        // Constant after initialization
        uintptr_t masked_addr;  // object at this address is called "name"

        // No explicit synchronization used.  Instead we assume that the
        // client who enables/disables invariants/logging on a mutex does so
        // while the mutex is not being concurrently accessed by others.
        void (*invariant)(void *arg);  // called on each event
        void *arg;            // first arg to (*invariant)()
        bool log;             // logging turned on

        // Constant after initialization
        char name[1];         // actually longer---NUL-terminated std::string
    } *synch_event[kNSynchEvent] ABEL_GUARDED_BY(synch_event_mu);

// Ensure that the object at "addr" has a SynchEvent struct associated with it,
// set "bits" in the word there (waiting until lockbit is clear before doing
// so), and return a refcounted reference that will remain valid until
// UnrefSynchEvent() is called.  If a new SynchEvent is allocated,
// the string name is copied into it.
// When used with a mutex, the caller should also ensure that kMuEvent
// is set in the mutex word, and similarly for condition variables and kCVEvent.
    static SynchEvent *EnsureSynchEvent(std::atomic<intptr_t> *addr,
                                        const char *name, intptr_t bits,
                                        intptr_t lockbit) {
        uint32_t h = reinterpret_cast<intptr_t>(addr) % kNSynchEvent;
        SynchEvent *e;
        // first look for existing SynchEvent struct..
        synch_event_mu.lock();
        for (e = synch_event[h];
             e != nullptr && e->masked_addr != hide_ptr(addr);
             e = e->next) {
        }
        if (e == nullptr) {  // no SynchEvent struct found; make one.
            if (name == nullptr) {
                name = "";
            }
            size_t l = strlen(name);
            e = reinterpret_cast<SynchEvent *>(
                    memory_internal::LowLevelAlloc::Alloc(sizeof(*e) + l));
            e->refcount = 2;    // one for return value, one for linked list
            e->masked_addr = hide_ptr(addr);
            e->invariant = nullptr;
            e->arg = nullptr;
            e->log = false;
            strcpy(e->name, name);  // NOLINT(runtime/printf)
            e->next = synch_event[h];
            AtomicSetBits(addr, bits, lockbit);
            synch_event[h] = e;
        } else {
            e->refcount++;      // for return value
        }
        synch_event_mu.unlock();
        return e;
    }

// Deallocate the SynchEvent *e, whose refcount has fallen to zero.
    static void DeleteSynchEvent(SynchEvent *e) {
        memory_internal::LowLevelAlloc::Free(e);
    }

// Decrement the reference count of *e, or do nothing if e==null.
    static void UnrefSynchEvent(SynchEvent *e) {
        if (e != nullptr) {
            synch_event_mu.lock();
            bool del = (--(e->refcount) == 0);
            synch_event_mu.unlock();
            if (del) {
                DeleteSynchEvent(e);
            }
        }
    }

// Forget the mapping from the object (mutex or cond_var) at address addr
// to SynchEvent object, and clear "bits" in its word (waiting until lockbit
// is clear before doing so).
    static void ForgetSynchEvent(std::atomic<intptr_t> *addr, intptr_t bits,
                                 intptr_t lockbit) {
        uint32_t h = reinterpret_cast<intptr_t>(addr) % kNSynchEvent;
        SynchEvent **pe;
        SynchEvent *e;
        synch_event_mu.lock();
        for (pe = &synch_event[h];
             (e = *pe) != nullptr && e->masked_addr != hide_ptr(addr);
             pe = &e->next) {
        }
        bool del = false;
        if (e != nullptr) {
            *pe = e->next;
            del = (--(e->refcount) == 0);
        }
        AtomicClearBits(addr, bits, lockbit);
        synch_event_mu.unlock();
        if (del) {
            DeleteSynchEvent(e);
        }
    }

// Return a refcounted reference to the SynchEvent of the object at address
// "addr", if any.  The pointer returned is valid until the UnrefSynchEvent() is
// called.
    static SynchEvent *GetSynchEvent(const void *addr) {
        uint32_t h = reinterpret_cast<intptr_t>(addr) % kNSynchEvent;
        SynchEvent *e;
        synch_event_mu.lock();
        for (e = synch_event[h];
             e != nullptr && e->masked_addr != hide_ptr(addr);
             e = e->next) {
        }
        if (e != nullptr) {
            e->refcount++;
        }
        synch_event_mu.unlock();
        return e;
    }

// Called when an event "ev" occurs on a mutex of cond_var "obj"
// if event recording is on
    static void PostSynchEvent(void *obj, int ev) {
        SynchEvent *e = GetSynchEvent(obj);
        // logging is on if event recording is on and either there's no event struct,
        // or it explicitly says to log
        if (e == nullptr || e->log) {
            void *pcs[40];
            int n = abel::GetStackTrace(pcs, ABEL_ARRAYSIZE(pcs), 1);
            // A buffer with enough space for the ASCII for all the PCs, even on a
            // 64-bit machine.
            char buffer[ABEL_ARRAYSIZE(pcs) * 24];
            int pos = snprintf(buffer, sizeof(buffer), " @");
            for (int i = 0; i != n; i++) {
                pos += snprintf(&buffer[pos], sizeof(buffer) - pos, " %p", pcs[i]);
            }
            ABEL_RAW_LOG(INFO, "%s%p %s %s", event_properties[ev].msg, obj,
                         (e == nullptr ? "" : e->name), buffer);
        }
        const int flags = event_properties[ev].flags;
        if ((flags & SYNCH_F_LCK) != 0 && e != nullptr && e->invariant != nullptr) {
            // Calling the invariant as is causes problems under ThreadSanitizer.
            // We are currently inside of mutex lock/unlock and are ignoring all
            // memory accesses and synchronization. If the invariant transitively
            // synchronizes something else and we ignore the synchronization, we will
            // get false positive race reports later.
            // Reuse EvalConditionAnnotated to properly call into user code.
            struct local {
                static bool pred(SynchEvent *ev) {
                    (*ev->invariant)(ev->arg);
                    return false;
                }
            };
            condition cond(&local::pred, e);
            mutex *mu = static_cast<mutex *>(obj);
            const bool locking = (flags & SYNCH_F_UNLOCK) == 0;
            const bool trylock = (flags & SYNCH_F_TRY) != 0;
            const bool read_lock = (flags & SYNCH_F_R) != 0;
            EvalConditionAnnotated(&cond, mu, locking, trylock, read_lock);
        }
        UnrefSynchEvent(e);
    }

//------------------------------------------------------------------

// The synch_wait_params struct encapsulates the way in which a thread is waiting:
// whether it has a timeout, the condition, exclusive/shared, and whether a
// condition variable wait has an associated mutex (as opposed to another
// type of lock).  It also points to the PerThreadSynch struct of its thread.
// cv_word tells Enqueue() to enqueue on a cond_var using CondVarEnqueue().
//
// This structure is held on the stack rather than directly in
// PerThreadSynch because a thread can be waiting on multiple Mutexes if,
// while waiting on one mutex, the implementation calls a client callback
// (such as a condition function) that acquires another mutex. We don't
// strictly need to allow this, but programmers become confused if we do not
// allow them to use functions such a LOG() within condition functions.  The
// PerThreadSynch struct points at the most recent synch_wait_params struct when
// the thread is on a mutex's waiter queue.
    struct synch_wait_params {
        synch_wait_params(mutex::MuHow how_arg, const condition *cond_arg,
                          KernelTimeout timeout_arg, mutex *cvmu_arg,
                          PerThreadSynch *thread_arg,
                          std::atomic<intptr_t> *cv_word_arg)
                : how(how_arg),
                  cond(cond_arg),
                  timeout(timeout_arg),
                  cvmu(cvmu_arg),
                  thread(thread_arg),
                  cv_word(cv_word_arg),
                  contention_start_cycles(abel::chrono_internal::cycle_clock::now()) {}

        const mutex::MuHow how;  // How this thread needs to wait.
        const condition *cond;  // The condition that this thread is waiting for.
        // In mutex, this field is set to zero if a timeout
        // expires.
        KernelTimeout timeout;  // timeout expiry---absolute time
        // In mutex, this field is set to zero if a timeout
        // expires.
        mutex *const cvmu;      // used for transfer from cond var to mutex
        PerThreadSynch *const thread;  // thread that is waiting

        // If not null, thread should be enqueued on the cond_var whose state
        // word is cv_word instead of queueing normally on the mutex.
        std::atomic<intptr_t> *cv_word;

        int64_t contention_start_cycles;  // abel_time (in cycles) when this thread started
        // to contend for the mutex.
    };

    struct SynchLocksHeld {
        int n;              // number of valid entries in locks[]
        bool overflow;      // true iff we overflowed the array at some point
        struct {
            mutex *mu;        // lock acquired
            int32_t count;      // times acquired
            GraphId id;       // deadlock_graph id of acquired lock
        } locks[40];
        // If a thread overfills the array during deadlock detection, we
        // continue, discarding information as needed.  If no overflow has
        // taken place, we can provide more error checking, such as
        // detecting when a thread releases a lock it does not hold.
    };

// A sentinel value in lists that is not 0.
// A 0 value is used to mean "not on a list".
    static PerThreadSynch *const kPerThreadSynchNull =
            reinterpret_cast<PerThreadSynch *>(1);

    static SynchLocksHeld *LocksHeldAlloc() {
        SynchLocksHeld *ret = reinterpret_cast<SynchLocksHeld *>(
                memory_internal::LowLevelAlloc::Alloc(sizeof(SynchLocksHeld)));
        ret->n = 0;
        ret->overflow = false;
        return ret;
    }

// Return the PerThreadSynch-struct for this thread.
    static PerThreadSynch *Synch_GetPerThread() {
        ThreadIdentity *identity = GetOrCreateCurrentThreadIdentity();
        return &identity->per_thread_synch;
    }

    static PerThreadSynch *Synch_GetPerThreadAnnotated(mutex *mu) {
        if (mu) {
            ABEL_TSAN_MUTEX_PRE_DIVERT(mu, 0);
        }
        PerThreadSynch *w = Synch_GetPerThread();
        if (mu) {
            ABEL_TSAN_MUTEX_POST_DIVERT(mu, 0);
        }
        return w;
    }

    static SynchLocksHeld *Synch_GetAllLocks() {
        PerThreadSynch *s = Synch_GetPerThread();
        if (s->all_locks == nullptr) {
            s->all_locks = LocksHeldAlloc();  // Freed by ReclaimThreadIdentity.
        }
        return s->all_locks;
    }

// Post on "w"'s associated PerThreadSem.
    ABEL_FORCE_INLINE void mutex::IncrementSynchSem(mutex *mu, PerThreadSynch *w) {
        if (mu) {
            ABEL_TSAN_MUTEX_PRE_DIVERT(mu, 0);
        }
        PerThreadSem::Post(w->thread_identity());
        if (mu) {
            ABEL_TSAN_MUTEX_POST_DIVERT(mu, 0);
        }
    }

// wait on "w"'s associated PerThreadSem; returns false if timeout expired.
    bool mutex::DecrementSynchSem(mutex *mu, PerThreadSynch *w, KernelTimeout t) {
        if (mu) {
            ABEL_TSAN_MUTEX_PRE_DIVERT(mu, 0);
        }
        assert(w == Synch_GetPerThread());
        static_cast<void>(w);
        bool res = PerThreadSem::wait(t);
        if (mu) {
            ABEL_TSAN_MUTEX_POST_DIVERT(mu, 0);
        }
        return res;
    }

// We're in a fatal signal handler that hopes to use mutex and to get
// lucky by not deadlocking.  We try to improve its chances of success
// by effectively disabling some of the consistency checks.  This will
// prevent certain ABEL_RAW_CHECK() statements from being triggered when
// re-rentry is detected.  The ABEL_RAW_CHECK() statements are those in the
// mutex code checking that the "waitp" field has not been reused.
    void mutex::InternalAttemptToUseMutexInFatalSignalHandler() {
        // Fix the per-thread state only if it exists.
        ThreadIdentity *identity = CurrentThreadIdentityIfPresent();
        if (identity != nullptr) {
            identity->per_thread_synch.suppress_fatal_errors = true;
        }
        // Don't do deadlock detection when we are already failing.
        synch_deadlock_detection.store(on_deadlock_cycle::kIgnore,
                                       std::memory_order_release);
    }

// --------------------------time support

// Return the current time plus the timeout.  Use the same clock as
// PerThreadSem::wait() for consistency.  Unfortunately, we don't have
// such a choice when a deadline is given directly.
    static abel::abel_time DeadlineFromTimeout(abel::duration timeout) {
#ifndef _WIN32
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return abel::time_from_timeval(tv) + timeout;
#else
        return abel::now() + timeout;
#endif
    }

// --------------------------Mutexes

// In the layout below, the msb of the bottom byte is currently unused.  Also,
// the following constraints were considered in choosing the layout:
//  o Both the debug allocator's "uninitialized" and "freed" patterns (0xab and
//    0xcd) are illegal: reader and writer lock both held.
//  o kMuWriter and kMuEvent should exceed kMuDesig and kMuWait, to enable the
//    bit-twiddling trick in mutex::unlock().
//  o kMuWriter / kMuReader == kMuWrWait / kMuWait,
//    to enable the bit-twiddling trick in CheckForMutexCorruption().
    static const intptr_t kMuReader = 0x0001L;  // a reader holds the lock
    static const intptr_t kMuDesig = 0x0002L;  // there's a designated waker
    static const intptr_t kMuWait = 0x0004L;  // threads are waiting
    static const intptr_t kMuWriter = 0x0008L;  // a writer holds the lock
    static const intptr_t kMuEvent = 0x0010L;  // record this mutex's events
// INVARIANT1:  there's a thread that was blocked on the mutex, is
// no longer, yet has not yet acquired the mutex.  If there's a
// designated waker, all threads can avoid taking the slow path in
// unlock because the designated waker will subsequently acquire
// the lock and wake someone.  To maintain INVARIANT1 the bit is
// set when a thread is unblocked(INV1a), and threads that were
// unblocked reset the bit when they either acquire or re-block
// (INV1b).
    static const intptr_t kMuWrWait = 0x0020L;  // runnable writer is waiting
// for a reader
    static const intptr_t kMuSpin = 0x0040L;  // spinlock protects wait list
    static const intptr_t kMuLow = 0x00ffL;  // mask all mutex bits
    static const intptr_t kMuHigh = ~kMuLow;  // mask pointer/reader count

// Hack to make constant values available to gdb pretty printer
    enum {
        kGdbMuSpin = kMuSpin,
        kGdbMuEvent = kMuEvent,
        kGdbMuWait = kMuWait,
        kGdbMuWriter = kMuWriter,
        kGdbMuDesig = kMuDesig,
        kGdbMuWrWait = kMuWrWait,
        kGdbMuReader = kMuReader,
        kGdbMuLow = kMuLow,
    };

// kMuWrWait implies kMuWait.
// kMuReader and kMuWriter are mutually exclusive.
// If kMuReader is zero, there are no readers.
// Otherwise, if kMuWait is zero, the high order bits contain a count of the
// number of readers.  Otherwise, the reader count is held in
// PerThreadSynch::readers of the most recently queued waiter, again in the
// bits above kMuLow.
    static const intptr_t kMuOne = 0x0100;  // a count of one reader

// flags passed to Enqueue and LockSlow{,WithTimeout,Loop}
    static const int kMuHasBlocked = 0x01;  // already blocked (MUST == 1)
    static const int kMuIsCond = 0x02;      // conditional waiter (CV or condition)

    static_assert(PerThreadSynch::kAlignment > kMuLow,
                  "PerThreadSynch::kAlignment must be greater than kMuLow");

// This struct contains various bitmasks to be used in
// acquiring and releasing a mutex in a particular mode.
    struct MuHowS {
        // if all the bits in fast_need_zero are zero, the lock can be acquired by
        // adding fast_add and oring fast_or.  The bit kMuDesig should be reset iff
        // this is the designated waker.
        intptr_t fast_need_zero;
        intptr_t fast_or;
        intptr_t fast_add;

        intptr_t slow_need_zero;  // fast_need_zero with events (e.g. logging)

        intptr_t slow_inc_need_zero;  // if all the bits in slow_inc_need_zero are
        // zero a reader can acquire a read share by
        // setting the reader bit and incrementing
        // the reader count (in last waiter since
        // we're now slow-path).  kMuWrWait be may
        // be ignored if we already waited once.
    };

    static const MuHowS kSharedS = {
            // shared or read lock
            kMuWriter | kMuWait | kMuEvent,   // fast_need_zero
            kMuReader,                        // fast_or
            kMuOne,                           // fast_add
            kMuWriter | kMuWait,              // slow_need_zero
            kMuSpin | kMuWriter | kMuWrWait,  // slow_inc_need_zero
    };
    static const MuHowS kExclusiveS = {
            // exclusive or write lock
            kMuWriter | kMuReader | kMuEvent,  // fast_need_zero
            kMuWriter,                         // fast_or
            0,                                 // fast_add
            kMuWriter | kMuReader,             // slow_need_zero
            ~static_cast<intptr_t>(0),         // slow_inc_need_zero
    };
    static const mutex::MuHow kShared = &kSharedS;        // shared lock
    static const mutex::MuHow kExclusive = &kExclusiveS;  // exclusive lock

#ifdef NDEBUG
    static constexpr bool kDebugMode = false;
#else
    static constexpr bool kDebugMode = true;
#endif

#ifdef THREAD_SANITIZER
    static unsigned TsanFlags(mutex::MuHow how) {
      return how == kShared ? __tsan_mutex_read_lock : 0;
    }
#endif

    static bool DebugOnlyIsExiting() {
        return false;
    }

    mutex::~mutex() {
        intptr_t v = mu_.load(std::memory_order_relaxed);
        if ((v & kMuEvent) != 0 && !DebugOnlyIsExiting()) {
            ForgetSynchEvent(&this->mu_, kMuEvent, kMuSpin);
        }
        if (kDebugMode) {
            this->ForgetDeadlockInfo();
        }
        ABEL_TSAN_MUTEX_DESTROY(this, __tsan_mutex_not_static);
    }

    void mutex::enable_debug_log(const char *name) {
        SynchEvent *e = EnsureSynchEvent(&this->mu_, name, kMuEvent, kMuSpin);
        e->log = true;
        UnrefSynchEvent(e);
    }

    void enable_mutex_invariant_debugging(bool enabled) {
        synch_check_invariants.store(enabled, std::memory_order_release);
    }

    void mutex::EnableInvariantDebugging(void (*invariant)(void *),
                                         void *arg) {
        if (synch_check_invariants.load(std::memory_order_acquire) &&
            invariant != nullptr) {
            SynchEvent *e = EnsureSynchEvent(&this->mu_, nullptr, kMuEvent, kMuSpin);
            e->invariant = invariant;
            e->arg = arg;
            UnrefSynchEvent(e);
        }
    }

    void set_mutex_deadlock_detection_mode(on_deadlock_cycle mode) {
        synch_deadlock_detection.store(mode, std::memory_order_release);
    }

// Return true iff threads x and y are waiting on the same condition for the
// same type of lock.  Requires that x and y be waiting on the same mutex
// queue.
    static bool MuSameCondition(PerThreadSynch *x, PerThreadSynch *y) {
        return x->waitp->how == y->waitp->how &&
               condition::guaranteed_equal(x->waitp->cond, y->waitp->cond);
    }

// Given the contents of a mutex word containing a PerThreadSynch pointer,
// return the pointer.
    static ABEL_FORCE_INLINE PerThreadSynch *GetPerThreadSynch(intptr_t v) {
        return reinterpret_cast<PerThreadSynch *>(v & kMuHigh);
    }

// The next several routines maintain the per-thread next and skip fields
// used in the mutex waiter queue.
// The queue is a circular singly-linked list, of which the "head" is the
// last element, and head->next if the first element.
// The skip field has the invariant:
//   For thread x, x->skip is one of:
//     - invalid (iff x is not in a mutex wait queue),
//     - null, or
//     - a pointer to a distinct thread waiting later in the same mutex queue
//       such that all threads in [x, x->skip] have the same condition and
//       lock type (MuSameCondition() is true for all pairs in [x, x->skip]).
// In addition, if x->skip is  valid, (x->may_skip || x->skip == null)
//
// By the spec of MuSameCondition(), it is not necessary when removing the
// first runnable thread y from the front a mutex queue to adjust the skip
// field of another thread x because if x->skip==y, x->skip must (have) become
// invalid before y is removed.  The function TryRemove can remove a specified
// thread from an arbitrary position in the queue whether runnable or not, so
// it fixes up skip fields that would otherwise be left dangling.
// The statement
//     if (x->may_skip && MuSameCondition(x, x->next)) { x->skip = x->next; }
// maintains the invariant provided x is not the last waiter in a mutex queue
// The statement
//          if (x->skip != null) { x->skip = x->skip->skip; }
// maintains the invariant.

// Returns the last thread y in a mutex waiter queue such that all threads in
// [x, y] inclusive share the same condition.  Sets skip fields of some threads
// in that range to optimize future evaluation of Skip() on x values in
// the range.  Requires thread x is in a mutex waiter queue.
// The locking is unusual.  Skip() is called under these conditions:
//   - spinlock is held in call from Enqueue(), with maybe_unlocking == false
//   - mutex is held in call from UnlockSlow() by last unlocker, with
//     maybe_unlocking == true
//   - both mutex and spinlock are held in call from DequeueAllWakeable() (from
//     UnlockSlow()) and TryRemove()
// These cases are mutually exclusive, so Skip() never runs concurrently
// with itself on the same mutex.   The skip chain is used in these other places
// that cannot occur concurrently:
//   - FixSkip() (from TryRemove()) - spinlock and mutex are held)
//   - Dequeue() (with spinlock and mutex held)
//   - UnlockSlow() (with spinlock and mutex held)
// A more complex case is Enqueue()
//   - Enqueue() (with spinlock held and maybe_unlocking == false)
//               This is the first case in which Skip is called, above.
//   - Enqueue() (without spinlock held; but queue is empty and being freshly
//                formed)
//   - Enqueue() (with spinlock held and maybe_unlocking == true)
// The first case has mutual exclusion, and the second isolation through
// working on an otherwise unreachable data structure.
// In the last case, Enqueue() is required to change no skip/next pointers
// except those in the added node and the former "head" node.  This implies
// that the new node is added after head, and so must be the new head or the
// new front of the queue.
    static PerThreadSynch *Skip(PerThreadSynch *x) {
        PerThreadSynch *x0 = nullptr;
        PerThreadSynch *x1 = x;
        PerThreadSynch *x2 = x->skip;
        if (x2 != nullptr) {
            // Each iteration attempts to advance sequence (x0,x1,x2) to next sequence
            // such that   x1 == x0->skip && x2 == x1->skip
            while ((x0 = x1, x1 = x2, x2 = x2->skip) != nullptr) {
                x0->skip = x2;      // short-circuit skip from x0 to x2
            }
            x->skip = x1;         // short-circuit skip from x to result
        }
        return x1;
    }

// "ancestor" appears before "to_be_removed" in the same mutex waiter queue.
// The latter is going to be removed out of order, because of a timeout.
// Check whether "ancestor" has a skip field pointing to "to_be_removed",
// and fix it if it does.
    static void FixSkip(PerThreadSynch *ancestor, PerThreadSynch *to_be_removed) {
        if (ancestor->skip == to_be_removed) {  // ancestor->skip left dangling
            if (to_be_removed->skip != nullptr) {
                ancestor->skip = to_be_removed->skip;  // can skip past to_be_removed
            } else if (ancestor->next != to_be_removed) {  // they are not adjacent
                ancestor->skip = ancestor->next;             // can skip one past ancestor
            } else {
                ancestor->skip = nullptr;  // can't skip at all
            }
        }
    }

    static void CondVarEnqueue(synch_wait_params *waitp);

// Enqueue thread "waitp->thread" on a waiter queue.
// Called with mutex spinlock held if head != nullptr
// If head==nullptr and waitp->cv_word==nullptr, then Enqueue() is
// idempotent; it alters no state associated with the existing (empty)
// queue.
//
// If waitp->cv_word == nullptr, queue the thread at either the front or
// the end (according to its priority) of the circular mutex waiter queue whose
// head is "head", and return the new head.  mu is the previous mutex state,
// which contains the reader count (perhaps adjusted for the operation in
// progress) if the list was empty and a read lock held, and the holder hint if
// the list was empty and a write lock held.  (flags & kMuIsCond) indicates
// whether this thread was transferred from a cond_var or is waiting for a
// non-trivial condition.  In this case, Enqueue() never returns nullptr
//
// If waitp->cv_word != nullptr, CondVarEnqueue() is called, and "head" is
// returned. This mechanism is used by cond_var to queue a thread on the
// condition variable queue instead of the mutex queue in implementing wait().
// In this case, Enqueue() can return nullptr (if head==nullptr).
    static PerThreadSynch *Enqueue(PerThreadSynch *head,
                                   synch_wait_params *waitp, intptr_t mu, int flags) {
        // If we have been given a cv_word, call CondVarEnqueue() and return
        // the previous head of the mutex waiter queue.
        if (waitp->cv_word != nullptr) {
            CondVarEnqueue(waitp);
            return head;
        }

        PerThreadSynch *s = waitp->thread;
        ABEL_RAW_CHECK(
                s->waitp == nullptr ||    // normal case
                s->waitp == waitp ||  // Fer()---transfer from condition variable
                s->suppress_fatal_errors,
                "detected illegal recursion into mutex code");
        s->waitp = waitp;
        s->skip = nullptr;             // maintain skip invariant (see above)
        s->may_skip = true;            // always true on entering queue
        s->wake = false;               // not being woken
        s->cond_waiter = ((flags & kMuIsCond) != 0);
        if (head == nullptr) {         // s is the only waiter
            s->next = s;                 // it's the only entry in the cycle
            s->readers = mu;             // reader count is from mu word
            s->maybe_unlocking = false;  // no one is searching an empty list
            head = s;                    // s is new head
        } else {
            PerThreadSynch *enqueue_after = nullptr;  // we'll put s after this element
#ifdef ABEL_HAVE_PTHREAD_GETSCHEDPARAM
            int64_t now_cycles = chrono_internal::cycle_clock::now();
            if (s->next_priority_read_cycles < now_cycles) {
                // Every so often, update our idea of the thread's priority.
                // pthread_getschedparam() is 5% of the block/wakeup time;
                // base_internal::cycle_clock::now() is 0.5%.
                int policy;
                struct sched_param param;
                const int err = pthread_getschedparam(pthread_self(), &policy, &param);
                if (err != 0) {
                    ABEL_RAW_LOG(ERROR, "pthread_getschedparam failed: %d", err);
                } else {
                    s->priority = param.sched_priority;
                    s->next_priority_read_cycles =
                            now_cycles +
                            static_cast<int64_t>(chrono_internal::cycle_clock::frequency());
                }
            }
            if (s->priority > head->priority) {  // s's priority is above head's
                // try to put s in priority-fifo order, or failing that at the front.
                if (!head->maybe_unlocking) {
                    // No unlocker can be scanning the queue, so we can insert between
                    // skip-chains, and within a skip-chain if it has the same condition as
                    // s.  We insert in priority-fifo order, examining the end of every
                    // skip-chain, plus every element with the same condition as s.
                    PerThreadSynch *advance_to = head;    // next value of enqueue_after
                    PerThreadSynch *cur;                  // successor of enqueue_after
                    do {
                        enqueue_after = advance_to;
                        cur = enqueue_after->next;  // this advance ensures progress
                        advance_to = Skip(cur);   // normally, advance to end of skip chain
                        // (side-effect: optimizes skip chain)
                        if (advance_to != cur && s->priority > advance_to->priority &&
                            MuSameCondition(s, cur)) {
                            // but this skip chain is not a singleton, s has higher priority
                            // than its tail and has the same condition as the chain,
                            // so we can insert within the skip-chain
                            advance_to = cur;         // advance by just one
                        }
                    } while (s->priority <= advance_to->priority);
                    // termination guaranteed because s->priority > head->priority
                    // and head is the end of a skip chain
                } else if (waitp->how == kExclusive &&
                           condition::guaranteed_equal(waitp->cond, nullptr)) {
                    // An unlocker could be scanning the queue, but we know it will recheck
                    // the queue front for writers that have no condition, which is what s
                    // is, so an insert at front is safe.
                    enqueue_after = head;       // add after head, at front
                }
            }
#endif
            if (enqueue_after != nullptr) {
                s->next = enqueue_after->next;
                enqueue_after->next = s;

                // enqueue_after can be: head, Skip(...), or cur.
                // The first two imply enqueue_after->skip == nullptr, and
                // the last is used only if MuSameCondition(s, cur).
                // We require this because clearing enqueue_after->skip
                // is impossible; enqueue_after's predecessors might also
                // incorrectly skip over s if we were to allow other
                // insertion points.
                ABEL_RAW_CHECK(
                        enqueue_after->skip == nullptr || MuSameCondition(enqueue_after, s),
                        "mutex Enqueue failure");

                if (enqueue_after != head && enqueue_after->may_skip &&
                    MuSameCondition(enqueue_after, enqueue_after->next)) {
                    // enqueue_after can skip to its new successor, s
                    enqueue_after->skip = enqueue_after->next;
                }
                if (MuSameCondition(s, s->next)) {  // s->may_skip is known to be true
                    s->skip = s->next;                // s may skip to its successor
                }
            } else {   // enqueue not done any other way, so
                // we're inserting s at the back
                // s will become new head; copy data from head into it
                s->next = head->next;        // add s after head
                head->next = s;
                s->readers = head->readers;  // reader count is from previous head
                s->maybe_unlocking = head->maybe_unlocking;  // same for unlock hint
                if (head->may_skip && MuSameCondition(head, s)) {
                    // head now has successor; may skip
                    head->skip = s;
                }
                head = s;  // s is new head
            }
        }
        s->state.store(PerThreadSynch::kQueued, std::memory_order_relaxed);
        return head;
    }

// Dequeue the successor pw->next of thread pw from the mutex waiter queue
// whose last element is head.  The new head element is returned, or null
// if the list is made empty.
// Dequeue is called with both spinlock and mutex held.
    static PerThreadSynch *Dequeue(PerThreadSynch *head, PerThreadSynch *pw) {
        PerThreadSynch *w = pw->next;
        pw->next = w->next;         // snip w out of list
        if (head == w) {            // we removed the head
            head = (pw == w) ? nullptr : pw;  // either emptied list, or pw is new head
        } else if (pw != head && MuSameCondition(pw, pw->next)) {
            // pw can skip to its new successor
            if (pw->next->skip !=
                nullptr) {  // either skip to its successors skip target
                pw->skip = pw->next->skip;
            } else {                   // or to pw's successor
                pw->skip = pw->next;
            }
        }
        return head;
    }

// Traverse the elements [ pw->next, h] of the circular list whose last element
// is head.
// Remove all elements with wake==true and place them in the
// singly-linked list wake_list in the order found.   Assumes that
// there is only one such element if the element has how == kExclusive.
// Return the new head.
    static PerThreadSynch *DequeueAllWakeable(PerThreadSynch *head,
                                              PerThreadSynch *pw,
                                              PerThreadSynch **wake_tail) {
        PerThreadSynch *orig_h = head;
        PerThreadSynch *w = pw->next;
        bool skipped = false;
        do {
            if (w->wake) {                    // remove this element
                ABEL_RAW_CHECK(pw->skip == nullptr, "bad skip in DequeueAllWakeable");
                // we're removing pw's successor so either pw->skip is zero or we should
                // already have removed pw since if pw->skip!=null, pw has the same
                // condition as w.
                head = Dequeue(head, pw);
                w->next = *wake_tail;           // keep list terminated
                *wake_tail = w;                 // add w to wake_list;
                wake_tail = &w->next;           // next addition to end
                if (w->waitp->how == kExclusive) {  // wake at most 1 writer
                    break;
                }
            } else {                // not waking this one; skip
                pw = Skip(w);       // skip as much as possible
                skipped = true;
            }
            w = pw->next;
            // We want to stop processing after we've considered the original head,
            // orig_h.  We can't test for w==orig_h in the loop because w may skip over
            // it; we are guaranteed only that w's predecessor will not skip over
            // orig_h.  When we've considered orig_h, either we've processed it and
            // removed it (so orig_h != head), or we considered it and skipped it (so
            // skipped==true && pw == head because skipping from head always skips by
            // just one, leaving pw pointing at head).  So we want to
            // continue the loop with the negation of that expression.
        } while (orig_h == head && (pw != head || !skipped));
        return head;
    }

// Try to remove thread s from the list of waiters on this mutex.
// Does nothing if s is not on the waiter list.
    void mutex::TryRemove(PerThreadSynch *s) {
        intptr_t v = mu_.load(std::memory_order_relaxed);
        // acquire spinlock & lock
        if ((v & (kMuWait | kMuSpin | kMuWriter | kMuReader)) == kMuWait &&
            mu_.compare_exchange_strong(v, v | kMuSpin | kMuWriter,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
            PerThreadSynch *h = GetPerThreadSynch(v);
            if (h != nullptr) {
                PerThreadSynch *pw = h;   // pw is w's predecessor
                PerThreadSynch *w;
                if ((w = pw->next) != s) {  // search for thread,
                    do {                      // processing at least one element
                        if (!MuSameCondition(s, w)) {  // seeking different condition
                            pw = Skip(w);                // so skip all that won't match
                            // we don't have to worry about dangling skip fields
                            // in the threads we skipped; none can point to s
                            // because their condition differs from s
                        } else {          // seeking same condition
                            FixSkip(w, s);  // fix up any skip pointer from w to s
                            pw = w;
                        }
                        // don't search further if we found the thread, or we're about to
                        // process the first thread again.
                    } while ((w = pw->next) != s && pw != h);
                }
                if (w == s) {                 // found thread; remove it
                    // pw->skip may be non-zero here; the loop above ensured that
                    // no ancestor of s can skip to s, so removal is safe anyway.
                    h = Dequeue(h, pw);
                    s->next = nullptr;
                    s->state.store(PerThreadSynch::kAvailable, std::memory_order_release);
                }
            }
            intptr_t nv;
            do {                        // release spinlock and lock
                v = mu_.load(std::memory_order_relaxed);
                nv = v & (kMuDesig | kMuEvent);
                if (h != nullptr) {
                    nv |= kMuWait | reinterpret_cast<intptr_t>(h);
                    h->readers = 0;            // we hold writer lock
                    h->maybe_unlocking = false;  // finished unlocking
                }
            } while (!mu_.compare_exchange_weak(v, nv,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
        }
    }

// wait until thread "s", which must be the current thread, is removed from the
// this mutex's waiter queue.  If "s->waitp->timeout" has a timeout, wake up
// if the wait extends past the absolute time specified, even if "s" is still
// on the mutex queue.  In this case, remove "s" from the queue and return
// true, otherwise return false.
    ABEL_XRAY_LOG_ARGS(1)

    void mutex::Block(PerThreadSynch *s) {
        while (s->state.load(std::memory_order_acquire) == PerThreadSynch::kQueued) {
            if (!DecrementSynchSem(this, s, s->waitp->timeout)) {
                // After a timeout, we go into a spin loop until we remove ourselves
                // from the queue, or someone else removes us.  We can't be sure to be
                // able to remove ourselves in a single lock acquisition because this
                // mutex may be held, and the holder has the right to read the centre
                // of the waiter queue without holding the spinlock.
                this->TryRemove(s);
                int c = 0;
                while (s->next != nullptr) {
                    c = Delay(c, GENTLE);
                    this->TryRemove(s);
                }
                if (kDebugMode) {
                    // This ensures that we test the case that TryRemove() is called when s
                    // is not on the queue.
                    this->TryRemove(s);
                }
                s->waitp->timeout = KernelTimeout::Never();      // timeout is satisfied
                s->waitp->cond = nullptr;  // condition no longer relevant for wakeups
            }
        }
        ABEL_RAW_CHECK(s->waitp != nullptr || s->suppress_fatal_errors,
                       "detected illegal recursion in mutex code");
        s->waitp = nullptr;
    }

// Wake thread w, and return the next thread in the list.
    PerThreadSynch *mutex::Wakeup(PerThreadSynch *w) {
        PerThreadSynch *next = w->next;
        w->next = nullptr;
        w->state.store(PerThreadSynch::kAvailable, std::memory_order_release);
        IncrementSynchSem(this, w);

        return next;
    }

    static GraphId GetGraphIdLocked(mutex *mu)
    ABEL_EXCLUSIVE_LOCKS_REQUIRED(deadlock_graph_mu) {
        if (!deadlock_graph) {  // (re)create the deadlock graph.
            deadlock_graph =
                    new(memory_internal::LowLevelAlloc::Alloc(sizeof(*deadlock_graph)))
                            GraphCycles;
        }
        return deadlock_graph->GetId(mu);
    }

    static GraphId GetGraphId(mutex *mu) ABEL_LOCKS_EXCLUDED(deadlock_graph_mu) {
        deadlock_graph_mu.lock();
        GraphId id = GetGraphIdLocked(mu);
        deadlock_graph_mu.unlock();
        return id;
    }

// Record a lock acquisition.  This is used in debug mode for deadlock
// detection.  The held_locks pointer points to the relevant data
// structure for each case.
    static void LockEnter(mutex *mu, GraphId id, SynchLocksHeld *held_locks) {
        int n = held_locks->n;
        int i = 0;
        while (i != n && held_locks->locks[i].id != id) {
            i++;
        }
        if (i == n) {
            if (n == ABEL_ARRAYSIZE(held_locks->locks)) {
                held_locks->overflow = true;  // lost some data
            } else {                        // we have room for lock
                held_locks->locks[i].mu = mu;
                held_locks->locks[i].count = 1;
                held_locks->locks[i].id = id;
                held_locks->n = n + 1;
            }
        } else {
            held_locks->locks[i].count++;
        }
    }

// Record a lock release.  Each call to LockEnter(mu, id, x) should be
// eventually followed by a call to LockLeave(mu, id, x) by the same thread.
// It does not process the event if is not needed when deadlock detection is
// disabled.
    static void LockLeave(mutex *mu, GraphId id, SynchLocksHeld *held_locks) {
        int n = held_locks->n;
        int i = 0;
        while (i != n && held_locks->locks[i].id != id) {
            i++;
        }
        if (i == n) {
            if (!held_locks->overflow) {
                // The deadlock id may have been reassigned after ForgetDeadlockInfo,
                // but in that case mu should still be present.
                i = 0;
                while (i != n && held_locks->locks[i].mu != mu) {
                    i++;
                }
                if (i == n) {  // mu missing means releasing unheld lock
                    SynchEvent *mu_events = GetSynchEvent(mu);
                    ABEL_RAW_LOG(FATAL,
                                 "thread releasing lock it does not hold: %p %s; ",
                                 static_cast<void *>(mu),
                                 mu_events == nullptr ? "" : mu_events->name);
                }
            }
        } else if (held_locks->locks[i].count == 1) {
            held_locks->n = n - 1;
            held_locks->locks[i] = held_locks->locks[n - 1];
            held_locks->locks[n - 1].id = InvalidGraphId();
            held_locks->locks[n - 1].mu =
                    nullptr;  // clear mu to please the leak detector.
        } else {
            assert(held_locks->locks[i].count > 0);
            held_locks->locks[i].count--;
        }
    }

// Call LockEnter() if in debug mode and deadlock detection is enabled.
    static ABEL_FORCE_INLINE void DebugOnlyLockEnter(mutex *mu) {
        if (kDebugMode) {
            if (synch_deadlock_detection.load(std::memory_order_acquire) !=
                on_deadlock_cycle::kIgnore) {
                LockEnter(mu, GetGraphId(mu), Synch_GetAllLocks());
            }
        }
    }

// Call LockEnter() if in debug mode and deadlock detection is enabled.
    static ABEL_FORCE_INLINE void DebugOnlyLockEnter(mutex *mu, GraphId id) {
        if (kDebugMode) {
            if (synch_deadlock_detection.load(std::memory_order_acquire) !=
                on_deadlock_cycle::kIgnore) {
                LockEnter(mu, id, Synch_GetAllLocks());
            }
        }
    }

// Call LockLeave() if in debug mode and deadlock detection is enabled.
    static ABEL_FORCE_INLINE void DebugOnlyLockLeave(mutex *mu) {
        if (kDebugMode) {
            if (synch_deadlock_detection.load(std::memory_order_acquire) !=
                on_deadlock_cycle::kIgnore) {
                LockLeave(mu, GetGraphId(mu), Synch_GetAllLocks());
            }
        }
    }

    static char *StackString(void **pcs, int n, char *buf, int maxlen,
                             bool symbolize) {
        static const int kSymLen = 200;
        char sym[kSymLen];
        int len = 0;
        for (int i = 0; i != n; i++) {
            if (symbolize) {
                if (!symbolizer(pcs[i], sym, kSymLen)) {
                    sym[0] = '\0';
                }
                snprintf(buf + len, maxlen - len, "%s\t@ %p %s\n",
                         (i == 0 ? "\n" : ""),
                         pcs[i], sym);
            } else {
                snprintf(buf + len, maxlen - len, " %p", pcs[i]);
            }
            len += strlen(&buf[len]);
        }
        return buf;
    }

    static char *CurrentStackString(char *buf, int maxlen, bool symbolize) {
        void *pcs[40];
        return StackString(pcs, abel::GetStackTrace(pcs, ABEL_ARRAYSIZE(pcs), 2), buf,
                           maxlen, symbolize);
    }

    namespace {
        enum {
            kMaxDeadlockPathLen = 10
        };  // maximum length of a deadlock cycle;
// a path this long would be remarkable
// Buffers required to report a deadlock.
// We do not allocate them on stack to avoid large stack frame.
        struct DeadlockReportBuffers {
            char buf[6100];
            GraphId path[kMaxDeadlockPathLen];
        };

        struct ScopedDeadlockReportBuffers {
            ScopedDeadlockReportBuffers() {
                b = reinterpret_cast<DeadlockReportBuffers *>(
                        memory_internal::LowLevelAlloc::Alloc(sizeof(*b)));
            }

            ~ScopedDeadlockReportBuffers() { memory_internal::LowLevelAlloc::Free(b); }

            DeadlockReportBuffers *b;
        };

// Helper to pass to GraphCycles::UpdateStackTrace.
        int GetStack(void **stack, int max_depth) {
            return abel::GetStackTrace(stack, max_depth, 3);
        }
    }  // anonymous namespace

// Called in debug mode when a thread is about to acquire a lock in a way that
// may block.
    static GraphId DeadlockCheck(mutex *mu) {
        if (synch_deadlock_detection.load(std::memory_order_acquire) ==
            on_deadlock_cycle::kIgnore) {
            return InvalidGraphId();
        }

        SynchLocksHeld *all_locks = Synch_GetAllLocks();

        abel::threading_internal::SpinLockHolder lock(&deadlock_graph_mu);
        const GraphId mu_id = GetGraphIdLocked(mu);

        if (all_locks->n == 0) {
            // There are no other locks held. Return now so that we don't need to
            // call GetSynchEvent(). This way we do not record the stack trace
            // for this mutex. It's ok, since if this mutex is involved in a deadlock,
            // it can't always be the first lock acquired by a thread.
            return mu_id;
        }

        // We prefer to keep stack traces that show a thread holding and acquiring
        // as many locks as possible.  This increases the chances that a given edge
        // in the acquires-before graph will be represented in the stack traces
        // recorded for the locks.
        deadlock_graph->UpdateStackTrace(mu_id, all_locks->n + 1, GetStack);

        // For each other mutex already held by this thread:
        for (int i = 0; i != all_locks->n; i++) {
            const GraphId other_node_id = all_locks->locks[i].id;
            const mutex *other =
                    static_cast<const mutex *>(deadlock_graph->Ptr(other_node_id));
            if (other == nullptr) {
                // Ignore stale lock
                continue;
            }

            // Add the acquired-before edge to the graph.
            if (!deadlock_graph->InsertEdge(other_node_id, mu_id)) {
                ScopedDeadlockReportBuffers scoped_buffers;
                DeadlockReportBuffers *b = scoped_buffers.b;
                static int number_of_reported_deadlocks = 0;
                number_of_reported_deadlocks++;
                // Symbolize only 2 first deadlock report to avoid huge slowdowns.
                bool symbolize = number_of_reported_deadlocks <= 2;
                ABEL_RAW_LOG(ERROR, "Potential mutex deadlock: %s",
                             CurrentStackString(b->buf, sizeof(b->buf), symbolize));
                int len = 0;
                for (int j = 0; j != all_locks->n; j++) {
                    void *pr = deadlock_graph->Ptr(all_locks->locks[j].id);
                    if (pr != nullptr) {
                        snprintf(b->buf + len, sizeof(b->buf) - len, " %p", pr);
                        len += static_cast<int>(strlen(&b->buf[len]));
                    }
                }
                ABEL_RAW_LOG(ERROR, "Acquiring %p    Mutexes held: %s",
                             static_cast<void *>(mu), b->buf);
                ABEL_RAW_LOG(ERROR, "Cycle: ");
                int path_len = deadlock_graph->FindPath(
                        mu_id, other_node_id, ABEL_ARRAYSIZE(b->path), b->path);
                for (int j = 0; j != path_len; j++) {
                    GraphId id = b->path[j];
                    mutex *path_mu = static_cast<mutex *>(deadlock_graph->Ptr(id));
                    if (path_mu == nullptr)
                        continue;
                    void **stack;
                    int depth = deadlock_graph->GetStackTrace(id, &stack);
                    snprintf(b->buf, sizeof(b->buf),
                             "mutex@%p stack: ", static_cast<void *>(path_mu));
                    StackString(stack, depth, b->buf + strlen(b->buf),
                                static_cast<int>(sizeof(b->buf) - strlen(b->buf)),
                                symbolize);
                    ABEL_RAW_LOG(ERROR, "%s", b->buf);
                }
                if (synch_deadlock_detection.load(std::memory_order_acquire) ==
                    on_deadlock_cycle::kAbort) {
                    deadlock_graph_mu.unlock();  // avoid deadlock in fatal sighandler
                    ABEL_RAW_LOG(FATAL, "dying due to potential deadlock");
                    return mu_id;
                }
                break;   // report at most one potential deadlock per acquisition
            }
        }

        return mu_id;
    }

// Invoke DeadlockCheck() iff we're in debug mode and
// deadlock checking has been enabled.
    static ABEL_FORCE_INLINE GraphId DebugOnlyDeadlockCheck(mutex *mu) {
        if (kDebugMode && synch_deadlock_detection.load(std::memory_order_acquire) !=
                          on_deadlock_cycle::kIgnore) {
            return DeadlockCheck(mu);
        } else {
            return InvalidGraphId();
        }
    }

    void mutex::ForgetDeadlockInfo() {
        if (kDebugMode && synch_deadlock_detection.load(std::memory_order_acquire) !=
                          on_deadlock_cycle::kIgnore) {
            deadlock_graph_mu.lock();
            if (deadlock_graph != nullptr) {
                deadlock_graph->RemoveNode(this);
            }
            deadlock_graph_mu.unlock();
        }
    }

    void mutex::assert_not_held() const {
        // We have the data to allow this check only if in debug mode and deadlock
        // detection is enabled.
        if (kDebugMode &&
            (mu_.load(std::memory_order_relaxed) & (kMuWriter | kMuReader)) != 0 &&
            synch_deadlock_detection.load(std::memory_order_acquire) !=
            on_deadlock_cycle::kIgnore) {
            GraphId id = GetGraphId(const_cast<mutex *>(this));
            SynchLocksHeld *locks = Synch_GetAllLocks();
            for (int i = 0; i != locks->n; i++) {
                if (locks->locks[i].id == id) {
                    SynchEvent *mu_events = GetSynchEvent(this);
                    ABEL_RAW_LOG(FATAL, "thread should not hold mutex %p %s",
                                 static_cast<const void *>(this),
                                 (mu_events == nullptr ? "" : mu_events->name));
                }
            }
        }
    }

// Attempt to acquire *mu, and return whether successful.  The implementation
// may spin for a short while if the lock cannot be acquired immediately.
    static bool TryAcquireWithSpinning(std::atomic<intptr_t> *mu) {
        int c = mutex_globals.spinloop_iterations;
        int result = -1;  // result of operation:  0=false, 1=true, -1=unknown

        do {  // do/while somewhat faster on AMD
            intptr_t v = mu->load(std::memory_order_relaxed);
            if ((v & (kMuReader | kMuEvent)) != 0) {  // a reader or tracing -> give up
                result = 0;
            } else if (((v & kMuWriter) == 0) &&  // no holder -> try to acquire
                       mu->compare_exchange_strong(v, kMuWriter | v,
                                                   std::memory_order_acquire,
                                                   std::memory_order_relaxed)) {
                result = 1;
            }
        } while (result == -1 && --c > 0);
        return result == 1;
    }

    ABEL_XRAY_LOG_ARGS(1)

    void mutex::lock() {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, 0);
        GraphId id = DebugOnlyDeadlockCheck(this);
        intptr_t v = mu_.load(std::memory_order_relaxed);
        // try fast acquire, then spin loop
        if ((v & (kMuWriter | kMuReader | kMuEvent)) != 0 ||
            !mu_.compare_exchange_strong(v, kMuWriter | v,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed)) {
            // try spin acquire, then slow loop
            if (!TryAcquireWithSpinning(&this->mu_)) {
                this->LockSlow(kExclusive, nullptr, 0);
            }
        }
        DebugOnlyLockEnter(this, id);
        ABEL_TSAN_MUTEX_POST_LOCK(this, 0, 0);
    }

    ABEL_XRAY_LOG_ARGS(1)

    void mutex::reader_lock() {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, __tsan_mutex_read_lock);
        GraphId id = DebugOnlyDeadlockCheck(this);
        intptr_t v = mu_.load(std::memory_order_relaxed);
        // try fast acquire, then slow loop
        if ((v & (kMuWriter | kMuWait | kMuEvent)) != 0 ||
            !mu_.compare_exchange_strong(v, (kMuReader | v) + kMuOne,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed)) {
            this->LockSlow(kShared, nullptr, 0);
        }
        DebugOnlyLockEnter(this, id);
        ABEL_TSAN_MUTEX_POST_LOCK(this, __tsan_mutex_read_lock, 0);
    }

    void mutex::LockWhen(const condition &cond) {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, 0);
        GraphId id = DebugOnlyDeadlockCheck(this);
        this->LockSlow(kExclusive, &cond, 0);
        DebugOnlyLockEnter(this, id);
        ABEL_TSAN_MUTEX_POST_LOCK(this, 0, 0);
    }

    bool mutex::LockWhenWithTimeout(const condition &cond, abel::duration timeout) {
        return LockWhenWithDeadline(cond, DeadlineFromTimeout(timeout));
    }

    bool mutex::LockWhenWithDeadline(const condition &cond, abel::abel_time deadline) {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, 0);
        GraphId id = DebugOnlyDeadlockCheck(this);
        bool res = LockSlowWithDeadline(kExclusive, &cond,
                                        KernelTimeout(deadline), 0);
        DebugOnlyLockEnter(this, id);
        ABEL_TSAN_MUTEX_POST_LOCK(this, 0, 0);
        return res;
    }

    void mutex::ReaderLockWhen(const condition &cond) {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, __tsan_mutex_read_lock);
        GraphId id = DebugOnlyDeadlockCheck(this);
        this->LockSlow(kShared, &cond, 0);
        DebugOnlyLockEnter(this, id);
        ABEL_TSAN_MUTEX_POST_LOCK(this, __tsan_mutex_read_lock, 0);
    }

    bool mutex::ReaderLockWhenWithTimeout(const condition &cond,
                                          abel::duration timeout) {
        return ReaderLockWhenWithDeadline(cond, DeadlineFromTimeout(timeout));
    }

    bool mutex::ReaderLockWhenWithDeadline(const condition &cond,
                                           abel::abel_time deadline) {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, __tsan_mutex_read_lock);
        GraphId id = DebugOnlyDeadlockCheck(this);
        bool res = LockSlowWithDeadline(kShared, &cond, KernelTimeout(deadline), 0);
        DebugOnlyLockEnter(this, id);
        ABEL_TSAN_MUTEX_POST_LOCK(this, __tsan_mutex_read_lock, 0);
        return res;
    }

    void mutex::Await(const condition &cond) {
        if (cond.eval()) {    // condition already true; nothing to do
            if (kDebugMode) {
                this->assert_reader_held();
            }
        } else {              // normal case
            ABEL_RAW_CHECK(this->AwaitCommon(cond, KernelTimeout::Never()),
                           "condition untrue on return from Await");
        }
    }

    bool mutex::AwaitWithTimeout(const condition &cond, abel::duration timeout) {
        return AwaitWithDeadline(cond, DeadlineFromTimeout(timeout));
    }

    bool mutex::AwaitWithDeadline(const condition &cond, abel::abel_time deadline) {
        if (cond.eval()) {      // condition already true; nothing to do
            if (kDebugMode) {
                this->assert_reader_held();
            }
            return true;
        }

        KernelTimeout t{deadline};
        bool res = this->AwaitCommon(cond, t);
        ABEL_RAW_CHECK(res || t.has_timeout(),
                       "condition untrue on return from Await");
        return res;
    }

    bool mutex::AwaitCommon(const condition &cond, KernelTimeout t) {
        this->assert_reader_held();
        MuHow how =
                (mu_.load(std::memory_order_relaxed) & kMuWriter) ? kExclusive : kShared;
        ABEL_TSAN_MUTEX_PRE_UNLOCK(this, TsanFlags(how));
        synch_wait_params waitp(
                how, &cond, t, nullptr /*no cvmu*/, Synch_GetPerThreadAnnotated(this),
                nullptr /*no cv_word*/);
        int flags = kMuHasBlocked;
        if (!condition::guaranteed_equal(&cond, nullptr)) {
            flags |= kMuIsCond;
        }
        this->UnlockSlow(&waitp);
        this->Block(waitp.thread);
        ABEL_TSAN_MUTEX_POST_UNLOCK(this, TsanFlags(how));
        ABEL_TSAN_MUTEX_PRE_LOCK(this, TsanFlags(how));
        this->LockSlowLoop(&waitp, flags);
        bool res = waitp.cond != nullptr ||  // => cond known true from LockSlowLoop
                   EvalConditionAnnotated(&cond, this, true, false, how == kShared);
        ABEL_TSAN_MUTEX_POST_LOCK(this, TsanFlags(how), 0);
        return res;
    }

    ABEL_XRAY_LOG_ARGS(1)

    bool mutex::try_lock() {
        ABEL_TSAN_MUTEX_PRE_LOCK(this, __tsan_mutex_try_lock);
        intptr_t v = mu_.load(std::memory_order_relaxed);
        if ((v & (kMuWriter | kMuReader | kMuEvent)) == 0 &&  // try fast acquire
            mu_.compare_exchange_strong(v, kMuWriter | v,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
            DebugOnlyLockEnter(this);
            ABEL_TSAN_MUTEX_POST_LOCK(this, __tsan_mutex_try_lock, 0);
            return true;
        }
        if ((v & kMuEvent) != 0) {              // we're recording events
            if ((v & kExclusive->slow_need_zero) == 0 &&  // try fast acquire
                mu_.compare_exchange_strong(
                        v, (kExclusive->fast_or | v) + kExclusive->fast_add,
                        std::memory_order_acquire, std::memory_order_relaxed)) {
                DebugOnlyLockEnter(this);
                PostSynchEvent(this, SYNCH_EV_TRYLOCK_SUCCESS);
                ABEL_TSAN_MUTEX_POST_LOCK(this, __tsan_mutex_try_lock, 0);
                return true;
            } else {
                PostSynchEvent(this, SYNCH_EV_TRYLOCK_FAILED);
            }
        }
        ABEL_TSAN_MUTEX_POST_LOCK(
                this, __tsan_mutex_try_lock | __tsan_mutex_try_lock_failed, 0);
        return false;
    }

    ABEL_XRAY_LOG_ARGS(1)

    bool mutex::reader_try_lock() {
        ABEL_TSAN_MUTEX_PRE_LOCK(this,
                                 __tsan_mutex_read_lock | __tsan_mutex_try_lock);
        intptr_t v = mu_.load(std::memory_order_relaxed);
        // The while-loops (here and below) iterate only if the mutex word keeps
        // changing (typically because the reader count changes) under the CAS.  We
        // limit the number of attempts to avoid having to think about livelock.
        int loop_limit = 5;
        while ((v & (kMuWriter | kMuWait | kMuEvent)) == 0 && loop_limit != 0) {
            if (mu_.compare_exchange_strong(v, (kMuReader | v) + kMuOne,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
                DebugOnlyLockEnter(this);
                ABEL_TSAN_MUTEX_POST_LOCK(
                        this, __tsan_mutex_read_lock | __tsan_mutex_try_lock, 0);
                return true;
            }
            loop_limit--;
            v = mu_.load(std::memory_order_relaxed);
        }
        if ((v & kMuEvent) != 0) {   // we're recording events
            loop_limit = 5;
            while ((v & kShared->slow_need_zero) == 0 && loop_limit != 0) {
                if (mu_.compare_exchange_strong(v, (kMuReader | v) + kMuOne,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed)) {
                    DebugOnlyLockEnter(this);
                    PostSynchEvent(this, SYNCH_EV_READERTRYLOCK_SUCCESS);
                    ABEL_TSAN_MUTEX_POST_LOCK(
                            this, __tsan_mutex_read_lock | __tsan_mutex_try_lock, 0);
                    return true;
                }
                loop_limit--;
                v = mu_.load(std::memory_order_relaxed);
            }
            if ((v & kMuEvent) != 0) {
                PostSynchEvent(this, SYNCH_EV_READERTRYLOCK_FAILED);
            }
        }
        ABEL_TSAN_MUTEX_POST_LOCK(this,
                                  __tsan_mutex_read_lock | __tsan_mutex_try_lock |
                                  __tsan_mutex_try_lock_failed,
                                  0);
        return false;
    }

    ABEL_XRAY_LOG_ARGS(1)

    void mutex::unlock() {
        ABEL_TSAN_MUTEX_PRE_UNLOCK(this, 0);
        DebugOnlyLockLeave(this);
        intptr_t v = mu_.load(std::memory_order_relaxed);

        if (kDebugMode && ((v & (kMuWriter | kMuReader)) != kMuWriter)) {
            ABEL_RAW_LOG(FATAL, "mutex unlocked when destroyed or not locked: v=0x%x",
                         static_cast<unsigned>(v));
        }

        // should_try_cas is whether we'll try a compare-and-swap immediately.
        // NOTE: optimized out when kDebugMode is false.
        bool should_try_cas = ((v & (kMuEvent | kMuWriter)) == kMuWriter &&
                               (v & (kMuWait | kMuDesig)) != kMuWait);
        // But, we can use an alternate computation of it, that compilers
        // currently don't find on their own.  When that changes, this function
        // can be simplified.
        intptr_t x = (v ^ (kMuWriter | kMuWait)) & (kMuWriter | kMuEvent);
        intptr_t y = (v ^ (kMuWriter | kMuWait)) & (kMuWait | kMuDesig);
        // Claim: "x == 0 && y > 0" is equal to should_try_cas.
        // Also, because kMuWriter and kMuEvent exceed kMuDesig and kMuWait,
        // all possible non-zero values for x exceed all possible values for y.
        // Therefore, (x == 0 && y > 0) == (x < y).
        if (kDebugMode && should_try_cas != (x < y)) {
            // We would usually use PRIdPTR here, but is not correctly implemented
            // within the android toolchain.
            ABEL_RAW_LOG(FATAL, "internal logic error %llx %llx %llx\n",
                         static_cast<long long>(v), static_cast<long long>(x),
                         static_cast<long long>(y));
        }
        if (x < y &&
            mu_.compare_exchange_strong(v, v & ~(kMuWrWait | kMuWriter),
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
            // fast writer release (writer with no waiters or with designated waker)
        } else {
            this->UnlockSlow(nullptr /*no waitp*/);  // take slow path
        }
        ABEL_TSAN_MUTEX_POST_UNLOCK(this, 0);
    }

// Requires v to represent a reader-locked state.
    static bool ExactlyOneReader(intptr_t v) {
        assert((v & (kMuWriter | kMuReader)) == kMuReader);
        assert((v & kMuHigh) != 0);
        // The more straightforward "(v & kMuHigh) == kMuOne" also works, but
        // on some architectures the following generates slightly smaller code.
        // It may be faster too.
        constexpr intptr_t kMuMultipleWaitersMask = kMuHigh ^kMuOne;
        return (v & kMuMultipleWaitersMask) == 0;
    }

    ABEL_XRAY_LOG_ARGS(1)

    void mutex::reader_unlock() {
        ABEL_TSAN_MUTEX_PRE_UNLOCK(this, __tsan_mutex_read_lock);
        DebugOnlyLockLeave(this);
        intptr_t v = mu_.load(std::memory_order_relaxed);
        assert((v & (kMuWriter | kMuReader)) == kMuReader);
        if ((v & (kMuReader | kMuWait | kMuEvent)) == kMuReader) {
            // fast reader release (reader with no waiters)
            intptr_t clear = ExactlyOneReader(v) ? kMuReader | kMuOne : kMuOne;
            if (mu_.compare_exchange_strong(v, v - clear,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
                ABEL_TSAN_MUTEX_POST_UNLOCK(this, __tsan_mutex_read_lock);
                return;
            }
        }
        this->UnlockSlow(nullptr /*no waitp*/);  // take slow path
        ABEL_TSAN_MUTEX_POST_UNLOCK(this, __tsan_mutex_read_lock);
    }

// The zap_desig_waker bitmask is used to clear the designated waker flag in
// the mutex if this thread has blocked, and therefore may be the designated
// waker.
    static const intptr_t zap_desig_waker[] = {
            ~static_cast<intptr_t>(0),  // not blocked
            ~static_cast<intptr_t>(
                    kMuDesig)  // blocked; turn off the designated waker bit
    };

// The ignore_waiting_writers bitmask is used to ignore the existence
// of waiting writers if a reader that has already blocked once
// wakes up.
    static const intptr_t ignore_waiting_writers[] = {
            ~static_cast<intptr_t>(0),  // not blocked
            ~static_cast<intptr_t>(
                    kMuWrWait)  // blocked; pretend there are no waiting writers
    };

// Internal version of LockWhen().  See LockSlowWithDeadline()
    void mutex::LockSlow(MuHow how, const condition *cond, int flags) {
        ABEL_RAW_CHECK(
                this->LockSlowWithDeadline(how, cond, KernelTimeout::Never(), flags),
                "condition untrue on return from LockSlow");
    }

// Compute cond->eval() and tell race detectors that we do it under mutex mu.
    static ABEL_FORCE_INLINE bool EvalConditionAnnotated(const condition *cond, mutex *mu,
                                                         bool locking, bool trylock,
                                                         bool read_lock) {
        // Delicate annotation dance.
        // We are currently inside of read/write lock/unlock operation.
        // All memory accesses are ignored inside of mutex operations + for unlock
        // operation tsan considers that we've already released the mutex.
        bool res = false;
#ifdef THREAD_SANITIZER
        const int flags = read_lock ? __tsan_mutex_read_lock : 0;
        const int tryflags = flags | (trylock ? __tsan_mutex_try_lock : 0);
#endif
        if (locking) {
            // For lock we pretend that we have finished the operation,
            // evaluate the predicate, then unlock the mutex and start locking it again
            // to match the annotation at the end of outer lock operation.
            // Note: we can't simply do POST_LOCK, eval, PRE_LOCK, because then tsan
            // will think the lock acquisition is recursive which will trigger
            // deadlock detector.
            ABEL_TSAN_MUTEX_POST_LOCK(mu, tryflags, 0);
            res = cond->eval();
            // There is no "try" version of unlock, so use flags instead of tryflags.
            ABEL_TSAN_MUTEX_PRE_UNLOCK(mu, flags);
            ABEL_TSAN_MUTEX_POST_UNLOCK(mu, flags);
            ABEL_TSAN_MUTEX_PRE_LOCK(mu, tryflags);
        } else {
            // Similarly, for unlock we pretend that we have unlocked the mutex,
            // lock the mutex, evaluate the predicate, and start unlocking it again
            // to match the annotation at the end of outer unlock operation.
            ABEL_TSAN_MUTEX_POST_UNLOCK(mu, flags);
            ABEL_TSAN_MUTEX_PRE_LOCK(mu, flags);
            ABEL_TSAN_MUTEX_POST_LOCK(mu, flags, 0);
            res = cond->eval();
            ABEL_TSAN_MUTEX_PRE_UNLOCK(mu, flags);
        }
        // Prevent unused param warnings in non-TSAN builds.
        static_cast<void>(mu);
        static_cast<void>(trylock);
        static_cast<void>(read_lock);
        return res;
    }

// Compute cond->eval() hiding it from race detectors.
// We are hiding it because inside of UnlockSlow we can evaluate a predicate
// that was just added by a concurrent lock operation; lock adds the predicate
// to the internal mutex list without actually acquiring the mutex
// (it only acquires the internal spinlock, which is rightfully invisible for
// tsan). As the result there is no tsan-visible synchronization between the
// addition and this thread. So if we would enable race detection here,
// it would race with the predicate initialization.
    static ABEL_FORCE_INLINE bool EvalConditionIgnored(mutex *mu, const condition *cond) {
        // Memory accesses are already ignored inside of lock/unlock operations,
        // but synchronization operations are also ignored. When we evaluate the
        // predicate we must ignore only memory accesses but not synchronization,
        // because missed synchronization can lead to false reports later.
        // So we "divert" (which un-ignores both memory accesses and synchronization)
        // and then separately turn on ignores of memory accesses.
        ABEL_TSAN_MUTEX_PRE_DIVERT(mu, 0);
        ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN();
        bool res = cond->eval();
        ANNOTATE_IGNORE_READS_AND_WRITES_END();
        ABEL_TSAN_MUTEX_POST_DIVERT(mu, 0);
        static_cast<void>(mu);  // Prevent unused param warning in non-TSAN builds.
        return res;
    }

// Internal equivalent of *LockWhenWithDeadline(), where
//   "t" represents the absolute timeout; !t.has_timeout() means "forever".
//   "how" is "kShared" (for ReaderLockWhen) or "kExclusive" (for LockWhen)
// In flags, bits are ored together:
// - kMuHasBlocked indicates that the client has already blocked on the call so
//   the designated waker bit must be cleared and waiting writers should not
//   obstruct this call
// - kMuIsCond indicates that this is a conditional acquire (condition variable,
//   Await,  LockWhen) so contention profiling should be suppressed.
    bool mutex::LockSlowWithDeadline(MuHow how, const condition *cond,
                                     KernelTimeout t, int flags) {
        intptr_t v = mu_.load(std::memory_order_relaxed);
        bool unlock = false;
        if ((v & how->fast_need_zero) == 0 &&  // try fast acquire
            mu_.compare_exchange_strong(
                    v, (how->fast_or | (v & zap_desig_waker[flags & kMuHasBlocked])) +
                       how->fast_add,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
            if (cond == nullptr ||
                EvalConditionAnnotated(cond, this, true, false, how == kShared)) {
                return true;
            }
            unlock = true;
        }
        synch_wait_params waitp(
                how, cond, t, nullptr /*no cvmu*/, Synch_GetPerThreadAnnotated(this),
                nullptr /*no cv_word*/);
        if (!condition::guaranteed_equal(cond, nullptr)) {
            flags |= kMuIsCond;
        }
        if (unlock) {
            this->UnlockSlow(&waitp);
            this->Block(waitp.thread);
            flags |= kMuHasBlocked;
        }
        this->LockSlowLoop(&waitp, flags);
        return waitp.cond != nullptr ||  // => cond known true from LockSlowLoop
               cond == nullptr ||
               EvalConditionAnnotated(cond, this, true, false, how == kShared);
    }

// RAW_CHECK_FMT() takes a condition, a printf-style format string, and
// the printf-style argument list.   The format string must be a literal.
// Arguments after the first are not evaluated unless the condition is true.
#define RAW_CHECK_FMT(cond, ...)                                   \
  do {                                                             \
    if (ABEL_UNLIKELY(!(cond))) {                             \
      ABEL_RAW_LOG(FATAL, "Check " #cond " failed: " __VA_ARGS__); \
    }                                                              \
  } while (0)

    static void CheckForMutexCorruption(intptr_t v, const char *label) {
        // Test for either of two situations that should not occur in v:
        //   kMuWriter and kMuReader
        //   kMuWrWait and !kMuWait
        const uintptr_t w = v ^kMuWait;
        // By flipping that bit, we can now test for:
        //   kMuWriter and kMuReader in w
        //   kMuWrWait and kMuWait in w
        // We've chosen these two pairs of values to be so that they will overlap,
        // respectively, when the word is left shifted by three.  This allows us to
        // save a branch in the common (correct) case of them not being coincident.
        static_assert(kMuReader << 3 == kMuWriter, "must match");
        static_assert(kMuWait << 3 == kMuWrWait, "must match");
        if (ABEL_LIKELY((w & (w << 3) & (kMuWriter | kMuWrWait)) == 0))
            return;
        RAW_CHECK_FMT((v & (kMuWriter | kMuReader)) != (kMuWriter | kMuReader),
                      "%s: mutex corrupt: both reader and writer lock held: %p",
                      label, reinterpret_cast<void *>(v));
        RAW_CHECK_FMT((v & (kMuWait | kMuWrWait)) != kMuWrWait,
                      "%s: mutex corrupt: waiting writer with no waiters: %p",
                      label, reinterpret_cast<void *>(v));
        assert(false);
    }

    void mutex::LockSlowLoop(synch_wait_params *waitp, int flags) {
        int c = 0;
        intptr_t v = mu_.load(std::memory_order_relaxed);
        if ((v & kMuEvent) != 0) {
            PostSynchEvent(this,
                           waitp->how == kExclusive ? SYNCH_EV_LOCK : SYNCH_EV_READERLOCK);
        }
        ABEL_RAW_CHECK(
                waitp->thread->waitp == nullptr || waitp->thread->suppress_fatal_errors,
                "detected illegal recursion into mutex code");
        for (;;) {
            v = mu_.load(std::memory_order_relaxed);
            CheckForMutexCorruption(v, "lock");
            if ((v & waitp->how->slow_need_zero) == 0) {
                if (mu_.compare_exchange_strong(
                        v, (waitp->how->fast_or |
                            (v & zap_desig_waker[flags & kMuHasBlocked])) +
                           waitp->how->fast_add,
                        std::memory_order_acquire, std::memory_order_relaxed)) {
                    if (waitp->cond == nullptr ||
                        EvalConditionAnnotated(waitp->cond, this, true, false,
                                               waitp->how == kShared)) {
                        break;  // we timed out, or condition true, so return
                    }
                    this->UnlockSlow(waitp);  // got lock but condition false
                    this->Block(waitp->thread);
                    flags |= kMuHasBlocked;
                    c = 0;
                }
            } else {                      // need to access waiter list
                bool dowait = false;
                if ((v & (kMuSpin | kMuWait)) == 0) {   // no waiters
                    // This thread tries to become the one and only waiter.
                    PerThreadSynch *new_h = Enqueue(nullptr, waitp, v, flags);
                    intptr_t nv = (v & zap_desig_waker[flags & kMuHasBlocked] & kMuLow) |
                                  kMuWait;
                    ABEL_RAW_CHECK(new_h != nullptr, "Enqueue to empty list failed");
                    if (waitp->how == kExclusive && (v & kMuReader) != 0) {
                        nv |= kMuWrWait;
                    }
                    if (mu_.compare_exchange_strong(
                            v, reinterpret_cast<intptr_t>(new_h) | nv,
                            std::memory_order_release, std::memory_order_relaxed)) {
                        dowait = true;
                    } else {            // attempted Enqueue() failed
                        // zero out the waitp field set by Enqueue()
                        waitp->thread->waitp = nullptr;
                    }
                } else if ((v & waitp->how->slow_inc_need_zero &
                            ignore_waiting_writers[flags & kMuHasBlocked]) == 0) {
                    // This is a reader that needs to increment the reader count,
                    // but the count is currently held in the last waiter.
                    if (mu_.compare_exchange_strong(
                            v, (v & zap_desig_waker[flags & kMuHasBlocked]) | kMuSpin |
                               kMuReader,
                            std::memory_order_acquire, std::memory_order_relaxed)) {
                        PerThreadSynch *h = GetPerThreadSynch(v);
                        h->readers += kMuOne;       // inc reader count in waiter
                        do {                        // release spinlock
                            v = mu_.load(std::memory_order_relaxed);
                        } while (!mu_.compare_exchange_weak(v, (v & ~kMuSpin) | kMuReader,
                                                            std::memory_order_release,
                                                            std::memory_order_relaxed));
                        if (waitp->cond == nullptr ||
                            EvalConditionAnnotated(waitp->cond, this, true, false,
                                                   waitp->how == kShared)) {
                            break;  // we timed out, or condition true, so return
                        }
                        this->UnlockSlow(waitp);           // got lock but condition false
                        this->Block(waitp->thread);
                        flags |= kMuHasBlocked;
                        c = 0;
                    }
                } else if ((v & kMuSpin) == 0 &&  // attempt to queue ourselves
                           mu_.compare_exchange_strong(
                                   v, (v & zap_desig_waker[flags & kMuHasBlocked]) | kMuSpin |
                                      kMuWait,
                                   std::memory_order_acquire, std::memory_order_relaxed)) {
                    PerThreadSynch *h = GetPerThreadSynch(v);
                    PerThreadSynch *new_h = Enqueue(h, waitp, v, flags);
                    intptr_t wr_wait = 0;
                    ABEL_RAW_CHECK(new_h != nullptr, "Enqueue to list failed");
                    if (waitp->how == kExclusive && (v & kMuReader) != 0) {
                        wr_wait = kMuWrWait;      // give priority to a waiting writer
                    }
                    do {                        // release spinlock
                        v = mu_.load(std::memory_order_relaxed);
                    } while (!mu_.compare_exchange_weak(
                            v, (v & (kMuLow & ~kMuSpin)) | kMuWait | wr_wait |
                               reinterpret_cast<intptr_t>(new_h),
                            std::memory_order_release, std::memory_order_relaxed));
                    dowait = true;
                }
                if (dowait) {
                    this->Block(waitp->thread);  // wait until removed from list or timeout
                    flags |= kMuHasBlocked;
                    c = 0;
                }
            }
            ABEL_RAW_CHECK(
                    waitp->thread->waitp == nullptr || waitp->thread->suppress_fatal_errors,
                    "detected illegal recursion into mutex code");
            c = Delay(c, GENTLE);          // delay, then try again
        }
        ABEL_RAW_CHECK(
                waitp->thread->waitp == nullptr || waitp->thread->suppress_fatal_errors,
                "detected illegal recursion into mutex code");
        if ((v & kMuEvent) != 0) {
            PostSynchEvent(this,
                           waitp->how == kExclusive ? SYNCH_EV_LOCK_RETURNING :
                           SYNCH_EV_READERLOCK_RETURNING);
        }
    }

// unlock this mutex, which is held by the current thread.
// If waitp is non-zero, it must be the wait parameters for the current thread
// which holds the lock but is not runnable because its condition is false
// or it is in the process of blocking on a condition variable; it must requeue
// itself on the mutex/condvar to wait for its condition to become true.
    void mutex::UnlockSlow(synch_wait_params *waitp) {
        intptr_t v = mu_.load(std::memory_order_relaxed);
        this->assert_reader_held();
        CheckForMutexCorruption(v, "unlock");
        if ((v & kMuEvent) != 0) {
            PostSynchEvent(this,
                           (v & kMuWriter) != 0 ? SYNCH_EV_UNLOCK : SYNCH_EV_READERUNLOCK);
        }
        int c = 0;
        // the waiter under consideration to wake, or zero
        PerThreadSynch *w = nullptr;
        // the predecessor to w or zero
        PerThreadSynch *pw = nullptr;
        // head of the list searched previously, or zero
        PerThreadSynch *old_h = nullptr;
        // a condition that's known to be false.
        const condition *known_false = nullptr;
        PerThreadSynch *wake_list = kPerThreadSynchNull;   // list of threads to wake
        intptr_t wr_wait = 0;        // set to kMuWrWait if we wake a reader and a
        // later writer could have acquired the lock
        // (starvation avoidance)
        ABEL_RAW_CHECK(waitp == nullptr || waitp->thread->waitp == nullptr ||
                       waitp->thread->suppress_fatal_errors,
                       "detected illegal recursion into mutex code");
        // This loop finds threads wake_list to wakeup if any, and removes them from
        // the list of waiters.  In addition, it places waitp.thread on the queue of
        // waiters if waitp is non-zero.
        for (;;) {
            v = mu_.load(std::memory_order_relaxed);
            if ((v & kMuWriter) != 0 && (v & (kMuWait | kMuDesig)) != kMuWait &&
                waitp == nullptr) {
                // fast writer release (writer with no waiters or with designated waker)
                if (mu_.compare_exchange_strong(v, v & ~(kMuWrWait | kMuWriter),
                                                std::memory_order_release,
                                                std::memory_order_relaxed)) {
                    return;
                }
            } else if ((v & (kMuReader | kMuWait)) == kMuReader && waitp == nullptr) {
                // fast reader release (reader with no waiters)
                intptr_t clear = ExactlyOneReader(v) ? kMuReader | kMuOne : kMuOne;
                if (mu_.compare_exchange_strong(v, v - clear,
                                                std::memory_order_release,
                                                std::memory_order_relaxed)) {
                    return;
                }
            } else if ((v & kMuSpin) == 0 &&  // attempt to get spinlock
                       mu_.compare_exchange_strong(v, v | kMuSpin,
                                                   std::memory_order_acquire,
                                                   std::memory_order_relaxed)) {
                if ((v & kMuWait) == 0) {       // no one to wake
                    intptr_t nv;
                    bool do_enqueue = true;  // always Enqueue() the first time
                    ABEL_RAW_CHECK(waitp != nullptr,
                                   "UnlockSlow is confused");  // about to sleep
                    do {    // must loop to release spinlock as reader count may change
                        v = mu_.load(std::memory_order_relaxed);
                        // decrement reader count if there are readers
                        intptr_t new_readers = (v >= kMuOne) ? v - kMuOne : v;
                        PerThreadSynch *new_h = nullptr;
                        if (do_enqueue) {
                            // If we are enqueuing on a cond_var (waitp->cv_word != nullptr) then
                            // we must not retry here.  The initial attempt will always have
                            // succeeded, further attempts would enqueue us against *this due to
                            // Fer() handling.
                            do_enqueue = (waitp->cv_word == nullptr);
                            new_h = Enqueue(nullptr, waitp, new_readers, kMuIsCond);
                        }
                        intptr_t clear = kMuWrWait | kMuWriter;  // by default clear write bit
                        if ((v & kMuWriter) == 0 && ExactlyOneReader(v)) {  // last reader
                            clear = kMuWrWait | kMuReader;                    // clear read bit
                        }
                        nv = (v & kMuLow & ~clear & ~kMuSpin);
                        if (new_h != nullptr) {
                            nv |= kMuWait | reinterpret_cast<intptr_t>(new_h);
                        } else {  // new_h could be nullptr if we queued ourselves on a
                            // cond_var
                            // In that case, we must place the reader count back in the mutex
                            // word, as Enqueue() did not store it in the new waiter.
                            nv |= new_readers & kMuHigh;
                        }
                        // release spinlock & our lock; retry if reader-count changed
                        // (writer count cannot change since we hold lock)
                    } while (!mu_.compare_exchange_weak(v, nv,
                                                        std::memory_order_release,
                                                        std::memory_order_relaxed));
                    break;
                }

                // There are waiters.
                // Set h to the head of the circular waiter list.
                PerThreadSynch *h = GetPerThreadSynch(v);
                if ((v & kMuReader) != 0 && (h->readers & kMuHigh) > kMuOne) {
                    // a reader but not the last
                    h->readers -= kMuOne;  // release our lock
                    intptr_t nv = v;       // normally just release spinlock
                    if (waitp != nullptr) {  // but waitp!=nullptr => must queue ourselves
                        PerThreadSynch *new_h = Enqueue(h, waitp, v, kMuIsCond);
                        ABEL_RAW_CHECK(new_h != nullptr,
                                       "waiters disappeared during Enqueue()!");
                        nv &= kMuLow;
                        nv |= kMuWait | reinterpret_cast<intptr_t>(new_h);
                    }
                    mu_.store(nv, std::memory_order_release);  // release spinlock
                    // can release with a store because there were waiters
                    break;
                }

                // Either we didn't search before, or we marked the queue
                // as "maybe_unlocking" and no one else should have changed it.
                ABEL_RAW_CHECK(old_h == nullptr || h->maybe_unlocking,
                               "mutex queue changed beneath us");

                // The lock is becoming free, and there's a waiter
                if (old_h != nullptr &&
                    !old_h->may_skip) {                  // we used old_h as a terminator
                    old_h->may_skip = true;                // allow old_h to skip once more
                    ABEL_RAW_CHECK(old_h->skip == nullptr, "illegal skip from head");
                    if (h != old_h && MuSameCondition(old_h, old_h->next)) {
                        old_h->skip = old_h->next;  // old_h not head & can skip to successor
                    }
                }
                if (h->next->waitp->how == kExclusive &&
                    condition::guaranteed_equal(h->next->waitp->cond, nullptr)) {
                    // easy case: writer with no condition; no need to search
                    pw = h;                       // wake w, the successor of h (=pw)
                    w = h->next;
                    w->wake = true;
                    // We are waking up a writer.  This writer may be racing against
                    // an already awake reader for the lock.  We want the
                    // writer to usually win this race,
                    // because if it doesn't, we can potentially keep taking a reader
                    // perpetually and writers will starve.  Worse than
                    // that, this can also starve other readers if kMuWrWait gets set
                    // later.
                    wr_wait = kMuWrWait;
                } else if (w != nullptr && (w->waitp->how == kExclusive || h == old_h)) {
                    // we found a waiter w to wake on a previous iteration and either it's
                    // a writer, or we've searched the entire list so we have all the
                    // readers.
                    if (pw == nullptr) {  // if w's predecessor is unknown, it must be h
                        pw = h;
                    }
                } else {
                    // At this point we don't know all the waiters to wake, and the first
                    // waiter has a condition or is a reader.  We avoid searching over
                    // waiters we've searched on previous iterations by starting at
                    // old_h if it's set.  If old_h==h, there's no one to wakeup at all.
                    if (old_h == h) {      // we've searched before, and nothing's new
                        // so there's no one to wake.
                        intptr_t nv = (v & ~(kMuReader | kMuWriter | kMuWrWait));
                        h->readers = 0;
                        h->maybe_unlocking = false;   // finished unlocking
                        if (waitp != nullptr) {       // we must queue ourselves and sleep
                            PerThreadSynch *new_h = Enqueue(h, waitp, v, kMuIsCond);
                            nv &= kMuLow;
                            if (new_h != nullptr) {
                                nv |= kMuWait | reinterpret_cast<intptr_t>(new_h);
                            }  // else new_h could be nullptr if we queued ourselves on a
                            // cond_var
                        }
                        // release spinlock & lock
                        // can release with a store because there were waiters
                        mu_.store(nv, std::memory_order_release);
                        break;
                    }

                    // set up to walk the list
                    PerThreadSynch *w_walk;   // current waiter during list walk
                    PerThreadSynch *pw_walk;  // previous waiter during list walk
                    if (old_h != nullptr) {  // we've searched up to old_h before
                        pw_walk = old_h;
                        w_walk = old_h->next;
                    } else {            // no prior search, start at beginning
                        pw_walk =
                                nullptr;  // h->next's predecessor may change; don't record it
                        w_walk = h->next;
                    }

                    h->may_skip = false;  // ensure we never skip past h in future searches
                    // even if other waiters are queued after it.
                    ABEL_RAW_CHECK(h->skip == nullptr, "illegal skip from head");

                    h->maybe_unlocking = true;  // we're about to scan the waiter list
                    // without the spinlock held.
                    // Enqueue must be conservative about
                    // priority queuing.

                    // We must release the spinlock to evaluate the conditions.
                    mu_.store(v, std::memory_order_release);  // release just spinlock
                    // can release with a store because there were waiters

                    // h is the last waiter queued, and w_walk the first unsearched waiter.
                    // Without the spinlock, the locations mu_ and h->next may now change
                    // underneath us, but since we hold the lock itself, the only legal
                    // change is to add waiters between h and w_walk.  Therefore, it's safe
                    // to walk the path from w_walk to h inclusive. (TryRemove() can remove
                    // a waiter anywhere, but it acquires both the spinlock and the mutex)

                    old_h = h;        // remember we searched to here

                    // Walk the path upto and including h looking for waiters we can wake.
                    while (pw_walk != h) {
                        w_walk->wake = false;
                        if (w_walk->waitp->cond ==
                            nullptr ||  // no condition => vacuously true OR
                            (w_walk->waitp->cond != known_false &&
                             // this thread's condition is not known false, AND
                             //  is in fact true
                             EvalConditionIgnored(this, w_walk->waitp->cond))) {
                            if (w == nullptr) {
                                w_walk->wake = true;    // can wake this waiter
                                w = w_walk;
                                pw = pw_walk;
                                if (w_walk->waitp->how == kExclusive) {
                                    wr_wait = kMuWrWait;
                                    break;                // bail if waking this writer
                                }
                            } else if (w_walk->waitp->how == kShared) {  // wake if a reader
                                w_walk->wake = true;
                            } else {   // writer with true condition
                                wr_wait = kMuWrWait;
                            }
                        } else {                  // can't wake; condition false
                            known_false = w_walk->waitp->cond;  // remember last false condition
                        }
                        if (w_walk->wake) {   // we're waking reader w_walk
                            pw_walk = w_walk;   // don't skip similar waiters
                        } else {              // not waking; skip as much as possible
                            pw_walk = Skip(w_walk);
                        }
                        // If pw_walk == h, then load of pw_walk->next can race with
                        // concurrent write in Enqueue(). However, at the same time
                        // we do not need to do the load, because we will bail out
                        // from the loop anyway.
                        if (pw_walk != h) {
                            w_walk = pw_walk->next;
                        }
                    }

                    continue;  // restart for(;;)-loop to wakeup w or to find more waiters
                }
                ABEL_RAW_CHECK(pw->next == w, "pw not w's predecessor");
                // The first (and perhaps only) waiter we've chosen to wake is w, whose
                // predecessor is pw.  If w is a reader, we must wake all the other
                // waiters with wake==true as well.  We may also need to queue
                // ourselves if waitp != null.  The spinlock and the lock are still
                // held.

                // This traverses the list in [ pw->next, h ], where h is the head,
                // removing all elements with wake==true and placing them in the
                // singly-linked list wake_list.  Returns the new head.
                h = DequeueAllWakeable(h, pw, &wake_list);

                intptr_t nv = (v & kMuEvent) | kMuDesig;
                // assume no waiters left,
                // set kMuDesig for INV1a

                if (waitp != nullptr) {  // we must queue ourselves and sleep
                    h = Enqueue(h, waitp, v, kMuIsCond);
                    // h is new last waiter; could be null if we queued ourselves on a
                    // cond_var
                }

                ABEL_RAW_CHECK(wake_list != kPerThreadSynchNull,
                               "unexpected empty wake list");

                if (h != nullptr) {  // there are waiters left
                    h->readers = 0;
                    h->maybe_unlocking = false;     // finished unlocking
                    nv |= wr_wait | kMuWait | reinterpret_cast<intptr_t>(h);
                }

                // release both spinlock & lock
                // can release with a store because there were waiters
                mu_.store(nv, std::memory_order_release);
                break;  // out of for(;;)-loop
            }
            c = Delay(c, AGGRESSIVE);  // aggressive here; no one can proceed till we do
        }                            // end of for(;;)-loop

        if (wake_list != kPerThreadSynchNull) {
            int64_t enqueue_timestamp = wake_list->waitp->contention_start_cycles;
            bool cond_waiter = wake_list->cond_waiter;
            do {
                wake_list = Wakeup(wake_list);              // wake waiters
            } while (wake_list != kPerThreadSynchNull);
            if (!cond_waiter) {
                // Sample lock contention events only if the (first) waiter was trying to
                // acquire the lock, not waiting on a condition variable or condition.
                int64_t wait_cycles = chrono_internal::cycle_clock::now() - enqueue_timestamp;
                mutex_tracer("slow release", this, wait_cycles);
                ABEL_TSAN_MUTEX_PRE_DIVERT(this, 0);
                submit_profile_data(enqueue_timestamp);
                ABEL_TSAN_MUTEX_POST_DIVERT(this, 0);
            }
        }
    }

// Used by cond_var implementation to reacquire mutex after waking from
// condition variable.  This routine is used instead of lock() because the
// waiting thread may have been moved from the condition variable queue to the
// mutex queue without a wakeup, by Trans().  In that case, when the thread is
// finally woken, the woken thread will believe it has been woken from the
// condition variable (i.e. its PC will be in when in the cond_var code), when
// in fact it has just been woken from the mutex.  Thus, it must enter the slow
// path of the mutex in the same state as if it had just woken from the mutex.
// That is, it must ensure to clear kMuDesig (INV1b).
    void mutex::Trans(MuHow how) {
        this->LockSlow(how, nullptr, kMuHasBlocked | kMuIsCond);
    }

// Used by cond_var implementation to effectively wake thread w from the
// condition variable.  If this mutex is free, we simply wake the thread.
// It will later acquire the mutex with high probability.  Otherwise, we
// enqueue thread w on this mutex.
    void mutex::Fer(PerThreadSynch *w) {
        int c = 0;
        ABEL_RAW_CHECK(w->waitp->cond == nullptr,
                       "mutex::Fer while waiting on condition");
        ABEL_RAW_CHECK(!w->waitp->timeout.has_timeout(),
                       "mutex::Fer while in timed wait");
        ABEL_RAW_CHECK(w->waitp->cv_word == nullptr,
                       "mutex::Fer with pending cond_var queueing");
        for (;;) {
            intptr_t v = mu_.load(std::memory_order_relaxed);
            // Note: must not queue if the mutex is unlocked (nobody will wake it).
            // For example, we can have only kMuWait (conditional) or maybe
            // kMuWait|kMuWrWait.
            // conflicting != 0 implies that the waking thread cannot currently take
            // the mutex, which in turn implies that someone else has it and can wake
            // us if we queue.
            const intptr_t conflicting =
                    kMuWriter | (w->waitp->how == kShared ? 0 : kMuReader);
            if ((v & conflicting) == 0) {
                w->next = nullptr;
                w->state.store(PerThreadSynch::kAvailable, std::memory_order_release);
                IncrementSynchSem(this, w);
                return;
            } else {
                if ((v & (kMuSpin | kMuWait)) == 0) {       // no waiters
                    // This thread tries to become the one and only waiter.
                    PerThreadSynch *new_h = Enqueue(nullptr, w->waitp, v, kMuIsCond);
                    ABEL_RAW_CHECK(new_h != nullptr,
                                   "Enqueue failed");  // we must queue ourselves
                    if (mu_.compare_exchange_strong(
                            v, reinterpret_cast<intptr_t>(new_h) | (v & kMuLow) | kMuWait,
                            std::memory_order_release, std::memory_order_relaxed)) {
                        return;
                    }
                } else if ((v & kMuSpin) == 0 &&
                           mu_.compare_exchange_strong(v, v | kMuSpin | kMuWait)) {
                    PerThreadSynch *h = GetPerThreadSynch(v);
                    PerThreadSynch *new_h = Enqueue(h, w->waitp, v, kMuIsCond);
                    ABEL_RAW_CHECK(new_h != nullptr,
                                   "Enqueue failed");  // we must queue ourselves
                    do {
                        v = mu_.load(std::memory_order_relaxed);
                    } while (!mu_.compare_exchange_weak(
                            v,
                            (v & kMuLow & ~kMuSpin) | kMuWait |
                            reinterpret_cast<intptr_t>(new_h),
                            std::memory_order_release, std::memory_order_relaxed));
                    return;
                }
            }
            c = Delay(c, GENTLE);
        }
    }

    void mutex::assert_held() const {
        if ((mu_.load(std::memory_order_relaxed) & kMuWriter) == 0) {
            SynchEvent *e = GetSynchEvent(this);
            ABEL_RAW_LOG(FATAL, "thread should hold write lock on mutex %p %s",
                         static_cast<const void *>(this),
                         (e == nullptr ? "" : e->name));
        }
    }

    void mutex::assert_reader_held() const {
        if ((mu_.load(std::memory_order_relaxed) & (kMuReader | kMuWriter)) == 0) {
            SynchEvent *e = GetSynchEvent(this);
            ABEL_RAW_LOG(
                    FATAL, "thread should hold at least a read lock on mutex %p %s",
                    static_cast<const void *>(this), (e == nullptr ? "" : e->name));
        }
    }

// -------------------------------- condition variables
    static const intptr_t kCvSpin = 0x0001L;   // spinlock protects waiter list
    static const intptr_t kCvEvent = 0x0002L;  // record events

    static const intptr_t kCvLow = 0x0003L;  // low order bits of CV

// Hack to make constant values available to gdb pretty printer
    enum {
        kGdbCvSpin = kCvSpin, kGdbCvEvent = kCvEvent, kGdbCvLow = kCvLow,
    };

    static_assert(PerThreadSynch::kAlignment > kCvLow,
                  "PerThreadSynch::kAlignment must be greater than kCvLow");

    void cond_var::enable_debug_log(const char *name) {
        SynchEvent *e = EnsureSynchEvent(&this->cv_, name, kCvEvent, kCvSpin);
        e->log = true;
        UnrefSynchEvent(e);
    }

    cond_var::~cond_var() {
        if ((cv_.load(std::memory_order_relaxed) & kCvEvent) != 0) {
            ForgetSynchEvent(&this->cv_, kCvEvent, kCvSpin);
        }
    }

// Remove thread s from the list of waiters on this condition variable.
    void cond_var::Remove(PerThreadSynch *s) {
        intptr_t v;
        int c = 0;
        for (v = cv_.load(std::memory_order_relaxed);;
             v = cv_.load(std::memory_order_relaxed)) {
            if ((v & kCvSpin) == 0 &&  // attempt to acquire spinlock
                cv_.compare_exchange_strong(v, v | kCvSpin,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
                PerThreadSynch *h = reinterpret_cast<PerThreadSynch *>(v & ~kCvLow);
                if (h != nullptr) {
                    PerThreadSynch *w = h;
                    while (w->next != s && w->next != h) {  // search for thread
                        w = w->next;
                    }
                    if (w->next == s) {           // found thread; remove it
                        w->next = s->next;
                        if (h == s) {
                            h = (w == s) ? nullptr : w;
                        }
                        s->next = nullptr;
                        s->state.store(PerThreadSynch::kAvailable, std::memory_order_release);
                    }
                }
                // release spinlock
                cv_.store((v & kCvEvent) | reinterpret_cast<intptr_t>(h),
                          std::memory_order_release);
                return;
            } else {
                c = Delay(c, GENTLE);            // try again after a delay
            }
        }
    }

// Queue thread waitp->thread on condition variable word cv_word using
// wait parameters waitp.
// We split this into a separate routine, rather than simply doing it as part
// of WaitCommon().  If we were to queue ourselves on the condition variable
// before calling mutex::UnlockSlow(), the mutex code might be re-entered (via
// the logging code, or via a condition function) and might potentially attempt
// to block this thread.  That would be a problem if the thread were already on
// a the condition variable waiter queue.  Thus, we use the waitp->cv_word
// to tell the unlock code to call CondVarEnqueue() to queue the thread on the
// condition variable queue just before the mutex is to be unlocked, and (most
// importantly) after any call to an external routine that might re-enter the
// mutex code.
    static void CondVarEnqueue(synch_wait_params *waitp) {
        // This thread might be transferred to the mutex queue by Fer() when
        // we are woken.  To make sure that is what happens, Enqueue() doesn't
        // call CondVarEnqueue() again but instead uses its normal code.  We
        // must do this before we queue ourselves so that cv_word will be null
        // when seen by the dequeuer, who may wish immediately to requeue
        // this thread on another queue.
        std::atomic<intptr_t> *cv_word = waitp->cv_word;
        waitp->cv_word = nullptr;

        intptr_t v = cv_word->load(std::memory_order_relaxed);
        int c = 0;
        while ((v & kCvSpin) != 0 ||  // acquire spinlock
               !cv_word->compare_exchange_weak(v, v | kCvSpin,
                                               std::memory_order_acquire,
                                               std::memory_order_relaxed)) {
            c = Delay(c, GENTLE);
            v = cv_word->load(std::memory_order_relaxed);
        }
        ABEL_RAW_CHECK(waitp->thread->waitp == nullptr, "waiting when shouldn't be");
        waitp->thread->waitp = waitp;      // prepare ourselves for waiting
        PerThreadSynch *h = reinterpret_cast<PerThreadSynch *>(v & ~kCvLow);
        if (h == nullptr) {  // add this thread to waiter list
            waitp->thread->next = waitp->thread;
        } else {
            waitp->thread->next = h->next;
            h->next = waitp->thread;
        }
        waitp->thread->state.store(PerThreadSynch::kQueued,
                                   std::memory_order_relaxed);
        cv_word->store((v & kCvEvent) | reinterpret_cast<intptr_t>(waitp->thread),
                       std::memory_order_release);
    }

    bool cond_var::WaitCommon(mutex *mutex, KernelTimeout t) {
        bool rc = false;          // return value; true iff we timed-out

        intptr_t mutex_v = mutex->mu_.load(std::memory_order_relaxed);
        mutex::MuHow mutex_how = ((mutex_v & kMuWriter) != 0) ? kExclusive : kShared;
        ABEL_TSAN_MUTEX_PRE_UNLOCK(mutex, TsanFlags(mutex_how));

        // maybe trace this call
        intptr_t v = cv_.load(std::memory_order_relaxed);
        cond_var_tracer("wait", this);
        if ((v & kCvEvent) != 0) {
            PostSynchEvent(this, SYNCH_EV_WAIT);
        }

        // Release mu and wait on condition variable.
        synch_wait_params waitp(mutex_how, nullptr, t, mutex,
                                Synch_GetPerThreadAnnotated(mutex), &cv_);
        // UnlockSlow() will call CondVarEnqueue() just before releasing the
        // mutex, thus queuing this thread on the condition variable.  See
        // CondVarEnqueue() for the reasons.
        mutex->UnlockSlow(&waitp);

        // wait for signal
        while (waitp.thread->state.load(std::memory_order_acquire) ==
               PerThreadSynch::kQueued) {
            if (!mutex::DecrementSynchSem(mutex, waitp.thread, t)) {
                this->Remove(waitp.thread);
                rc = true;
            }
        }

        ABEL_RAW_CHECK(waitp.thread->waitp != nullptr, "not waiting when should be");
        waitp.thread->waitp = nullptr;  // cleanup

        // maybe trace this call
        cond_var_tracer("Unwait", this);
        if ((v & kCvEvent) != 0) {
            PostSynchEvent(this, SYNCH_EV_WAIT_RETURNING);
        }

        // From synchronization point of view wait is unlock of the mutex followed
        // by lock of the mutex. We've annotated start of unlock in the beginning
        // of the function. Now, finish unlock and annotate lock of the mutex.
        // (Trans is effectively lock).
        ABEL_TSAN_MUTEX_POST_UNLOCK(mutex, TsanFlags(mutex_how));
        ABEL_TSAN_MUTEX_PRE_LOCK(mutex, TsanFlags(mutex_how));
        mutex->Trans(mutex_how);  // Reacquire mutex
        ABEL_TSAN_MUTEX_POST_LOCK(mutex, TsanFlags(mutex_how), 0);
        return rc;
    }

    bool cond_var::wait_with_timeout(mutex *mu, abel::duration timeout) {
        return wait_with_deadline(mu, DeadlineFromTimeout(timeout));
    }

    bool cond_var::wait_with_deadline(mutex *mu, abel::abel_time deadline) {
        return WaitCommon(mu, KernelTimeout(deadline));
    }

    void cond_var::wait(mutex *mu) {
        WaitCommon(mu, KernelTimeout::Never());
    }

// Wake thread w
// If it was a timed wait, w will be waiting on w->cv
// Otherwise, if it was not a mutex mutex, w will be waiting on w->sem
// Otherwise, w is transferred to the mutex mutex via mutex::Fer().
    void cond_var::Wakeup(PerThreadSynch *w) {
        if (w->waitp->timeout.has_timeout() || w->waitp->cvmu == nullptr) {
            // The waiting thread only needs to observe "w->state == kAvailable" to be
            // released, we must cache "cvmu" before clearing "next".
            mutex *mu = w->waitp->cvmu;
            w->next = nullptr;
            w->state.store(PerThreadSynch::kAvailable, std::memory_order_release);
            mutex::IncrementSynchSem(mu, w);
        } else {
            w->waitp->cvmu->Fer(w);
        }
    }

    void cond_var::signal() {
        ABEL_TSAN_MUTEX_PRE_SIGNAL(nullptr, 0);
        intptr_t v;
        int c = 0;
        for (v = cv_.load(std::memory_order_relaxed); v != 0;
             v = cv_.load(std::memory_order_relaxed)) {
            if ((v & kCvSpin) == 0 &&  // attempt to acquire spinlock
                cv_.compare_exchange_strong(v, v | kCvSpin,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
                PerThreadSynch *h = reinterpret_cast<PerThreadSynch *>(v & ~kCvLow);
                PerThreadSynch *w = nullptr;
                if (h != nullptr) {  // remove first waiter
                    w = h->next;
                    if (w == h) {
                        h = nullptr;
                    } else {
                        h->next = w->next;
                    }
                }
                // release spinlock
                cv_.store((v & kCvEvent) | reinterpret_cast<intptr_t>(h),
                          std::memory_order_release);
                if (w != nullptr) {
                    cond_var::Wakeup(w);                // wake waiter, if there was one
                    cond_var_tracer("signal wakeup", this);
                }
                if ((v & kCvEvent) != 0) {
                    PostSynchEvent(this, SYNCH_EV_SIGNAL);
                }
                ABEL_TSAN_MUTEX_POST_SIGNAL(nullptr, 0);
                return;
            } else {
                c = Delay(c, GENTLE);
            }
        }
        ABEL_TSAN_MUTEX_POST_SIGNAL(nullptr, 0);
    }

    void cond_var::signal_all() {
        ABEL_TSAN_MUTEX_PRE_SIGNAL(nullptr, 0);
        intptr_t v;
        int c = 0;
        for (v = cv_.load(std::memory_order_relaxed); v != 0;
             v = cv_.load(std::memory_order_relaxed)) {
            // empty the list if spinlock free
            // We do this by simply setting the list to empty using
            // compare and swap.   We then have the entire list in our hands,
            // which cannot be changing since we grabbed it while no one
            // held the lock.
            if ((v & kCvSpin) == 0 &&
                cv_.compare_exchange_strong(v, v & kCvEvent, std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
                PerThreadSynch *h = reinterpret_cast<PerThreadSynch *>(v & ~kCvLow);
                if (h != nullptr) {
                    PerThreadSynch *w;
                    PerThreadSynch *n = h->next;
                    do {                          // for every thread, wake it up
                        w = n;
                        n = n->next;
                        cond_var::Wakeup(w);
                    } while (w != h);
                    cond_var_tracer("signal_all wakeup", this);
                }
                if ((v & kCvEvent) != 0) {
                    PostSynchEvent(this, SYNCH_EV_SIGNALALL);
                }
                ABEL_TSAN_MUTEX_POST_SIGNAL(nullptr, 0);
                return;
            } else {
                c = Delay(c, GENTLE);           // try again after a delay
            }
        }
        ABEL_TSAN_MUTEX_POST_SIGNAL(nullptr, 0);
    }

    void releasable_mutex_lock::Release() {
        ABEL_RAW_CHECK(this->mu_ != nullptr,
                       "releasable_mutex_lock::Release may only be called once");
        this->mu_->unlock();
        this->mu_ = nullptr;
    }

#ifdef THREAD_SANITIZER
    extern "C" void __tsan_read1(void *addr);
#else
#define __tsan_read1(addr)  // do nothing if TSan not enabled
#endif

// A function that just returns its argument, dereferenced
    static bool Dereference(void *arg) {
        // ThreadSanitizer does not instrument this file for memory accesses.
        // This function dereferences a user variable that can participate
        // in a data race, so we need to manually tell TSan about this memory access.
        __tsan_read1(arg);
        return *(static_cast<bool *>(arg));
    }

    condition::condition() {}   // null constructor, used for kTrue only
    const condition condition::kTrue;

    condition::condition(bool (*func)(void *), void *arg)
            : eval_(&call_void_ptr_function),
              function_(func),
              method_(nullptr),
              arg_(arg) {}

    bool condition::call_void_ptr_function(const condition *c) {
        return (*c->function_)(c->arg_);
    }

    condition::condition(const bool *cond)
            : eval_(call_void_ptr_function),
              function_(Dereference),
              method_(nullptr),
            // const_cast is safe since Dereference does not modify arg
              arg_(const_cast<bool *>(cond)) {}

    bool condition::eval() const {
        // eval_ == null for kTrue
        return (this->eval_ == nullptr) || (*this->eval_)(this);
    }

    bool condition::guaranteed_equal(const condition *a, const condition *b) {
        if (a == nullptr) {
            return b == nullptr || b->eval_ == nullptr;
        }
        if (b == nullptr || b->eval_ == nullptr) {
            return a->eval_ == nullptr;
        }
        return a->eval_ == b->eval_ && a->function_ == b->function_ &&
               a->arg_ == b->arg_ && a->method_ == b->method_;
    }


}  // namespace abel
