//
// Created by liyinbin on 2021/4/19.
//

#ifndef ABEL_IO_INTERNAL_IOBUF_BLOCK_H_
#define ABEL_IO_INTERNAL_IOBUF_BLOCK_H_

#include "abel/io/internal/iobuf_block.h"
#include <array>
#include <chrono>
#include <limits>

#include "abel/base/profile.h"
#include "abel/io/internal/iobuf_base.h"
#include "abel/memory/object_pool.h"
#include "abel/memory/ref_ptr.h"

namespace abel {


    class alignas(abel::hardware_destructive_interference_size) native_iobuf_block
            : public iobuf_block {
    public:
        virtual char* mutable_data() noexcept = 0;
    };

    // Allocate a buffer block.
    //
    // Size of the buffer block is determined by GFlags on startup, see
    // implementation for detail.
    abel::ref_ptr<native_iobuf_block> make_native_ionuf_block();

    // This buffer references a non-owning memory region.
    //
    // The buffer creator is responsible for making sure the memory region
    // referenced by this object is not mutated during the whole lifetime of this
    // object.
    //
    // This class calls user's callback on destruction. This provides a way for the
    // user to know when the buffer being referenced is safe to release.
    template <class F>
    class ref_iobuf_block : public iobuf_block,
                                   private F /* EBO */ {
    public:
        explicit ref_iobuf_block(const void* ptr, std::size_t size,
                                        F&& completion_cb)
                : F(std::move(completion_cb)), ptr_(ptr), size_(size) {}
        ~ref_iobuf_block() { (*this)(); }

        const char* data() const noexcept override {
            return reinterpret_cast<const char*>(ptr_);
        }
        std::size_t size() const noexcept override { return size_; }

    private:
        const void* ptr_;
        std::size_t size_;
    };

}  // namespace abel

#endif  // ABEL_IO_INTERNAL_IOBUF_BLOCK_H_
