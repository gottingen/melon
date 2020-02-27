// Do not include.  This is an implementation detail of base/mutex.h.
//
// Declares three classes:
//
// base::internal::mutex_impl - implementation helper for Mutex
// base::internal::cond_var_impl - implementation helper for cond_var
// base::internal::synchronization_storage<T> - implementation helper for
//                                             Mutex, cond_var

#include <type_traits>

#if defined(_WIN32)
#include <condition_variable>
#include <mutex>
#else

#include <pthread.h>

#endif

#include <abel/functional/call_once.h>
#include <abel/chrono/time.h>

// Declare that Mutex::ReaderLock is actually lock().  Intended primarily
// for tests, and even then as a last resort.
#ifdef ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE
#error ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE cannot be directly set
#else
#define ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE 1
#endif

// Declare that Mutex::EnableInvariantDebugging is not implemented.
// Intended primarily for tests, and even then as a last resort.
#ifdef ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED
#error ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED cannot be directly set
#else
#define ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED 1
#endif

namespace abel {

    class condition;

    namespace synchronization_internal {

        class mutex_impl;

// Do not use this implementation detail of cond_var. Provides most of the
// implementation, but should not be placed directly in static storage
// because it will not linker initialize properly. See
// synchronization_storage<T> below for what we mean by linker
// initialization.
        class cond_var_impl {
        public:
            cond_var_impl();

            cond_var_impl(const cond_var_impl &) = delete;

            cond_var_impl &operator=(const cond_var_impl &) = delete;

            ~cond_var_impl();

            void signal();

            void signal_all();

            void wait(mutex_impl *mutex);

            bool wait_with_deadline(mutex_impl *mutex, abel::Time deadline);

        private:
#if defined(_WIN32)
            std::condition_variable_any std_cv_;
#else
            pthread_cond_t pthread_cv_;
#endif
        };

// Do not use this implementation detail of Mutex. Provides most of the
// implementation, but should not be placed directly in static storage
// because it will not linker initialize properly. See
// synchronization_storage<T> below for what we mean by linker
// initialization.
        class mutex_impl {
        public:
            mutex_impl();

            mutex_impl(const mutex_impl &) = delete;

            mutex_impl &operator=(const mutex_impl &) = delete;

            ~mutex_impl();

            void lock();

            bool try_lock();

            void unlock();

            void await(const condition &cond);

            bool await_with_deadline(const condition &cond, abel::Time deadline);

        private:
            friend class cond_var_impl;

#if defined(_WIN32)
            std::mutex std_mutex_;
#else
            pthread_mutex_t pthread_mutex_;
#endif

            // True if the underlying mutex is locked.  If the destructor is entered
            // while locked_, the underlying mutex is unlocked.  Mutex supports
            // destruction while locked, but the same is undefined behavior for both
            // pthread_mutex_t and std::mutex.
            bool locked_ = false;

