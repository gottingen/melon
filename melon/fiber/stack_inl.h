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


#pragma once

#include <melon/fiber/config.h>

namespace fiber {

    struct MainStackClass {
    };

    struct SmallStackClass {
        static int stack_size_flag();
        // Older gcc does not allow static const enum, use int instead.
        static const int stacktype = (int) STACK_TYPE_SMALL;
    };

    struct NormalStackClass {
        static int stack_size_flag();
        static const int stacktype = (int) STACK_TYPE_NORMAL;
    };

    struct LargeStackClass {
        static int stack_size_flag();
        static const int stacktype = (int) STACK_TYPE_LARGE;
    };

    template<typename StackClass>
    struct StackFactory {
        struct Wrapper : public ContextualStack {
            explicit Wrapper(void (*entry)(intptr_t)) {
                if (allocate_stack_storage(&storage, StackClass::stack_size_flag(),
                                           turbo::get_flag(FLAGS_guard_page_size)) != 0) {
                    storage.zeroize();
                    context = NULL;
                    return;
                }
                context = fiber_make_fcontext(storage.bottom, storage.stacksize, entry);
                stacktype = (StackType) StackClass::stacktype;
            }

            ~Wrapper() {
                if (context) {
                    context = NULL;
                    deallocate_stack_storage(&storage);
                    storage.zeroize();
                }
            }
        };

        static ContextualStack *get_stack(void (*entry)(intptr_t)) {
            return mutil::get_object<Wrapper>(entry);
        }

        static void return_stack(ContextualStack *sc) {
            mutil::return_object(static_cast<Wrapper *>(sc));
        }
    };

    template<>
    struct StackFactory<MainStackClass> {
        static ContextualStack *get_stack(void (*)(intptr_t)) {
            ContextualStack *s = new(std::nothrow) ContextualStack;
            if (NULL == s) {
                return NULL;
            }
            s->context = NULL;
            s->stacktype = STACK_TYPE_MAIN;
            s->storage.zeroize();
            return s;
        }

        static void return_stack(ContextualStack *s) {
            delete s;
        }
    };

    inline ContextualStack *get_stack(StackType type, void (*entry)(intptr_t)) {
        switch (type) {
            case STACK_TYPE_PTHREAD:
                return NULL;
            case STACK_TYPE_SMALL:
                return StackFactory<SmallStackClass>::get_stack(entry);
            case STACK_TYPE_NORMAL:
                return StackFactory<NormalStackClass>::get_stack(entry);
            case STACK_TYPE_LARGE:
                return StackFactory<LargeStackClass>::get_stack(entry);
            case STACK_TYPE_MAIN:
                return StackFactory<MainStackClass>::get_stack(entry);
        }
        return NULL;
    }

    inline void return_stack(ContextualStack *s) {
        if (NULL == s) {
            return;
        }
        switch (s->stacktype) {
            case STACK_TYPE_PTHREAD:
                assert(false);
                return;
            case STACK_TYPE_SMALL:
                return StackFactory<SmallStackClass>::return_stack(s);
            case STACK_TYPE_NORMAL:
                return StackFactory<NormalStackClass>::return_stack(s);
            case STACK_TYPE_LARGE:
                return StackFactory<LargeStackClass>::return_stack(s);
            case STACK_TYPE_MAIN:
                return StackFactory<MainStackClass>::return_stack(s);
        }
    }

    inline void jump_stack(ContextualStack *from, ContextualStack *to) {
        fiber_jump_fcontext(&from->context, to->context, 0/*not skip remained*/);
    }

}  // namespace fiber

namespace mutil {

    template<>
    struct ObjectPoolBlockMaxItem<
            fiber::StackFactory<fiber::LargeStackClass>::Wrapper> {
        static const size_t value = 64;
    };
    template<>
    struct ObjectPoolBlockMaxItem<
            fiber::StackFactory<fiber::NormalStackClass>::Wrapper> {
        static const size_t value = 64;
    };

    template<>
    struct ObjectPoolBlockMaxItem<
            fiber::StackFactory<fiber::SmallStackClass>::Wrapper> {
        static const size_t value = 64;
    };

    template<>
    struct ObjectPoolFreeChunkMaxItem<
            fiber::StackFactory<fiber::SmallStackClass>::Wrapper> {
        inline static size_t value() {
            return (turbo::get_flag(FLAGS_tc_stack_small) <= 0 ? 0 : turbo::get_flag(FLAGS_tc_stack_small));
        }
    };

    template<>
    struct ObjectPoolFreeChunkMaxItem<
            fiber::StackFactory<fiber::NormalStackClass>::Wrapper> {
        inline static size_t value() {
            return (turbo::get_flag(FLAGS_tc_stack_normal) <= 0 ? 0 : turbo::get_flag(FLAGS_tc_stack_normal));
        }
    };

    template<>
    struct ObjectPoolFreeChunkMaxItem<
            fiber::StackFactory<fiber::LargeStackClass>::Wrapper> {
        inline static size_t value() { return 1UL; }
    };

    template<>
    struct ObjectPoolValidator<
            fiber::StackFactory<fiber::LargeStackClass>::Wrapper> {
        inline static bool validate(
                const fiber::StackFactory<fiber::LargeStackClass>::Wrapper *w) {
            return w->context != NULL;
        }
    };

    template<>
    struct ObjectPoolValidator<
            fiber::StackFactory<fiber::NormalStackClass>::Wrapper> {
        inline static bool validate(
                const fiber::StackFactory<fiber::NormalStackClass>::Wrapper *w) {
            return w->context != NULL;
        }
    };

    template<>
    struct ObjectPoolValidator<
            fiber::StackFactory<fiber::SmallStackClass>::Wrapper> {
        inline static bool validate(
                const fiber::StackFactory<fiber::SmallStackClass>::Wrapper *w) {
            return w->context != NULL;
        }
    };

}  // namespace mutil
