// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#include <gflags/gflags.h>                            // DEFINE_int32
#include "melon/base/compat.h"
#include "melon/base/fd_utility.h"                         // make_close_on_exec
#include "melon/log/logging.h"                            // MELON_LOG
#include "melon/hash/murmurhash3.h"// fmix32
#include "melon/fiber/internal/fiber.h"                          // fiber_start_background
#include "melon/rpc/event_dispatcher.h"

#ifdef MELON_RPC_SOCKET_HAS_EOF
#include "melon/rpc/details/has_epollrdhup.h"
#endif

#include "melon/rpc/reloadable_flags.h"

#if defined(MELON_PLATFORM_OSX)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#endif

namespace melon::rpc {

    DEFINE_int32(event_dispatcher_num, 1, "Number of event dispatcher");

    DEFINE_bool(usercode_in_pthread, false,
                "Call user's callback in pthreads, use fibers otherwise");

    EventDispatcher::EventDispatcher()
            : _epfd(-1), _stop(false), _tid(0), _consumer_thread_attr(FIBER_ATTR_NORMAL) {
#if defined(MELON_PLATFORM_LINUX)
        _epfd = epoll_create(1024 * 1024);
        if (_epfd < 0) {
            MELON_PLOG(FATAL) << "Fail to create epoll";
            return;
        }
#elif defined(MELON_PLATFORM_OSX)
        _epfd = kqueue();
        if (_epfd < 0) {
            MELON_PLOG(FATAL) << "Fail to create kqueue";
            return;
        }
#else
#error Not implemented
#endif
        MELON_CHECK_EQ(0, melon::base::make_close_on_exec(_epfd));

        _wakeup_fds[0] = -1;
        _wakeup_fds[1] = -1;
        if (pipe(_wakeup_fds) != 0) {
            MELON_PLOG(FATAL) << "Fail to create pipe";
            return;
        }
    }

    EventDispatcher::~EventDispatcher() {
        Stop();
        Join();
        if (_epfd >= 0) {
            close(_epfd);
            _epfd = -1;
        }
        if (_wakeup_fds[0] > 0) {
            close(_wakeup_fds[0]);
            close(_wakeup_fds[1]);
        }
    }

    int EventDispatcher::Start(const fiber_attribute *consumer_thread_attr) {
        if (_epfd < 0) {
#if defined(MELON_PLATFORM_LINUX)
            MELON_LOG(FATAL) << "epoll was not created";
#elif defined(MELON_PLATFORM_OSX)
            MELON_LOG(FATAL) << "kqueue was not created";
#endif
            return -1;
        }

        if (_tid != 0) {
            MELON_LOG(FATAL) << "Already started this dispatcher(" << this
                       << ") in fiber=" << _tid;
            return -1;
        }

        // Set _consumer_thread_attr before creating epoll/kqueue thread to make sure
        // everyting seems sane to the thread.
        _consumer_thread_attr = (consumer_thread_attr ?
                                 *consumer_thread_attr : FIBER_ATTR_NORMAL);

        //_consumer_thread_attr is used in StartInputEvent(), assign flag NEVER_QUIT to it will cause new fiber
        // that created by epoll_wait() never to quit.
        _epoll_thread_attr = _consumer_thread_attr | FIBER_NEVER_QUIT;

        // Polling thread uses the same attr for consumer threads (NORMAL right
        // now). Previously, we used small stack (32KB) which may be overflowed
        // when the older comlog (e.g. 3.1.85) calls com_openlog_r(). Since this
        // is also a potential issue for consumer threads, using the same attr
        // should be a reasonable solution.
        int rc = fiber_start_background(
                &_tid, &_epoll_thread_attr, RunThis, this);
        if (rc) {
            MELON_LOG(FATAL) << "Fail to create epoll/kqueue thread: " << melon_error(rc);
            return -1;
        }
        return 0;
    }

    bool EventDispatcher::Running() const {
        return !_stop && _epfd >= 0 && _tid != 0;
    }

