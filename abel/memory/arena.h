//
// Created by liyinbin on 2020/3/30.
//

#ifndef ABEL_MEMORY_ARENA_H_
#define ABEL_MEMORY_ARENA_H_


#include <stdint.h>
#include <abel/base/profile.h>

ABEL_DISABLE_CLANG_WARNING(-Wzero-length-array)
namespace abel {

    struct arena_options {
        size_t initial_block_size;
        size_t max_block_size;

        // Constructed with default options.
        arena_options();
    };

// Just a proof-of-concept, will be refactored in future CI.
    class arena {
    public:
        explicit arena(const arena_options &options = arena_options());

        ~arena();

        void swap(arena &);

        void *allocate(size_t n);

        void *allocate_aligned(size_t n);  // not implemented.
        void clear();

    private:
        ABEL_NON_COPYABLE(arena);

        struct block {
            uint32_t left_space() const { return size - alloc_size; }

            block *next;
            uint32_t alloc_size;
            uint32_t size;
            char data[0];
        };

        void *allocate_in_other_blocks(size_t n);

        void *allocate_new_block(size_t n);

        block *pop_block(block *&head) {
            block *saved_head = head;
            head = head->next;
            return saved_head;
        }

        block *_cur_block;
        block *_isolated_blocks;
        size_t _block_size;
        arena_options _options;
    };

    inline void *arena::allocate(size_t n) {
        if (_cur_block != nullptr && _cur_block->left_space() >= n) {
            void *ret = _cur_block->data + _cur_block->alloc_size;
            _cur_block->alloc_size += n;
            return ret;
        }
        return allocate_in_other_blocks(n);
    }

}  // namespace abel
ABEL_RESTORE_CLANG_WARNING()
#endif //ABEL_MEMORY_ARENA_H_
