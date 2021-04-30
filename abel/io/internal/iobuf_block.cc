//
// Created by liyinbin on 2021/4/19.
//


#include "abel/io/internal/iobuf_block.h"

#include <string>
#include <unordered_map>

#include "abel/base/profile.h"
#include "abel/memory/object_pool.h"
#include "abel/memory/ref_ptr.h"

namespace abel {

    namespace {

        template<std::size_t kSize>
        struct alignas(abel::hardware_destructive_interference_size) FixedNativeBufferBlock
                : native_iobuf_block {
            char *mutable_data() noexcept override { return buffer.data(); }

            const char *data() const noexcept override { return buffer.data(); }

            std::size_t size() const noexcept override { return buffer.size(); }

            void destroy() noexcept override {
                abel::object_pool::put<FixedNativeBufferBlock>(this);
            }

            static constexpr auto kBufferSize = kSize - sizeof(native_iobuf_block);
            std::array<char, kBufferSize> buffer;
        };

        template<std::size_t kSize>
        abel::ref_ptr<native_iobuf_block> MakeNativeBufferBlockOfBytes() {
            return abel::ref_ptr(abel::adopt_ptr_v,
                                 abel::object_pool::get<FixedNativeBufferBlock<kSize>>().leak());
        }

        // We initialize this function with a default value so that UTs will always
        // work, even without touching FLARE_TEST_MAIN.
        abel::ref_ptr<native_iobuf_block> (*make_native_buffer_block)() =
        MakeNativeBufferBlockOfBytes<4096>;

    }  // namespace

    abel::ref_ptr<native_iobuf_block> make_native_ionuf_block() {
        return make_native_buffer_block();
    }

}  // namespace abel

namespace abel {

    template<>
    struct pool_traits<abel::FixedNativeBufferBlock<4096>> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 16384;  // 64M per node.
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 4096;  // 16M per thread.
        static constexpr auto kTransferBatchSize = 1024;       // Extra 4M.
    };

    template<>
    struct pool_traits<abel::FixedNativeBufferBlock<65536>> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 1024;  // 64M per node.
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 256;  // 16M per thread.
        static constexpr auto kTransferBatchSize = 64;        // Extra 4M.
    };

    template<>
    struct pool_traits<abel::FixedNativeBufferBlock<1048576>> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 128;  // 128M per node.
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 64;  // 64M per thread.
        static constexpr auto kTransferBatchSize = 16;       // Extra 16M.
    };

}  // namespace abel
