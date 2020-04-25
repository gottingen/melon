//
// Created by liyinbin on 2020/1/29.
//

#include <abel/thread/this_thread.h>
#include <cstdint>
#include <pthread.h>
#include <abel/base/profile.h>
#include <abel/system/platform_headers.h>
#include <cerrno>
#include <pthread.h>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <cstdlib>

namespace abel {

    namespace this_thread {

        __thread size_t sys_thread_id{0};
        __thread char sys_thread_name[16]{
        };

    } //namespace this_thread

    static void cache_thread_id() {
#ifdef _WIN32
        this_thread::sys_thread_id = static_cast<size_t>(::GetCurrentThreadId());
#elif __linux__
#if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
#define SYS_gettid __NR_gettid
#endif
        this_thread::sys_thread_id = static_cast<size_t>(syscall(SYS_gettid));
#elif __FreeBSD__
        long tid;
        thr_self(&tid);
        this_thread::sys_thread_id = static_cast<size_t>(tid);
#elif __APPLE__
        uint64_t tid;
        pthread_threadid_np(nullptr, &tid);
        this_thread::sys_thread_id = static_cast<size_t>(tid);
#else // Default to standard C++11 (other Unix)
        this_thread::sys_thread_id = static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
    }

    size_t thread_id() {
        if (ABEL_UNLIKELY(this_thread::sys_thread_id == 0)) {
            cache_thread_id();
        }
        return this_thread::sys_thread_id;
    }

    namespace detail {

        class thread_exit_helper {
        public:
            typedef void (*Fn)(void *);

            typedef std::pair<Fn, void *> Pair;

            ~thread_exit_helper() {
                // Call function reversely.
                while (!_fns.empty()) {
                    Pair back = _fns.back();
                    _fns.pop_back();
                    // Notice that _fns may be changed after calling Fn.
                    back.first(back.second);
                }
            }

            int add(Fn fn, void *arg) {
                try {
                    if (_fns.capacity() < 16) {
                        _fns.reserve(16);
                    }
                    _fns.push_back(std::make_pair(fn, arg));
                } catch (...) {
                    errno = ENOMEM;
                    return -1;
                }
                return 0;
            }

            void remove(Fn fn, void *arg) {
                std::vector<Pair>::iterator
                        it = std::find(_fns.begin(), _fns.end(), std::make_pair(fn, arg));
                if (it != _fns.end()) {
                    std::vector<Pair>::iterator ite = it + 1;
                    for (; ite != _fns.end() && ite->first == fn && ite->second == arg;
                           ++ite) {}
                    _fns.erase(it, ite);
                }
            }

        private:
            std::vector<Pair> _fns;
        };

        static pthread_key_t thread_atexit_key;
        static pthread_once_t thread_atexit_once = PTHREAD_ONCE_INIT;

        static void delete_thread_exit_helper(void *arg) {
            delete static_cast<thread_exit_helper *>(arg);
        }

        static void helper_exit_global() {
            detail::thread_exit_helper *h =
                    (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (h) {
                pthread_setspecific(detail::thread_atexit_key, nullptr);
                delete h;
            }
        }

        static void make_thread_atexit_key() {
            if (pthread_key_create(&thread_atexit_key, delete_thread_exit_helper) != 0) {
                fprintf(stderr, "Fail to create thread_atexit_key, abort\n");
                abort();
            }
            // If caller is not pthread, delete_thread_exit_helper will not be called.
            // We have to rely on atexit().
            atexit(helper_exit_global);
        }

        detail::thread_exit_helper *get_or_new_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);

            detail::thread_exit_helper *h =
                    (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (nullptr == h) {
                h = new(std::nothrow) detail::thread_exit_helper;
                if (nullptr != h) {
                    pthread_setspecific(detail::thread_atexit_key, h);
                }
            }
            return h;
        }

        detail::thread_exit_helper *get_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);
            return (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
        }

        static void call_single_arg_fn(void *fn) {
            ((void (*)()) fn)();
        }

    }  // namespace detail

    int thread_atexit(void (*fn)(void *), void *arg) {
        if (nullptr == fn) {
            errno = EINVAL;
            return -1;
        }
        detail::thread_exit_helper *h = detail::get_or_new_thread_exit_helper();
        if (h) {
            return h->add(fn, arg);
        }
        errno = ENOMEM;
        return -1;
    }

    int thread_atexit(void (*fn)()) {
        if (nullptr == fn) {
            errno = EINVAL;
            return -1;
        }
        return thread_atexit(detail::call_single_arg_fn, (void *) fn);
    }

    void thread_atexit_cancel(void (*fn)(void *), void *arg) {
        if (fn != nullptr) {
            detail::thread_exit_helper *h = detail::get_thread_exit_helper();
            if (h) {
                h->remove(fn, arg);
            }
        }
    }

    void thread_atexit_cancel(void (*fn)()) {
        if (nullptr != fn) {
            thread_atexit_cancel(detail::call_single_arg_fn, (void *) fn);
        }
    }
}
