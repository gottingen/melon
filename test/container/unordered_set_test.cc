//

#include <unordered_set>

#include <test/container/unordered_set_constructor_test.h>
#include <test/container/unordered_set_lookup_test.h>
#include <test/container/unordered_set_members_test.h>
#include <test/container/unordered_set_modifiers_test.h>

namespace abel {

    namespace container_internal {
        namespace {

            using SetTypes = ::testing::Types<
                    std::unordered_set<int, StatefulTestingHash, StatefulTestingEqual,
                            Alloc<int>>,
                    std::unordered_set<std::string, StatefulTestingHash, StatefulTestingEqual,
                            Alloc<std::string>>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, ConstructorTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, LookupTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, MembersTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, ModifiersTest, SetTypes);

        }  // namespace
    }  // namespace container_internal

}  // namespace abel
