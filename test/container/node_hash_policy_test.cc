//

#include <abel/container/internal/node_hash_policy.h>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/container/internal/hash_policy_traits.h>

namespace abel {

    namespace container_internal {
        namespace {

            using ::testing::Pointee;

            struct Policy : node_hash_policy<int &, Policy> {
                using key_type = int;
                using init_type = int;

                template<class Alloc>
                static int *new_element(Alloc *alloc, int value) {
                    return new int(value);
                }

                template<class Alloc>
                static void delete_element(Alloc *alloc, int *elem) {
                    delete elem;
                }
            };

            using NodePolicy = hash_policy_traits<Policy>;

            struct NodeTest : ::testing::Test {
                std::allocator<int> alloc;
                int n = 53;
                int *a = &n;
            };

            TEST_F(NodeTest, ConstructDestroy) {
                NodePolicy::construct(&alloc, &a, 42);
                EXPECT_THAT(a, Pointee(42));
                NodePolicy::destroy(&alloc, &a);
            }

            TEST_F(NodeTest, transfer) {
                int s = 42;
                int *b = &s;
                NodePolicy::transfer(&alloc, &a, &b);
                EXPECT_EQ(&s, a);
            }

        }  // namespace
    }  // namespace container_internal

}  // namespace abel
