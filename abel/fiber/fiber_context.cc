//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/fiber_context.h"

#include "abel/memory/ref_ptr.h"

namespace abel {

    fiber_local<fiber_context*> fiber_context::current_;

    void fiber_context::clear() {
        DCHECK_MSG(unsafe_ref_count() == 1,
                "Unexpected: `fiber_context` is using by others when `Clear()`-ed.");

        for (auto&& e : inline_els_) {
            // This is ugly TBH.
            e.~ElsEntry();
            new (&e) ElsEntry();
        }
        external_els_.clear();
    }

    ref_ptr<fiber_context> fiber_context::capture() {
        return ref_ptr(ref_ptr_v, *current_);
    }

    ref_ptr<fiber_context> fiber_context::create() {
        return get_ref_counted<fiber_context>();
    }

    fiber_context::ElsEntry* fiber_context::GetElsEntrySlow(
            std::size_t slot_index) {
//        ABEL_LOG_WARNING_ONCE(
//                "Excessive ELS usage. Performance will likely degrade.");
        // TODO(yinbinli): Optimize this.
        std::scoped_lock _(external_els_lock_);
        return &external_els_[slot_index];
    }

}  // namespace abel
