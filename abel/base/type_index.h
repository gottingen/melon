//
// Created by liyinbin on 2021/4/2.
//

#ifndef ABEL_BASE_TYPE_INDEX_H_
#define ABEL_BASE_TYPE_INDEX_H_

#include <functional>
#include <typeindex>

namespace abel {

    namespace base_internal {

        /// for each type/class
        struct type_index_entry {
            std::type_index runtime_type_index;
        };

        template<class T>
        inline const type_index_entry kTypeIndexEntity{std::type_index(typeid(T))};

    }  // namespace base_internal

    class type_index {
    public:
        constexpr type_index() noexcept : _entity(nullptr) {}

        std::type_index get_runtime_type_index() const {
            return _entity->runtime_type_index;
        }

        constexpr bool operator==(type_index t) const noexcept {
            return _entity == t._entity;
        }

        constexpr bool operator!=(type_index t) const noexcept {
            return _entity != t._entity;
        }

        constexpr bool operator<(type_index t) const noexcept {
            return _entity < t._entity;
        }

    private:
        template<class T>
        friend constexpr type_index get_type_index();

        friend struct std::hash<type_index>;

        constexpr explicit type_index(const base_internal::type_index_entry *entity) noexcept
                : _entity(entity) {}

    private:
        const base_internal::type_index_entry *_entity;
    };

    template<class T>
    constexpr type_index get_type_index() {
        return type_index(&base_internal::kTypeIndexEntity < T > );
    }
}  // namespace abel

namespace std {

    template<>
    struct hash<abel::type_index> {
        size_t operator()(const abel::type_index &type) const noexcept {
            return std::hash<const void *>{}(type._entity);
        }
    };

}  // namespace std

#endif  // ABEL_BASE_TYPE_INDEX_H_
