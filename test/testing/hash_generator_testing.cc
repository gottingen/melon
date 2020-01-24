
#include <testing/hash_generator_testing.h>
#include <deque>

namespace abel {

namespace container_internal {
namespace hash_internal {
namespace {

class RandomDeviceSeedSeq {
public:
    using result_type = typename std::random_device::result_type;

    template<class Iterator>
    void generate (Iterator start, Iterator end) {
        while (start != end) {
            *start = gen_();
            ++start;
        }
    }

private:
    std::random_device gen_;
};

}  // namespace

std::mt19937_64 *GetSharedRng () {
    RandomDeviceSeedSeq seed_seq;
    static auto *rng = new std::mt19937_64(seed_seq);
    return rng;
}

std::string Generator<std::string>::operator () () const {
    // NOLINTNEXTLINE(runtime/int)
    std::uniform_int_distribution<short> chars(0x20, 0x7E);
    std::string res;
    res.resize(32);
    std::generate(res.begin(), res.end(),
                  [&] () { return chars(*GetSharedRng()); });
    return res;
}

abel::string_view Generator<abel::string_view>::operator () () const {
    static auto *arena = new std::deque<std::string>();
    // NOLINTNEXTLINE(runtime/int)
    std::uniform_int_distribution<short> chars(0x20, 0x7E);
    arena->emplace_back();
    auto &res = arena->back();
    res.resize(32);
    std::generate(res.begin(), res.end(),
                  [&] () { return chars(*GetSharedRng()); });
    return res;
}

}  // namespace hash_internal
}  // namespace container_internal

}  // namespace abel
