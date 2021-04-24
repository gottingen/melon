//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/internal/fiber_entity.h"

#include "gtest/gtest.h"


namespace abel {
    namespace fiber_internal {

        class SystemFiberOrNot : public ::testing::TestWithParam<bool> {
        };

        TEST_P(SystemFiberOrNot, GetMaster) {
            set_up_master_fiber_entity();

            auto *fiber = get_master_fiber_entity();
            ASSERT_TRUE(!!fiber);
            // What can we test here?
        }

        TEST_P(SystemFiberOrNot, CreateDestroy) {
            auto fiber = create_fiber_entity(nullptr, GetParam(), [] {});
            ASSERT_TRUE(!!fiber);
            free_fiber_entity(fiber);
        }

        TEST_P(SystemFiberOrNot, get_stack_top) {
            auto fiber = create_fiber_entity(nullptr, GetParam(), [] {});
            ASSERT_TRUE(fiber->get_stack_top());
            free_fiber_entity(fiber);
        }

        fiber_entity *master;

        TEST_P(SystemFiberOrNot, Switch) {
            master = get_master_fiber_entity();
            int x = 0;
            auto fiber = create_fiber_entity(nullptr, GetParam(), [&] {
                x = 10;
                // Jump back to the master fiber.
                master->resume();
            });
            fiber->resume();

            // Back from `cb`.
            ASSERT_EQ(10, x);
            free_fiber_entity(fiber);
        }

        TEST_P(SystemFiberOrNot, GetCurrent) {
            master = get_master_fiber_entity();
            ASSERT_EQ(master, get_current_fiber_entity());

            fiber_entity *ptr;
            auto fiber = create_fiber_entity(nullptr, GetParam(), [&] {
                ASSERT_EQ(get_current_fiber_entity(), ptr);
                get_master_fiber_entity()->resume();  // Equivalent to `master->resume().`
            });
            ptr = fiber;
            fiber->resume();

            ASSERT_EQ(master, get_current_fiber_entity()
            );  // We're back.
            free_fiber_entity(fiber);
        }

        TEST_P(SystemFiberOrNot, resume_on) {
            struct Context {
                fiber_entity *expected;
                bool tested = false;
            };
            bool fiber_run = false;
            auto fiber = create_fiber_entity(nullptr, GetParam(), [&] {
                get_master_fiber_entity()->resume();
                fiber_run = true;
                get_master_fiber_entity()->resume();
            });

            Context ctx;
            ctx.expected = fiber;
            fiber->resume();  // We (master fiber) will be immediately resumed by
            // `fiber_cb`.
            fiber->resume_on([&] {
                ASSERT_EQ(get_current_fiber_entity(), ctx.expected);
                ctx.tested = true;
            });

            ASSERT_TRUE(ctx
                                .tested);
            ASSERT_TRUE(fiber_run);
            ASSERT_EQ(master, get_current_fiber_entity());
            free_fiber_entity(fiber);
        }

        TEST_P(SystemFiberOrNot, Fls) {
            static const std::size_t kSlots[] = {
                    0,
                    1,
                    fiber_entity::kInlineLocalStorageSlots + 5,
                    fiber_entity::kInlineLocalStorageSlots + 9999,
            };

            for (auto slot_index: kSlots) {
                auto self = get_current_fiber_entity();
                *self->get_fls(slot_index) = make_erased<int>(5);

                bool fiber_run = false;
                auto fiber = create_fiber_entity(nullptr, GetParam(), [&] {
                    auto self = get_current_fiber_entity();
                    auto fls = self->get_fls(slot_index);
                    ASSERT_FALSE(*fls);

                    get_master_fiber_entity()->resume();

                    ASSERT_EQ(fls, self->get_fls(slot_index));
                    *fls = make_erased<int>(10);

                    get_master_fiber_entity()->resume();

                    ASSERT_EQ(fls, self->get_fls(slot_index));
                    ASSERT_EQ(10, *reinterpret_cast<int *>(fls->get()));
                    fiber_run = true;

                    get_master_fiber_entity()->resume();
                });

                ASSERT_EQ(self, get_master_fiber_entity()
                );
                auto fls = self->get_fls(slot_index);
                ASSERT_EQ(5, *reinterpret_cast
                        <int *>(fls->get()));

                fiber->resume();

                ASSERT_EQ(fls, self->get_fls(slot_index));

                fiber->resume();

                ASSERT_EQ(5, *reinterpret_cast<int *>(fls->get()));
                ASSERT_EQ(fls, self->get_fls(slot_index));
                *reinterpret_cast<int *>(fls->get()) = 7;

                fiber->resume();

                ASSERT_EQ(7, *reinterpret_cast<int *>(fls->get()));
                ASSERT_EQ(fls, self->get_fls(slot_index));

                ASSERT_TRUE(fiber_run);
                ASSERT_EQ(master, get_current_fiber_entity());
                free_fiber_entity(fiber);
            }
        }

        TEST_P(SystemFiberOrNot, ResumeOnMaster) {
            struct Context {
                fiber_entity *expected;
                bool tested = false;
            };
            Context ctx;
            auto fiber = create_fiber_entity(nullptr, GetParam(), [&] {
                master->resume_on([&] {
                    ASSERT_EQ(get_current_fiber_entity(), ctx.expected);
                    ctx.tested = true;
                    // Continue running master fiber on return.
                });
            });

            ctx.expected = get_master_fiber_entity();
            fiber->resume();

            ASSERT_TRUE(ctx.tested);
            ASSERT_EQ(master, get_current_fiber_entity());
            free_fiber_entity(fiber);
        }

        INSTANTIATE_TEST_SUITE_P(fiber_entity, SystemFiberOrNot,::testing::Values(true, false)
        );

    } //namespace fiber_internal
}  // namespace abel