            // Signaled before releasing the lock, in support of Await.
            cond_var_impl released_;
        };

// Do not use this implementation detail of cond_var and Mutex.  A storage
// space for T that supports a LinkerInitialized constructor. T must
// have a default constructor, which is called by the first call to
// get(). T's destructor is never called if the LinkerInitialized
// constructor is called.
//
// Objects constructed with the default constructor are constructed and
// destructed like any other object, and should never be allocated in
// static storage.
//
// Objects constructed with the LinkerInitialized constructor should
// always be in static storage. For such objects, calls to get() are always
// valid, except from signal handlers.
//
// Note that this implementation relies on undefined language behavior that
// are known to hold for the set of supported compilers. An analysis
// follows.
//
// From the C++11 standard:
//
// [basic.life] says an object has non-trivial initialization if it is of
// class type and it is initialized by a constructor other than a trivial
// default constructor.  (the LinkerInitialized constructor is
// non-trivial)
//
// [basic.life] says the lifetime of an object with a non-trivial
// constructor begins when the call to the constructor is complete.
//
// [basic.life] says the lifetime of an object with non-trivial destructor
// ends when the call to the destructor begins.
//
// [basic.life] p5 specifies undefined behavior when accessing non-static
// members of an instance outside its
// lifetime. (synchronization_storage::get() access non-static members)
//
// So, LinkerInitialized object of synchronization_storage uses a
// non-trivial constructor, which is called at some point during dynamic
// initialization, and is therefore subject to order of dynamic
// initialization bugs, where get() is called before the object's
// constructor is, resulting in undefined behavior.
//
// Similarly, a LinkerInitialized synchronization_storage object has a
// non-trivial destructor, and so its lifetime ends at some point during
// destruction of objects with static storage duration [basic.start.term]
// p4. There is a window where other exit code could call get() after this
// occurs, resulting in undefined behavior.
//
// Combined, these statements imply that LinkerInitialized instances
// of synchronization_storage<T> rely on undefined behavior.
//
// However, in practice, the implementation works on all supported
// compilers. Specifically, we rely on:
//
// a) zero-initialization being sufficient to initialize
// LinkerInitialized instances for the purposes of calling
// get(), regardless of when the constructor is called. This is
// because the is_dynamic_ boolean is correctly zero-initialized to
// false.
//
// b) the LinkerInitialized constructor is a NOP, and immaterial to
// even to concurrent calls to get().
//
// c) the destructor being a NOP for LinkerInitialized objects
// (guaranteed by a check for !is_dynamic_), and so any concurrent and
// subsequent calls to get() functioning as if the destructor were not
// called, by virtue of the instances' storage remaining valid after the
// destructor runs.
//
// d) That a-c apply transitively when synchronization_storage<T> is the
// only member of a class allocated in static storage.
//
// Nothing in the language standard guarantees that a-d hold.  In practice,
// these hold in all supported compilers.
//
// Future direction:
//
// Ideally, we would simply use std::mutex or a similar class, which when
// allocated statically would support use immediately after static
// initialization up until static storage is reclaimed (i.e. the properties
// we require of all "linker initialized" instances).
//
// Regarding construction in static storage, std::mutex is required to
// provide a constexpr default constructor [thread.mutex.class], which
// ensures the instance's lifetime begins with static initialization
// [basic.start.init], and so is immune to any problems caused by the order
// of dynamic initialization. However, as of this writing Microsoft's
// Visual Studio does not provide a constexpr constructor for std::mutex.
// See
// https://blogs.msdn.microsoft.com/vcblog/2015/06/02/constexpr-complete-for-vs-2015-rtm-c11-compiler-c17-stl/
//
// Regarding destruction of instances in static storage, [basic.life] does
// say an object ends when storage in which the occupies is released, in
// the case of non-trivial destructor. However, std::mutex is not specified
// to have a trivial destructor.
//
// So, we would need a class with a constexpr default constructor and a
// trivial destructor. Today, we can achieve neither desired property using
// std::mutex directly.
        template<typename T>
        class synchronization_storage {
        public:
            // Instances allocated on the heap or on the stack should use the default
            // constructor.
            synchronization_storage()
                    : is_dynamic_(true), once_() {}

            // Instances allocated in static storage (not on the heap, not on the
            // stack) should use this constructor.
            explicit synchronization_storage(base_internal::LinkerInitialized) {}

            constexpr explicit synchronization_storage(abel::const_init_type)
                    : is_dynamic_(false), once_(), space_{{0}} {}

            synchronization_storage(synchronization_storage &) = delete;

            synchronization_storage &operator=(synchronization_storage &) = delete;

            ~synchronization_storage() {
                if (is_dynamic_) {
                    get()->~T();
                }
            }

            // Retrieve the object in storage. This is fast and thread safe, but does
            // incur the cost of abel::call_once().
            //
            // For instances in static storage constructed with the
            // LinkerInitialized constructor, may be called at any time without
            // regard for order of dynamic initialization or destruction of objects
            // in static storage. See the class comment for caveats.
            T *get() {
                abel::call_once(once_, synchronization_storage::Construct, this);
                return reinterpret_cast<T *>(&space_);
            }

        private:
            static void Construct(synchronization_storage<T> *self) {
                new(&self->space_) T();
            }

            // When true, T's destructor is run when this is destructed.
            //
            // The LinkerInitialized constructor assumes this value will be set
            // false by static initialization.
            bool is_dynamic_;

            abel::once_flag once_;

            // An aligned space for T.
            typename std::aligned_storage<sizeof(T), alignof(T)>::type space_;
        };

    }  // namespace synchronization_internal

}  // namespace abel
