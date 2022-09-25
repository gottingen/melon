#ifndef MELON_CONTAINER_FLAT_HASH_MAP_DUMP_H_
#define MELON_CONTAINER_FLAT_HASH_MAP_DUMP_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include "melon/container/flat_hash_map.h"

namespace melon {

    namespace type_traits_internal {

#if defined(__GLIBCXX__) && __GLIBCXX__ < 20150801
        template<typename T> struct IsTriviallyCopyable : public std::integral_constant<bool, __has_trivial_copy(T)> {};
#else
        template<typename T>
        struct IsTriviallyCopyable : public std::is_trivially_copyable<T> {
        };
#endif

        template<class T1, class T2>
        struct IsTriviallyCopyable<std::pair<T1, T2>> {
            static constexpr bool value = IsTriviallyCopyable<T1>::value && IsTriviallyCopyable<T2>::value;
        };
    }

    namespace priv {

#if !defined(MELON_MAP_NON_DETERMINISTIC) && !defined(MELON_MAP_DISABLE_DUMP)

        // ------------------------------------------------------------------------
        // dump/load for raw_hash_set
        // ------------------------------------------------------------------------
        template<class Policy, class Hash, class Eq, class Alloc>
        template<typename OutputArchive>
        bool raw_hash_set<Policy, Hash, Eq, Alloc>::melon_map_dump(OutputArchive &ar) const {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");

            ar.save_binary(&size_, sizeof(size_t));
            if (size_ == 0)
                return true;
            ar.save_binary(&capacity_, sizeof(size_t));
            ar.save_binary(ctrl_, sizeof(ctrl_t) * (capacity_ + Group::kWidth + 1));
            ar.save_binary(slots_, sizeof(slot_type) * capacity_);
            return true;
        }

        template<class Policy, class Hash, class Eq, class Alloc>
        template<typename InputArchive>
        bool raw_hash_set<Policy, Hash, Eq, Alloc>::melon_map_load(InputArchive &ar) {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");
            raw_hash_set<Policy, Hash, Eq, Alloc>().swap(*this); // clear any existing content
            ar.load_binary(&size_, sizeof(size_t));
            if (size_ == 0)
                return true;
            ar.load_binary(&capacity_, sizeof(size_t));

            // allocate memory for ctrl_ and slots_
            initialize_slots(capacity_);
            ar.load_binary(ctrl_, sizeof(ctrl_t) * (capacity_ + Group::kWidth + 1));
            ar.load_binary(slots_, sizeof(slot_type) * capacity_);
            return true;
        }

        // ------------------------------------------------------------------------
        // dump/load for parallel_hash_set
        // ------------------------------------------------------------------------
        template<size_t N,
                template<class, class, class, class> class RefSet,
                class Mtx_,
                class Policy, class Hash, class Eq, class Alloc>
        template<typename OutputArchive>
        bool parallel_hash_set<N, RefSet, Mtx_, Policy, Hash, Eq, Alloc>::melon_map_dump(OutputArchive &ar) const {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");

            size_t submap_count = subcnt();
            ar.save_binary(&submap_count, sizeof(size_t));
            for (size_t i = 0; i < sets_.size(); ++i) {
                auto &inner = sets_[i];
                typename Lockable::UniqueLock m(const_cast<Inner &>(inner));
                if (!inner.set_.melon_map_dump(ar)) {
                    std::cerr << "Failed to dump submap " << i << std::endl;
                    return false;
                }
            }
            return true;
        }

        template<size_t N,
                template<class, class, class, class> class RefSet,
                class Mtx_,
                class Policy, class Hash, class Eq, class Alloc>
        template<typename InputArchive>
        bool parallel_hash_set<N, RefSet, Mtx_, Policy, Hash, Eq, Alloc>::melon_map_load(InputArchive &ar) {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");

            size_t submap_count = 0;
            ar.load_binary(&submap_count, sizeof(size_t));
            if (submap_count != subcnt()) {
                std::cerr << "submap count(" << submap_count << ") != N(" << N << ")" << std::endl;
                return false;
            }

            for (size_t i = 0; i < submap_count; ++i) {
                auto &inner = sets_[i];
                typename Lockable::UniqueLock m(const_cast<Inner &>(inner));
                if (!inner.set_.melon_map_load(ar)) {
                    std::cerr << "Failed to load submap " << i << std::endl;
                    return false;
                }
            }
            return true;
        }

#endif // !defined(MELON_MAP_NON_DETERMINISTIC) && !defined(MELON_MAP_DISABLE_DUMP)

    } // namespace priv



