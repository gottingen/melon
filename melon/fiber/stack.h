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


#ifndef MELON_FIBER_ALLOCATE_STACK_H_
#define MELON_FIBER_ALLOCATE_STACK_H_

#include <assert.h>
#include <gflags/gflags.h>          // DECLARE_int32
#include <melon/fiber/types.h>
#include <melon/fiber/context.h>        // fiber_fcontext_t
#include <melon/utility/object_pool.h>

namespace fiber {

    struct StackStorage {
        int stacksize;
        int guardsize;
        // Assume stack grows upwards.
        // http://www.boost.org/doc/libs/1_55_0/libs/context/doc/html/context/stack.html
        void *bottom;
        unsigned valgrind_stack_id;

        // Clears all members.
        void zeroize() {
            stacksize = 0;
            guardsize = 0;
            bottom = NULL;
            valgrind_stack_id = 0;
        }
    };

    // Allocate a piece of stack.
    int allocate_stack_storage(StackStorage *s, int stacksize, int guardsize);

    // Deallocate a piece of stack. Parameters MUST be returned or set by the
    // corresponding allocate_stack_storage() otherwise behavior is undefined.
    void deallocate_stack_storage(StackStorage *s);

    enum StackType {
        STACK_TYPE_MAIN = 0,
        STACK_TYPE_PTHREAD = FIBER_STACKTYPE_PTHREAD,
        STACK_TYPE_SMALL = FIBER_STACKTYPE_SMALL,
        STACK_TYPE_NORMAL = FIBER_STACKTYPE_NORMAL,
        STACK_TYPE_LARGE = FIBER_STACKTYPE_LARGE
    };

    struct ContextualStack {
        fiber_fcontext_t context;
        StackType stacktype;
        StackStorage storage;
    };

    // Get a stack in the `type' and run `entry' at the first time that the
    // stack is jumped.
    ContextualStack *get_stack(StackType type, void (*entry)(intptr_t));

    // Recycle a stack. NULL does nothing.
    void return_stack(ContextualStack *);

    // Jump from stack `from' to stack `to'. `from' must be the stack of callsite
    // (to save contexts before jumping)
    void jump_stack(ContextualStack *from, ContextualStack *to);

}  // namespace fiber

#include <melon/fiber/stack_inl.h>

#endif  // MELON_FIBER_ALLOCATE_STACK_H_
