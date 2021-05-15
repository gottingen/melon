//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_CONTEXT_H_
#define ABEL_FIBER_CONTEXT_H_


#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "abel/functional/function.h"
#include "abel/fiber/internal/index_alloc.h"
#include "abel/memory/ref_ptr.h"
#include "abel/fiber/fiber_local.h"

namespace abel {

    namespace fiber_internal {

        struct ExecutionLocalIndexTag;

    }  // namespace fiber_internal

    // `fiber_context` serves as a container for all information relevant to a
    // logical fiber / a group of fibers of execution.
    //
    // The fiber runtime implicitly passes execution context in:
    //
    // - `Async`
    // - `Set(Detached)Timer`
    //
    // Note that starting a new fiber won't automatically inheriting current
    // execution context, you need to `Capture` and `Run` in it manually.
    class fiber_context : public pool_ref_counted<fiber_context> {
    public:
        // Call `cb` in this execution context.
        template<class F>
        decltype(auto) execute(F &&cb) {
            abel::scoped_deferred _([old = *current_] { *current_ = old; });
            *current_ = this;
            return std::forward<F>(cb)();
        }

        // Clear this execution context for reuse.
        void clear();

        // Capture current execution context.
        static abel::ref_ptr<fiber_context> capture();

        // Create a new execution context.
        static abel::ref_ptr<fiber_context> create();

        // Get current execution context.
        static fiber_context *current() { return *current_; }

    private:
        template<class>
        friend
        class execution_local;

        // Keep size of this structure a power of two helps code-gen.
        struct ElsEntry {
            std::atomic<void *> ptr{nullptr};

            void (*deleter)(void *);

            ~ElsEntry() {
                if (auto p = ptr.load(std::memory_order_acquire)) {
                    deleter(p);
                    deleter = nullptr;
                    ptr.store(nullptr, std::memory_order_relaxed);
                }
            }
        };

        // For the moment we do not make heavy use of execution local storage, 8
        // should be sufficient.
        static constexpr auto kInlineElsSlots = 8;
        static fiber_local<fiber_context *> current_;

        ElsEntry *GetElsEntry(std::size_t slot_index) {
            if (ABEL_LIKELY(slot_index < std::size(inline_els_))) {
                return &inline_els_[slot_index];
            }
            return GetElsEntrySlow(slot_index);
        }

        ElsEntry *GetElsEntrySlow(std::size_t slow_index);

    private:
        ElsEntry inline_els_[kInlineElsSlots];
        std::mutex external_els_lock_;
        std::unordered_map<std::size_t, ElsEntry> external_els_;

        // Lock shared by ELS initialization. Unless the execution context is
        // concurrently running in multiple threads and are all trying to initializing
        // ELS, this lock shouldn't contend too much.
        std::mutex els_init_lock_;
    };

    // Local storage a given execution context.
    //
    // Note that since execution context can be shared by multiple (possibly
    // concurrently running) fibers, access to `T` must be synchronized.
    //
    // `execution_local` guarantees thread-safety when initialize `T`.
    template<class T>
    class execution_local {
    public:
        execution_local() : slot_index_(GetIndexAlloc()->next()) {}

        ~execution_local() { GetIndexAlloc()->free(slot_index_); }

        // Accessor.
        T *operator->() const noexcept { return get(); }

        T &operator*() const noexcept { return *get(); }

        T *get() const noexcept { return Get(); }

        // Initializes the value (in a single-threaded env., as obvious). This can
        // save you the overhead of grabbing initialization lock. Beside, this allows
        // you to specify your own deleter.
        //
        // This method is provided for perf. reasons, and for the moment it's FOR
        // INTERNAL USE ONLY.
        void UnsafeInit(T *ptr, void (*deleter)(T *)) {
            auto &&entry = fiber_context::current()->GetElsEntry(slot_index_);
            DCHECK(entry,
                        "Initializing ELS must be done inside execution context.");
            DCHECK(entry->ptr.load(std::memory_order_relaxed) == nullptr,
                        "Initializeing an already-initialized ELS?");
            entry->ptr.store(ptr, std::memory_order_release);
            // FIXME: U.B. here?
            entry->deleter = reinterpret_cast<void (*)(void *)>(deleter);
        }

    private:
        T *Get() const noexcept {
            auto &&current = fiber_context::current();
            DCHECK(current,
                         "Getting ELS is only meaningful inside execution context.");

            auto &&entry = current->GetElsEntry(slot_index_);
            if (auto ptr = entry->ptr.load(std::memory_order_acquire);
                    ABEL_LIKELY(ptr)) {
                return reinterpret_cast<T *>(ptr);  // Already initialized. life is good.
            }
            return UninitializedGetSlow();
        }

        T *UninitializedGetSlow() const noexcept;

        static index_alloc *GetIndexAlloc() noexcept {
            return index_alloc::For<fiber_internal::ExecutionLocalIndexTag>();
        }

    private:
        std::size_t slot_index_;
    };

    // Calls `f`, possibly within an execution context, if one is given.
    template<class F>
    void with_fiber_context_if_present(fiber_context *ec, F &&f) {
        if (ec) {
            ec->execute(std::forward<F>(f));
        } else {
            std::forward<F>(f)();
        }
    }


    template<class T>
    T *execution_local<T>::UninitializedGetSlow() const noexcept {
        auto ectx = fiber_context::current();
        auto &&entry = ectx->GetElsEntry(slot_index_);
        std::scoped_lock _(ectx->els_init_lock_);
        if (entry->ptr.load(std::memory_order_acquire) == nullptr) {  // DCLP.
            auto ptr = std::make_unique<T>();
            entry->deleter = [](auto *p) { delete reinterpret_cast<T *>(p); };
            entry->ptr.store(ptr.release(), std::memory_order_release);
        }

        // Memory order does not matter here, the object is visible to us anyway.
        return reinterpret_cast<T *>(entry->ptr.load(std::memory_order_relaxed));
    }

}  // namespace abel

namespace abel {

    template<>
    struct pool_traits<fiber_context> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 8192;
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 1024;
        static constexpr auto kTransferBatchSize = 1024;

        // Free any resources held by `ec` prior to recycling it.
        static void OnPut(fiber_context *ec) { ec->clear(); }
    };

}  // namespace abel


#endif  // ABEL_FIBER_CONTEXT_H_
