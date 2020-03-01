//

#include <memory>
#include <unordered_map>

#include <test/container/unordered_map_constructor_test.h>
#include <test/container/unordered_map_lookup_test.h>
#include <test/container/unordered_map_members_test.h>
#include <test/container/unordered_map_modifiers_test.h>

namespace abel {

    namespace container_internal {
        namespace {

            using MapTypes = ::testing::Types<
                    std::unordered_map<int, int, StatefulTestingHash, StatefulTestingEqual,
                            Alloc<std::pair<const int, int>>>,
                    std::unordered_map<std::string, std::string, StatefulTestingHash,
                            StatefulTestingEqual,
                            Alloc<std::pair<const std::string, std::string>>>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, ConstructorTest, MapTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, LookupTest, MapTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, MembersTest, MapTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, ModifiersTest, MapTypes);

            using UniquePtrMapTypes = ::testing::Types<std::unordered_map<
                    int, std::unique_ptr<int>, StatefulTestingHash, StatefulTestingEqual,
                    Alloc<std::pair<const int, std::unique_ptr<int>>>>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, UniquePtrModifiersTest,
                                           UniquePtrMapTypes);

        }  // namespace
    }  // namespace container_internal

}  // namespace abel
