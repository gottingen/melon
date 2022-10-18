
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_STORAGE_FWD_H_
#define MELON_FUTURE_DETAIL_STORAGE_FWD_H_


#include "melon/future/detail/utility.h"
#include <atomic>
#include <type_traits>

namespace melon::future_internal {

    template<typename... Ts>
    struct segmented_callback_result {
        std::tuple<Ts...> values_;
    };

    template<typename... Ts>
    class future_handler_interface {
    public:
        using full_fill_type = full_fill_type_t<Ts...>;
        using fail_type = fail_type_t<Ts...>;
        using finish_type = finish_type_t<Ts...>;

        future_handler_interface() = default;

        future_handler_interface(const future_handler_interface &) = delete;

        future_handler_interface(future_handler_interface &&) = delete;

        future_handler_interface &operator=(const future_handler_interface &) = delete;

        future_handler_interface &operator=(future_handler_interface &&) = delete;

        virtual ~future_handler_interface() {}

        // The future has been completed
        virtual void full_fill(full_fill_type) = 0;

        // The future has been potentially completed.
        // There may be 0 or all errors.
        virtual void finish(finish_type) = 0;

        // The future has been failed.
        // virtual void fail(fail_type) = 0;
    };

    template<typename QueueT, typename Enable = void, typename... Ts>
    class future_handler_base : public future_handler_interface<Ts...> {
    public:
        future_handler_base(QueueT *q) : queue_(q) {}

    protected:
        QueueT *get_queue() { return queue_; }

    private:
        QueueT *queue_;
    };

    // If the queue's push() method is static, do not bother storing the pointer.
    template<typename QueueT, typename... Ts>
    class future_handler_base<QueueT, std::enable_if_t<has_static_push_v<QueueT>>,
            Ts...> : public future_handler_interface<Ts...> {
    public:
        future_handler_base(QueueT *) {}

    protected:
        constexpr static QueueT *get_queue() { return nullptr; }
    };

    constexpr std::uint8_t Future_storage_state_ready_bit = 1;
    constexpr std::uint8_t Future_storage_state_finished_bit = 2;

    // Holds the shared state associated with a Future<>.
    template<typename Alloc, typename... Ts>
    class future_storage : public Alloc {
        template<typename T>
        friend
        struct storage_ptr;

        future_storage(const Alloc &alloc);

    public:
        using allocator_type = Alloc;

        using full_fill_type = full_fill_type_t<Ts...>;
        using fail_type = fail_type_t<Ts...>;
        using finish_type = finish_type_t<Ts...>;

        using future_type = basic_future<Alloc, Ts...>;

        ~future_storage();

        void full_fill(full_fill_type &&v);

        template<typename Arg_alloc>
        void full_fill(basic_future<Arg_alloc, Ts...> &&f);

        template<typename... Us>
        void full_fill(segmented_callback_result<Us...> &&f);

        void finish(finish_type &&f);

        template<typename Arg_alloc>
        void finish(basic_future<Arg_alloc, Ts...> &&f);

        void fail(fail_type &&e);

        template<typename Handler_t, typename QueueT, typename... Args_t>
        void set_handler(QueueT *queue, Args_t &&... args);

        Alloc &allocator() { return *static_cast<Alloc *>(this); }

        const Alloc &allocator() const { return *static_cast<Alloc *>(this); }

    private:
        struct callback_data {
            future_handler_interface<Ts...> *callback_ = nullptr;
        };

        callback_data cb_data_;

        // finished is in an union because it only gets constructed on demand.
        union {
            finish_type finished_;
        };

        // storage_ptr support
        template<typename T>
        friend
        struct storage_ptr;

        std::atomic<std::uint8_t> state_ = 0;
        std::atomic<std::uint8_t> ref_count_ = 0;
    };

    // Yes, we are using a custom std::shared_ptr<> alternative. This is because
    // common handlers have a owning pointer to a future_storage, and each byte
    // saved increases the likelyhood that it will fit in SBO, which has very large
    // performance impact.
    template<typename T>
    struct storage_ptr {
        storage_ptr() = default;

        storage_ptr(T *val) : ptr_(val) {
            assert(val);
            inc();
        }

        storage_ptr(const storage_ptr &rhs) : ptr_(rhs.ptr_) {
            if (ptr_) {
                inc();
            }
        }

        storage_ptr(storage_ptr &&rhs) : ptr_(rhs.ptr_) { rhs.ptr_ = nullptr; }

        storage_ptr &operator=(const storage_ptr &rhs) {
            ptr_ = rhs.ptr_;
            if (ptr_) {
                inc();
            }
        }

        storage_ptr &operator=(storage_ptr &&rhs) {
            clear();

            ptr_ = rhs.ptr_;
            rhs.ptr_ = nullptr;

            return *this;
        }

        void clear() {
            if (ptr_) {
                if (ptr_->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    using alloc_traits = std::allocator_traits<typename T::allocator_type>;
                    using Alloc = typename alloc_traits::template rebind_alloc<T>;

                    Alloc real_alloc(ptr_->allocator());
                    ptr_->~T();
                    real_alloc.deallocate(ptr_, 1);
                }
            }
        }

        void reset() {
            clear();
            ptr_ = nullptr;
        }

        operator bool() const { return ptr_ != nullptr; }

        ~storage_ptr() { clear(); }

        T &operator*() const { return *ptr_; }

        T *operator->() const { return ptr_; }

        void allocate(typename T::allocator_type const &alloc) {
            clear();
            using alloc_traits = std::allocator_traits<typename T::allocator_type>;
            using Alloc = typename alloc_traits::template rebind_alloc<T>;

            Alloc real_alloc(alloc);
            T *new_ptr = real_alloc.allocate(1);
            try {
                ptr_ = new(new_ptr) T(alloc);
                inc();
            } catch (...) {
                real_alloc.deallocate(new_ptr, 1);
            }
        }

    private:
        void inc() { ptr_->ref_count_.fetch_add(1, std::memory_order_relaxed); }

        T *ptr_ = nullptr;
    };

    template<typename Alloc, typename T>
    struct storage_for_cb_result {
        using type = future_storage<Alloc, decay_future_t<T>>;
    };

    template<typename Alloc, typename T>
    struct storage_for_cb_result<Alloc, expected < T, std::exception_ptr>> {
        using type = typename storage_for_cb_result<Alloc, T>::type;
    };

    template<typename Alloc, typename... Us>
    struct storage_for_cb_result<Alloc, segmented_callback_result < Us...>> {
    using type = future_storage<Alloc, decay_future_t<Us>...>;
    };

    template<typename Alloc, typename T>
    using storage_for_cb_result_t = typename storage_for_cb_result<Alloc, T>::type;

}  // namespace melon::future_internal
#endif  // MELON_FUTURE_DETAIL_STORAGE_FWD_H_