    void EventDispatcher::Stop() {
        _stop = true;

        if (_epfd >= 0) {
#if defined(MELON_PLATFORM_LINUX)
            epoll_event evt = { EPOLLOUT,  { nullptr } };
            epoll_ctl(_epfd, EPOLL_CTL_ADD, _wakeup_fds[1], &evt);
#elif defined(MELON_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, _wakeup_fds[1], EVFILT_WRITE, EV_ADD | EV_ENABLE,
                   0, 0, nullptr);
            kevent(_epfd, &kqueue_event, 1, nullptr, 0, nullptr);
#endif
        }
    }

    void EventDispatcher::Join() {
        if (_tid) {
            fiber_join(_tid, nullptr);
            _tid = 0;
        }
    }

    int EventDispatcher::AddEpollOut(SocketId socket_id, int fd, bool pollin) {
        if (_epfd < 0) {
            errno = EINVAL;
            return -1;
        }

#if defined(MELON_PLATFORM_LINUX)
        epoll_event evt;
        evt.data.u64 = socket_id;
        evt.events = EPOLLOUT | EPOLLET;
#ifdef MELON_RPC_SOCKET_HAS_EOF
        evt.events |= has_epollrdhup;
#endif
        if (pollin) {
            evt.events |= EPOLLIN;
            if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &evt) < 0) {
                // This fd has been removed from epoll via `RemoveConsumer',
                // in which case errno will be ENOENT
                return -1;
            }
        } else {
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt) < 0) {
                return -1;
            }
        }
#elif defined(MELON_PLATFORM_OSX)
        struct kevent evt;
        //TODO(zhujiashun): add EV_EOF
        EV_SET(&evt, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR,
               0, 0, (void *) socket_id);
        if (kevent(_epfd, &evt, 1, nullptr, 0, nullptr) < 0) {
            return -1;
        }
        if (pollin) {
            EV_SET(&evt, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR,
                   0, 0, (void *) socket_id);
            if (kevent(_epfd, &evt, 1, nullptr, 0, nullptr) < 0) {
                return -1;
            }
        }
#endif
        return 0;
    }

    int EventDispatcher::RemoveEpollOut(SocketId socket_id,
                                        int fd, bool pollin) {
#if defined(MELON_PLATFORM_LINUX)
        if (pollin) {
            epoll_event evt;
            evt.data.u64 = socket_id;
            evt.events = EPOLLIN | EPOLLET;
#ifdef MELON_RPC_SOCKET_HAS_EOF
            evt.events |= has_epollrdhup;
#endif
            return epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &evt);
        } else {
            return epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
        }
#elif defined(MELON_PLATFORM_OSX)
        struct kevent evt;
        EV_SET(&evt, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        if (kevent(_epfd, &evt, 1, nullptr, 0, nullptr) < 0) {
            return -1;
        }
        if (pollin) {
            EV_SET(&evt, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR,
                   0, 0, (void *) socket_id);
            return kevent(_epfd, &evt, 1, nullptr, 0, nullptr);
        }
        return 0;
#endif
        return -1;
    }

    int EventDispatcher::AddConsumer(SocketId socket_id, int fd) {
        if (_epfd < 0) {
            errno = EINVAL;
            return -1;
        }
#if defined(MELON_PLATFORM_LINUX)
        epoll_event evt;
        evt.events = EPOLLIN | EPOLLET;
        evt.data.u64 = socket_id;
#ifdef MELON_RPC_SOCKET_HAS_EOF
        evt.events |= has_epollrdhup;
#endif
        return epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt);
#elif defined(MELON_PLATFORM_OSX)
        struct kevent evt;
        EV_SET(&evt, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR,
               0, 0, (void *) socket_id);
        return kevent(_epfd, &evt, 1, nullptr, 0, nullptr);
#endif
        return -1;
    }

    int EventDispatcher::RemoveConsumer(int fd) {
        if (fd < 0) {
            return -1;
        }
        // Removing the consumer from dispatcher before closing the fd because
        // if process was forked and the fd is not marked as close-on-exec,
        // closing does not set reference count of the fd to 0, thus does not
        // remove the fd from epoll. More badly, the fd will not be removable
        // from epoll again! If the fd was level-triggered and there's data left,
        // epoll_wait will keep returning events of the fd continuously, making
        // program abnormal.
#if defined(MELON_PLATFORM_LINUX)
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
            MELON_PLOG(WARNING) << "Fail to remove fd=" << fd << " from epfd=" << _epfd;
            return -1;
        }
