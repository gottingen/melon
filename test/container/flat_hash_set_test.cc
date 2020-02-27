//

#include <abel/container/flat_hash_set.h>

#include <vector>

#include <testing/hash_generator_testing.h>
#include <test/container/unordered_set_constructor_test.h>
#include <test/container/unordered_set_lookup_test.h>
#include <test/container/unordered_set_members_test.h>
#include <test/container/unordered_set_modifiers_test.h>
#include <abel/memory/memory.h>
#include <abel/strings/string_view.h>

namespace abel {

    namespace container_internal {
        namespace {

            using ::abel::container_internal::hash_internal::Enum;
            using ::abel::container_internal::hash_internal::EnumClass;
            using ::testing::IsEmpty;
            using ::testing::Pointee;
            using ::testing::UnorderedElementsAre;
            using ::testing::UnorderedElementsAreArray;

            template<class T>
            using Set =
            abel::flat_hash_set<T, StatefulTestingHash, StatefulTestingEqual, Alloc<T>>;

            using SetTypes =
            ::testing::Types<Set<int>, Set<std::string>, Set<Enum>, Set<EnumClass>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, ConstructorTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, LookupTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, MembersTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, ModifiersTest, SetTypes);

            TEST(FlatHashSet, EmplaceString) {
                std::vector<std::string> v = {"a", "b"};
                abel::flat_hash_set<abel::string_view> hs(v.begin(), v.end());
                EXPECT_THAT(hs, UnorderedElementsAreArray(v));
            }

            TEST(FlatHashSet, BitfieldArgument) {
                union {
                    int n : 1;
                };
                n = 0;
                abel::flat_hash_set<int> s = {n};
                s.insert(n);
                s.insert(s.end(), n);
                s.insert({n});
                s.erase(n);
                s.count(n);
                s.prefetch(n);
                s.find(n);
                s.contains(n);
                s.equal_range(n);
            }

            TEST(FlatHashSet, MergeExtractInsert) {
                struct Hash {
                    size_t operator()(const std::unique_ptr<int> &p) const { return *p; }
                };
                struct Eq {
                    bool operator()(const std::unique_ptr<int> &a,
                                    const std::unique_ptr<int> &b) const {
                        return *a == *b;
                    }
                };
                abel::flat_hash_set<std::unique_ptr<int>, Hash, Eq> set1, set2;
                set1.insert(abel::make_unique<int>(7));
                set1.insert(abel::make_unique<int>(17));

                set2.insert(abel::make_unique<int>(7));
                set2.insert(abel::make_unique<int>(19));

                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17)));
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(19)));

                set1.merge(set2);

                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17), Pointee(19)));
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

                auto node = set1.extract(abel::make_unique<int>(7));
                EXPECT_TRUE(node);
                EXPECT_THAT(node.value(), Pointee(7));
                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(17), Pointee(19)));

                auto insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_FALSE(insert_result.inserted);
                EXPECT_TRUE(insert_result.node);
                EXPECT_THAT(insert_result.node.value(), Pointee(7));
                EXPECT_EQ(**insert_result.position, 7);
                EXPECT_NE(insert_result.position->get(), insert_result.node.value().get());
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

                node = set1.extract(abel::make_unique<int>(17));
                EXPECT_TRUE(node);
                EXPECT_THAT(node.value(), Pointee(17));
                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(19)));

                node.value() = abel::make_unique<int>(23);

                insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_TRUE(insert_result.inserted);
                EXPECT_FALSE(insert_result.node);
                EXPECT_EQ(**insert_result.position, 23);
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(23)));
            }

            bool IsEven(int k) { return k % 2 == 0; }

            TEST(FlatHashSet, EraseIf) {
                // Erase all elements.
                {
                    flat_hash_set<int> s = {1, 2, 3, 4, 5};
                    erase_if(s, [](int) { return true; });
                    EXPECT_THAT(s, IsEmpty());
                }
                // Erase no elements.
                {
                    flat_hash_set<int> s = {1, 2, 3, 4, 5};
                    erase_if(s, [](int) { return false; });
                    EXPECT_THAT(s, UnorderedElementsAre(1, 2, 3, 4, 5));
                }
                // Erase specific elements.
                {
                    flat_hash_set<int> s = {1, 2, 3, 4, 5};
                    erase_if(s, [](int k) { return k % 2 == 1; });
                    EXPECT_THAT(s, UnorderedElementsAre(2, 4));
                }
                // Predicate is function reference.
                {
                    flat_hash_set<int> s = {1, 2, 3, 4, 5};
                    erase_if(s, IsEven);
                    EXPECT_THAT(s, UnorderedElementsAre(1, 3, 5));
                }
                // Predicate is function pointer.
                {
                    flat_hash_set<int> s = {1, 2, 3, 4, 5};
                    erase_if(s, &IsEven);
                    EXPECT_THAT(s, UnorderedElementsAre(1, 3, 5));
                }
            }

        }  // namespace
    }  // namespace container_internal

}  // namespace abel
