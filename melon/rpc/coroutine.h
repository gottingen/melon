//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#ifndef MELON_RPC_COROUTINE_H_
#define MELON_RPC_COROUTINE_H_

#if __cplusplus >= 202002L

#define MELON_ENABLE_COROUTINE 1

#include <coroutine>
#include <functional>
#include <atomic>
#include <melon/rpc/callback.h>

namespace melon {
namespace experimental {

namespace detail {
class AwaitablePromiseBase;
template <typename T>
class AwaitablePromise;
}

class AwaitableDone;
class Coroutine;

// WARN：The melon coroutine feature is experimental, DO NOT use in production environment!

// Awaitable<T> is used as coroutine return type, for example:
//  Awaitable<int> func1() {
//      co_return 42;
//  }
//  Awaitable<std::string> func2() {
//  	int ret = co_await func1();
//      co_return std::to_string(ret);
//  }
template <typename T>
class Awaitable {
public:
    using promise_type = detail::AwaitablePromise<T>;

    ~Awaitable() {}

    // NOTE: compiler will generate calls to these functions automatically,
    // DO NOT call them manually
    bool await_ready();
    template <typename U>
    void await_suspend(std::coroutine_handle<detail::AwaitablePromise<U> > awaiting);
    T await_resume();

private:
friend class detail::AwaitablePromise<T>;
friend class AwaitableDone;
friend class Coroutine;

    Awaitable() = delete;
    Awaitable(promise_type* p) : _promise(p) {}

    promise_type* promise() {
        return _promise;
    }

    promise_type* _promise;
};

// Utility for a coroutine to wait for RPC call. Usage:
//    AwaitableDone done;
//    stub.CallMethod(&cntl, &req, &resp, &done);
//    co_await done.awaitable();
// 
class AwaitableDone : public google::protobuf::Closure {
public:
    AwaitableDone();
    
    void Run() override;

    Awaitable<void>& awaitable() {
        return _awaitable;
    }
private:
    Awaitable<void> _awaitable;
};

// Class for management of coroutine
// 1. To create a new coroutine and wait it finish:
//  Awaitable<void> func(double val);
//  
//  int main() {
//      Coroutine coro(func(1.0));
//      coro.join();
//  }
// 2. To wait a coroutine in another coroutine:
//  Awaitable<void> another_func() {
//      Coroutine coro(func(1.0));
//      co_await coro.awaitable<void>();
//  }
// 3. To create a detached coroutine without waiting:
//  Coroutine coro(func(1.0), true);
// 4. To sleep in a coroutine:
//  co_await Coroutine::usleep(100);
// 
// NOTE: Inside coroutine function, DO NOT call pthread-blocking or 
// fiber-blocking functions (eg. fiber_join(), fiber_usleep(), syncronized RPC),
// otherwise may cause dead lock or long latency.
class Coroutine {
public:
    template <typename T>
    Coroutine(Awaitable<T>&& aw, bool detach = false);

    ~Coroutine();

    template <typename T = void>
    T join();

    template <typename T = void>
    Awaitable<T> awaitable();

    static Awaitable<int> usleep(int sleep_us);

private:
    detail::AwaitablePromiseBase* _promise{nullptr};
    bool _waited{false};
    std::atomic<int>* _butex{nullptr};
};

} // namespace experimental
} // namespace melon

#include <melon/rpc/coroutine_inl.h>

#endif // __cplusplus >= 202002L

#endif // MELON_RPC_COROUTINE_H_