#elif defined(MELON_PLATFORM_OSX)
        struct kevent evt;
        EV_SET(&evt, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        kevent(_epfd, &evt, 1, nullptr, 0, nullptr);
        EV_SET(&evt, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        kevent(_epfd, &evt, 1, nullptr, 0, nullptr);
#endif
        return 0;
    }

    void *EventDispatcher::RunThis(void *arg) {
        ((EventDispatcher *) arg)->Run();
        return nullptr;
    }

    void EventDispatcher::Run() {
        while (!_stop) {
#if defined(MELON_PLATFORM_LINUX)
            epoll_event e[32];
#ifdef MELON_RPC_ADDITIONAL_EPOLL
            // Performance downgrades in examples.
            int n = epoll_wait(_epfd, e, MELON_ARRAY_SIZE(e), 0);
            if (n == 0) {
                n = epoll_wait(_epfd, e, MELON_ARRAY_SIZE(e), -1);
            }
#else
            const int n = epoll_wait(_epfd, e, MELON_ARRAY_SIZE(e), -1);
#endif
#elif defined(MELON_PLATFORM_OSX)
            struct kevent e[32];
            int n = kevent(_epfd, nullptr, 0, e, MELON_ARRAY_SIZE(e), nullptr);
#endif
            if (_stop) {
                // epoll_ctl/epoll_wait should have some sort of memory fencing
                // guaranteeing that we(after epoll_wait) see _stop set before
                // epoll_ctl.
                break;
            }
            if (n < 0) {
                if (EINTR == errno) {
                    // We've checked _stop, no wake-up will be missed.
                    continue;
                }
#if defined(MELON_PLATFORM_LINUX)
                    MELON_PLOG(FATAL) << "Fail to epoll_wait epfd=" << _epfd;
#elif defined(MELON_PLATFORM_OSX)
                MELON_PLOG(FATAL) << "Fail to kqueue epfd=" << _epfd;
#endif
                break;
            }
            for (int i = 0; i < n; ++i) {
#if defined(MELON_PLATFORM_LINUX)
                if (e[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)
#ifdef MELON_RPC_SOCKET_HAS_EOF
                    || (e[i].events & has_epollrdhup)
#endif
                    ) {
                    // We don't care about the return value.
                    Socket::StartInputEvent(e[i].data.u64, e[i].events,
                                            _consumer_thread_attr);
                }
#elif defined(MELON_PLATFORM_OSX)
                if ((e[i].flags & EV_ERROR) || e[i].filter == EVFILT_READ) {
                    // We don't care about the return value.
                    Socket::StartInputEvent((SocketId) e[i].udata, e[i].filter,
                                            _consumer_thread_attr);
                }
#endif
            }
            for (int i = 0; i < n; ++i) {
#if defined(MELON_PLATFORM_LINUX)
                if (e[i].events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
                    // We don't care about the return value.
                    Socket::HandleEpollOut(e[i].data.u64);
                }
#elif defined(MELON_PLATFORM_OSX)
                if ((e[i].flags & EV_ERROR) || e[i].filter == EVFILT_WRITE) {
                    // We don't care about the return value.
                    Socket::HandleEpollOut((SocketId) e[i].udata);
                }
#endif
            }
        }
    }

    static EventDispatcher *g_edisp = nullptr;
    static pthread_once_t g_edisp_once = PTHREAD_ONCE_INIT;

    static void StopAndJoinGlobalDispatchers() {
        for (int i = 0; i < FLAGS_event_dispatcher_num; ++i) {
            g_edisp[i].Stop();
            g_edisp[i].Join();
        }
    }

    void InitializeGlobalDispatchers() {
        g_edisp = new EventDispatcher[FLAGS_event_dispatcher_num];
        for (int i = 0; i < FLAGS_event_dispatcher_num; ++i) {
            const fiber_attribute attr = FLAGS_usercode_in_pthread ?
                                        FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL;
            MELON_CHECK_EQ(0, g_edisp[i].Start(&attr));
        }
        // This atexit is will be run before g_task_control.stop() because above
        // Start() initializes g_task_control by creating fiber (to run epoll/kqueue).
        MELON_CHECK_EQ(0, atexit(StopAndJoinGlobalDispatchers));
    }

    EventDispatcher &GetGlobalEventDispatcher(int fd) {
        pthread_once(&g_edisp_once, InitializeGlobalDispatchers);
        if (FLAGS_event_dispatcher_num == 1) {
            return g_edisp[0];
        }
        int index = melon::hash::fmix32(fd) % FLAGS_event_dispatcher_num;
        return g_edisp[index];
    }

} // namespace melon::rpc
