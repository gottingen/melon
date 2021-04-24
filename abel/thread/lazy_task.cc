//
// Created by liyinbin on 2021/4/5.
//


#include "abel/thread/lazy_task.h"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

#include "abel/base/profile.h"
#include "abel/thread/biased_mutex.h"
#include "abel/memory/non_destroy.h"
#include "abel/thread/thread_local.h"


namespace abel {

    namespace {

// The callback's ref is copied to each thread's local queue, so it's
// ref-counted.
        using CallbackPtr = std::shared_ptr<abel::function<void()>>;

        struct Desc {
            std::uint64_t id;
            abel::time_point next_fires_at;
            abel::duration interval;
            CallbackPtr callback;
        };

// A mimic of `std::priority_queue`. This one provides `EraseIf` functionality.
        class Queue {
            struct DescComp;

        public:
            Desc* Top() { return &heap_.front(); }
            bool Empty() const noexcept { return heap_.empty(); }

            void Push(Desc desc) {
                heap_.push_back(std::move(desc));
                std::push_heap(heap_.begin(), heap_.end(), DescComp());
            }

            Desc Pop() {
                std::pop_heap(heap_.begin(), heap_.end(), DescComp());
                auto result = std::move(heap_.back());
                heap_.pop_back();
                return result;
            }

            void Clear() { heap_.clear(); }

            template <class F>
            void EraseIf(F&& pred) {
                heap_.erase(std::remove_if(heap_.begin(), heap_.end(), pred), heap_.end());
                std::make_heap(heap_.begin(), heap_.end(), DescComp());
            }

        private:
            struct DescComp {
                bool operator()(const Desc& left, const Desc& right) const noexcept {
                    // Entry with a smaller timestamp is ordered first.
                    return left.next_fires_at > right.next_fires_at;
                }
            };

            std::vector<Desc> heap_;
        };

        struct alignas(hardware_destructive_interference_size) ThreadLocalQueue {
            // Synchronizes with setting / deleting callbacks. (It should be obvious that,
            // in usual cases, we don't need a lock to update this structure as it's
            // thread-local.)
            //
            // This mutex is biased as we don't expect it to be held by the "slower side"
            // (callback setter / deleter) too often.
            biased_mutex lock;

            // Version of our local copy of `callbacks`.
            std::atomic<std::uint64_t> version;

            // Priority queue of pending callbacks.
            Queue callbacks;
        };

        struct GlobalQueue {
            std::atomic<std::uint64_t> version{1};
            std::mutex lock;
            std::vector<Desc> callbacks;
        };

        std::atomic<std::uint64_t> next_callback_id = 1;

        thread_local_store<ThreadLocalQueue> tls_queues;

        GlobalQueue* GetGlobalQueue() {
            static non_destroy<GlobalQueue> queue;
            return queue.get();
        }

    }  // namespace

    std::uint64_t set_thread_lazy_task(
            abel::function<void()> callback, abel::duration min_interval) {
        DCHECK_MSG(min_interval > abel::duration::seconds(0), "Hang will occur.");

        auto id = next_callback_id++;
        auto&& queue = GetGlobalQueue();
        std::scoped_lock _(queue->lock);
        queue->callbacks.push_back(Desc{
                .id = id,
                .next_fires_at = abel::time_now() + min_interval,
                .interval = min_interval,
                .callback = std::make_shared<abel::function<void()>>(std::move(callback))});
        queue->version.fetch_add(1, std::memory_order_relaxed);
        return id;
    }

    void delete_thread_lazy_task(std::uint64_t handle) {
        CallbackPtr ptr;

        // Remove if from global queue first.
        {
            auto&& queue = GetGlobalQueue();
            std::scoped_lock _(queue->lock);
            auto&& cbs = queue->callbacks;

            // Find `ptr` first. It's only used for sanity check, though.
            auto iter = std::find_if(cbs.begin(), cbs.end(),
                                     [&](auto&& e) { return e.id == handle; });
            DCHECK(iter != cbs.end());
            ptr = iter->callback;

            // Now remove the handle from global queue.
            iter = std::remove_if(cbs.begin(), cbs.end(),
                                  [&](auto&& e) { return e.id == handle; });
            DCHECK_EQ(cbs.end() - iter, 1);
            cbs.erase(iter, cbs.end());

            // Broadcast the change.
            queue->version.fetch_add(1, std::memory_order_relaxed);
        }

        // And then sweep thread-locally cached queues.
        tls_queues.for_each([&](ThreadLocalQueue* queue) {
            std::scoped_lock _(*queue->lock.get_really_slow_side());
            queue->callbacks.EraseIf([&](auto&& e) { return e.id == handle; });
        });

        DCHECK(ptr.unique());  // It shouldn't be referenced anywhere else.
        // Let `ptr` go.
    }

    void notify_thread_lazy_task() {
        auto now = abel::time_now();
        auto&& tls_queue = tls_queues.get();
        auto&& global_queue = GetGlobalQueue();

        std::scoped_lock _(*tls_queue->lock.get_blessed_side());
        auto&& cbs = tls_queue->callbacks;

        if (ABEL_UNLIKELY(global_queue->version.load(std::memory_order_relaxed) !=
                           tls_queue->version.load(std::memory_order_relaxed))) {
            // Our queue is out-of-date, update it first.
            cbs.Clear();
            std::scoped_lock _(global_queue->lock);
            for (auto&& e : global_queue->callbacks) {
                cbs.Push(e);
            }
            tls_queue->version.store(
                    global_queue->version.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
        }

        while (ABEL_UNLIKELY(!cbs.Empty() && cbs.Top()->next_fires_at < now)) {
            auto current = cbs.Pop();
            (*current.callback)();
            current.next_fires_at = now + current.interval;
            cbs.Push(std::move(current));
        }
    }

}  // namespace abel