    // ------------------------------------------------------------------------
    // BinaryArchive
    //       File is closed when archive object is destroyed
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    class binary_output_archive {
    public:
        binary_output_archive(const char *file_path) {
            ofs_.open(file_path, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
        }

        bool save_binary(const void *p, size_t sz) {
            ofs_.write(reinterpret_cast<const char *>(p), sz);
            return true;
        }

    private:
        std::ofstream ofs_;
    };


    class binary_input_archive {
    public:
        binary_input_archive(const char *file_path) {
            ifs_.open(file_path, std::ofstream::in | std::ofstream::binary);
        }

        bool load_binary(void *p, size_t sz) {
            ifs_.read(reinterpret_cast<char *>(p), sz);
            return true;
        }

    private:
        std::ifstream ifs_;
    };

} // namespace melon


#ifdef CEREAL_SIZE_TYPE

template <class T>
using melon_triv_copyable = typename melon::type_traits_internal::IsTriviallyCopyable<T>;
    
namespace cereal
{
    // Overload Cereal serialization code for melon::flat_hash_map
    // -----------------------------------------------------------
    template <class K, class V, class Hash, class Eq, class A>
    void save(typename std::enable_if<melon_triv_copyable<K>::value && melon_triv_copyable<V>::value, typename cereal::binary_output_archive>::type &ar,
              melon::flat_hash_map<K, V, Hash, Eq, A> const &hmap)
    {
        hmap.melon_map_dump(ar);
    }

    template <class K, class V, class Hash, class Eq, class A>
    void load(typename std::enable_if<melon_triv_copyable<K>::value && melon_triv_copyable<V>::value, typename cereal::binary_input_archive>::type &ar,
              melon::flat_hash_map<K, V, Hash, Eq, A>  &hmap)
    {
        hmap.melon_map_load(ar);
    }


    // Overload Cereal serialization code for melon::parallel_flat_hash_map
    // --------------------------------------------------------------------
    template <class K, class V, class Hash, class Eq, class A, size_t N, class Mtx_>
    void save(typename std::enable_if<melon_triv_copyable<K>::value && melon_triv_copyable<V>::value, typename cereal::binary_output_archive>::type &ar,
              melon::parallel_flat_hash_map<K, V, Hash, Eq, A, N, Mtx_> const &hmap)
    {
        hmap.melon_map_dump(ar);
    }

    template <class K, class V, class Hash, class Eq, class A, size_t N, class Mtx_>
    void load(typename std::enable_if<melon_triv_copyable<K>::value && melon_triv_copyable<V>::value, typename cereal::binary_input_archive>::type &ar,
              melon::parallel_flat_hash_map<K, V, Hash, Eq, A, N, Mtx_>  &hmap)
    {
        hmap.melon_map_load(ar);
    }

    // Overload Cereal serialization code for melon::flat_hash_set
    // -----------------------------------------------------------
    template <class K, class Hash, class Eq, class A>
    void save(typename std::enable_if<melon_triv_copyable<K>::value, typename cereal::binary_output_archive>::type &ar,
              melon::flat_hash_set<K, Hash, Eq, A> const &hset)
    {
        hset.melon_map_dump(ar);
    }

    template <class K, class Hash, class Eq, class A>
    void load(typename std::enable_if<melon_triv_copyable<K>::value, typename cereal::binary_input_archive>::type &ar,
              melon::flat_hash_set<K, Hash, Eq, A>  &hset)
    {
        hset.melon_map_load(ar);
    }

    // Overload Cereal serialization code for melon::parallel_flat_hash_set
    // --------------------------------------------------------------------
    template <class K, class Hash, class Eq, class A, size_t N, class Mtx_>
    void save(typename std::enable_if<melon_triv_copyable<K>::value, typename cereal::binary_output_archive>::type &ar,
              melon::parallel_flat_hash_set<K, Hash, Eq, A, N, Mtx_> const &hset)
    {
        hset.melon_map_dump(ar);
    }

    template <class K, class Hash, class Eq, class A, size_t N, class Mtx_>
    void load(typename std::enable_if<melon_triv_copyable<K>::value, typename cereal::binary_input_archive>::type &ar,
              melon::parallel_flat_hash_set<K, Hash, Eq, A, N, Mtx_>  &hset)
    {
        hset.melon_map_load(ar);
    }
}

#endif


#endif  // MELON_CONTAINER_FLAT_HASH_MAP_DUMP_H_
